#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "promise.h"
#include "map/map.h"

typedef struct
{
    /** @type {Map<promise_handle_t, promise_t*>} */
    map_handle_t promises;
    void* id_seed;
} promise_manager_t;

typedef struct promise_handler_s
{
    struct promise_handler_s* next;
    promise_then_handler_t then;
    void* then_ctx;
    promise_catch_handler_t catch;
    void* catch_ctx;
} promise_handler_t;

typedef struct
{
    promise_handler_t* first_handler;
    promise_handler_t* last_handler;
    bool resolved;
    promise_data_t resolve_data;
    void(*free_data)(void* data, void* ctx);
    void* free_data_ctx;
    bool data_taken_over;
    bool rejected;
    promise_data_t reject_reason;
    void(*free_reason)(void* reason, void* ctx);
    void* free_reason_ctx;
    bool reason_taken_over;
} promise_t;

static void promise_free(promise_t* promise);
static void promise_free_with_opt(promise_t* promise, bool do_free_data, bool do_free_reason);
static void promise_destroy_with_ctx(void* data, void* ctx);


promise_manager_handle_t promise_manager_new()
{
    promise_manager_t* manager = malloc(sizeof(promise_manager_t));
    if(!manager)
        goto error;
    memset(manager,0,sizeof(promise_manager_t));
    
    manager->id_seed = NULL+1;
    manager->promises = map_create();
    if(!manager->promises)
        goto error;

    return (promise_manager_handle_t)manager;
error:
    promise_manager_free((promise_manager_handle_t)manager);
    return NULL;
}

void promise_manager_free(promise_manager_handle_t manager_handle)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    if(manager)
    {
        if(manager->promises)
            map_delete(manager->promises,promise_destroy_with_ctx,NULL);
        free(manager);
    }
}


promise_handle_t promise_new(promise_manager_handle_t manager_handle)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    promise_t* promise = NULL;
    if(!manager)
        goto error;
    promise = malloc(sizeof(promise_t));
    if(!promise)
        goto error;
    memset(promise,0,sizeof(promise_t));
    promise->first_handler = NULL;
    promise->last_handler = NULL;
    promise->resolved = false;
    promise->rejected = false;
    promise->free_data = NULL;
    promise->free_reason = NULL;
    promise->data_taken_over = false;
    promise->reason_taken_over = false;
    promise_handle_t promise_handle = manager->id_seed++;
    /** promise handle should never overlap, not handled here */
    if(map_add(manager->promises,&promise_handle,sizeof(promise_handle),promise)==NULL)
        goto error;
    return promise_handle;
error:
    promise_free(promise);
    return NULL;
}

void promise_destroy(promise_manager_handle_t manager_handle, promise_handle_t promise_handle)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    if(!manager)
        return;
    promise_t* promise = map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
    promise_free_with_opt(promise,true,true);
}

int promise_resolve(
    promise_manager_handle_t manager_handle, promise_handle_t promise_handle, 
    promise_data_t data, void(*free_data)(void*,void*), void* ctx)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    if(!manager)
        goto error;
    promise_t* promise = map_get(manager->promises,&promise_handle,sizeof(promise_handle));
    if(!promise)
        goto error;
    if(promise->resolved || promise->rejected)
        goto error;
    promise->resolved = true;
    promise->resolve_data = data;
    promise->free_data = free_data;
    promise->free_data_ctx = ctx;
    if(promise->first_handler != NULL)
    {
        /** If there are handlers set */
        promise_handler_t* handler = promise->first_handler;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            handler->then(promise->resolve_data,handler->then_ctx);
            handler = next_handler;
        }
        /** free promise and remove from promises */
        promise_free(promise);
        map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
    } 
    return 0;
error:
    return -1;
}

int promise_reject(
    promise_manager_handle_t manager_handle, promise_handle_t promise_handle, 
    promise_data_t reason, void(*free_reason)(void*,void*), void* ctx)
{
        promise_manager_t* manager = (promise_manager_t*)manager_handle;
    if(!manager)
        goto error;
    promise_t* promise = map_get(manager->promises,&promise_handle,sizeof(promise_handle));
    if(!promise)
        goto error;
    if(promise->resolved || promise->rejected)
        goto error;
    promise->rejected = true;
    promise->reject_reason = reason;
    promise->free_reason = free_reason;
    promise->free_reason_ctx = ctx;
    if(promise->first_handler != NULL)
    {
        /** If there are handlers set */
        promise_handler_t* handler = promise->first_handler;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            handler->catch(promise->reject_reason,handler->catch_ctx);
            handler = next_handler;
        }
        /** free promise and remove from promises */
        promise_free(promise);
        map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
    } 
    return 0;
error:
    return -1;
}

int promise_then_and_catch(
    promise_manager_handle_t manager_handle, promise_handle_t promise_handle, 
    promise_then_handler_t then, void* then_ctx, bool takeover_data,
    promise_catch_handler_t catch, void* catch_ctx, bool takeover_reason)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    if(!manager)
        goto error;
    promise_t* promise = map_get(manager->promises,&promise_handle,sizeof(promise_handle));
    if(!promise)
        goto error;
    if((!then) || (!catch))
        goto error;
    if(promise->data_taken_over && takeover_data)
        goto error;
    if(promise->reason_taken_over && takeover_reason)
        goto error;
    promise_handler_t* new_handler = malloc(sizeof(promise_handler_t));
    if(!new_handler)
        goto error;
    memset(new_handler,0,sizeof(promise_handler_t));
    new_handler->then = then;
    new_handler->then_ctx = then_ctx;
    new_handler->catch = catch;
    new_handler->catch_ctx = catch_ctx;
    new_handler->next = NULL;
    if(promise->last_handler == NULL)
    {
        promise->first_handler = new_handler;
        promise->last_handler = new_handler;
    }
    else
    {
        promise->last_handler->next = new_handler;
        promise->last_handler = new_handler;
    }
    promise->data_taken_over = promise->data_taken_over || takeover_data;
    promise->reason_taken_over = promise->reason_taken_over || takeover_reason;
    if(promise->resolved)
    {
        /** Promise is already resolved but not handled */
        promise_handler_t* handler = promise->first_handler;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            handler->then(promise->resolve_data,handler->then_ctx);
            handler = next_handler;
        }
        /** free promise and remove from promises */
        promise_free(promise);
        map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
    }
    else if(promise->rejected)
    {
        /** Promise is already rejected but not catched */
        promise_handler_t* handler = promise->first_handler;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            handler->catch(promise->reject_reason,handler->catch_ctx);
            handler = next_handler;
        }
        /** free promise and remove from promises */
        promise_free(promise);
        map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
    }
    return 0;
error:
    return -1;
}

/** static functions */

static void promise_free_with_opt(promise_t* promise, bool do_free_data, bool do_free_reason)
{
    if(promise)
    {
        if(promise->free_data && do_free_data)
            promise->free_data(promise->resolve_data.ptr,promise->free_data_ctx);
        if(promise->free_reason && do_free_reason)
            promise->free_reason(promise->reject_reason.ptr,promise->free_reason_ctx);
        promise_handler_t* handler = promise->first_handler;
        while(handler)
        {
            promise_handler_t* next = handler->next;
            free(handler);
            handler = next;
        }
        free(promise);
    }
}

static void promise_free(promise_t* promise)
{
    if(promise)
        promise_free_with_opt(promise,!promise->data_taken_over,!promise->reason_taken_over);
}

static void promise_destroy_with_ctx(void* data, void* ctx)
{
    promise_t* promise = (promise_t*)data;
    promise_free_with_opt(promise,true,true);
}
