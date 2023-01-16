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
    bool takeover_data;
    bool takeover_reason;
} promise_handler_t;

typedef struct
{
    promise_handler_t* first_handler;
    promise_handler_t* last_handler;
    /** resolve */
    bool resolved;
    promise_data_t resolve_data;
    void(*free_data)(void* data, void* ctx);
    void* free_data_ctx;
    bool data_booked;               /** if there is already a handler booked the data */
    bool data_taken_over;           /** if the data is already taken over by a handler */
    /** reject */
    bool rejected;
    promise_data_t reject_reason;
    void(*free_reason)(void* reason, void* ctx);
    void* free_reason_ctx;
    bool reason_booked;             /** if there is already a handler booked the reason */
    bool reason_taken_over;         /** if the reason is already taken over by a handler */
    /** internal use */
    struct
    {
        void* data;
        void(*free_data)(void*, void*);
        void* free_ctx;  
    } internal;
} promise_t;

static void promise_free(promise_t* promise);
static void promise_free_with_ctx(void* data, void* ctx);

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
            map_delete(manager->promises,promise_free_with_ctx,NULL);
        free(manager);
    }
}

static promise_handle_t promise_new_internal(
    promise_manager_handle_t manager_handle, 
    void* user_data, void(*free_user_data)(void*,void*),void* free_user_data_ctx)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    promise_t* promise = NULL;
    if(!manager)
        goto error;
    promise = malloc(sizeof(promise_t));
    if(!promise)
        goto error;
    memset(promise,0,sizeof(promise_t));
    promise->internal.data = user_data;
    promise->internal.free_data = free_user_data;
    promise->internal.free_ctx = free_user_data_ctx;
    promise->first_handler = NULL;
    promise->last_handler = NULL;
    promise->resolved = false;
    promise->rejected = false;
    promise->free_data = NULL;
    promise->free_reason = NULL;
    promise->data_taken_over = false;
    promise->reason_taken_over = false;
    promise->data_booked = false;
    promise->reason_booked = false;
    promise_handle_t promise_handle = manager->id_seed++;
    /** promise handle should never overlap, not handled here */
    if(map_add(manager->promises,&promise_handle,sizeof(promise_handle),promise)==NULL)
        goto error;
    return promise_handle;
error:
    promise_free(promise);
    return NULL;
}

promise_handle_t promise_new(promise_manager_handle_t manager_handle)
{
    return promise_new_internal(manager_handle,NULL,NULL,NULL);
}

void promise_destroy(promise_manager_handle_t manager_handle, promise_handle_t promise_handle)
{
    promise_manager_t* manager = (promise_manager_t*)manager_handle;
    if(!manager)
        return;
    promise_t* promise = map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
    promise_free(promise);
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
        promise_handler_t* takeover_handler = NULL;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            if(handler->takeover_data)
                takeover_handler = handler; /** there can be at most one takeover handler */
            else
                handler->then(promise->resolve_data,handler->then_ctx,NULL,NULL);
            handler = next_handler;
        }
        if(takeover_handler)    /** call the takeover handler last */
        {
            promise->data_taken_over = true;
            takeover_handler->then(promise->resolve_data,takeover_handler->then_ctx,free_data,ctx);
        }
        /** free promise and remove from promises if it is still in promises */
        promise_t* old_promise = map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
        promise_free(old_promise);
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
        promise_handler_t* takeover_handler = NULL;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            if(handler->takeover_reason)
                takeover_handler = handler;
            else
                handler->catch(promise->reject_reason,handler->catch_ctx,NULL,NULL);
            handler = next_handler;
        }
        if(takeover_handler)    /** call the take over handler last */
        {
            promise->reason_taken_over = true;
            takeover_handler->catch(promise->reject_reason,takeover_handler->catch_ctx,free_reason,ctx);
        }
        /** free promise and remove from promises if it is still in promises */
        promise_t* old_promise = map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
        promise_free(old_promise);
    }
    return 0;
error:
    return -1;
}

int promise_await(
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
    if(promise->data_booked && takeover_data)
        goto error;
    if(promise->reason_booked && takeover_reason)
        goto error;
    promise_handler_t* new_handler = malloc(sizeof(promise_handler_t));
    if(!new_handler)
        goto error;
    memset(new_handler,0,sizeof(promise_handler_t));
    new_handler->then = then;
    new_handler->then_ctx = then_ctx;
    new_handler->catch = catch;
    new_handler->catch_ctx = catch_ctx;
    new_handler->takeover_data = takeover_data;
    new_handler->takeover_reason = takeover_reason;
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
    promise->data_booked = promise->data_booked || takeover_data;
    promise->reason_booked = promise->reason_booked || takeover_reason;
    if(promise->resolved)
    {
        /** Promise is already resolved but not handled */
        promise_handler_t* handler = promise->first_handler;
        promise_handler_t* takeover_handler = NULL;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            if(handler->takeover_data)
                takeover_handler = handler;
            else
                handler->then(promise->resolve_data,handler->then_ctx,NULL,NULL);
            handler = next_handler;
        }
        if(takeover_handler)
        {
            promise->data_taken_over = true;
            takeover_handler->then(promise->resolve_data,takeover_handler->then_ctx,promise->free_data,promise->free_data_ctx);
        }
        /** free promise and remove from promises if it is still in promises */
        promise_t* old_promise = map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
        promise_free(old_promise);
    }
    else if(promise->rejected)
    {
        /** Promise is already rejected but not catched */
        promise_handler_t* handler = promise->first_handler;
        promise_handler_t* takeover_handler = NULL;
        while(handler)
        {
            promise_handler_t* next_handler = handler->next;
            if(handler->takeover_reason)
                takeover_handler = handler;
            else
                handler->catch(promise->reject_reason,handler->catch_ctx,NULL,NULL);
            handler = next_handler;
        }
        if(takeover_handler)
        {
            promise->reason_taken_over = true;
            takeover_handler->catch(promise->reject_reason,takeover_handler->catch_ctx,promise->free_reason,promise->free_reason_ctx);
        }
        /** free promise and remove from promises if it is still in promises */
        promise_t* old_promise = map_remove(manager->promises,&promise_handle,sizeof(promise_handle));
        promise_free(old_promise);
    }
    return 0;
error:
    return -1;
}

/** static functions */

static void promise_free(promise_t* promise)
{
    if(promise)
    {
        if(promise->free_data && (!promise->data_taken_over))
            promise->free_data(promise->resolve_data.ptr,promise->free_data_ctx);
        if(promise->free_reason && (!promise->reason_taken_over))
            promise->free_reason(promise->reject_reason.ptr,promise->free_reason_ctx);
        promise_handler_t* handler = promise->first_handler;
        while(handler)
        {
            promise_handler_t* next = handler->next;
            free(handler);
            handler = next;
        }
        if(promise->internal.free_data)
            promise->internal.free_data(promise->internal.data,promise->internal.free_ctx);
        free(promise);
    }
}

static void promise_free_with_ctx(void* data, void* ctx)
{
    promise_t* promise = (promise_t*)data;
    promise_free(promise);
}


/** promise group ****************************************/

typedef struct promise_group_sub_promise_ctx_s promise_group_sub_promise_ctx_t;
typedef struct promise_group_s promise_group_t;

struct promise_group_sub_promise_ctx_s
{
    int index;
    promise_handle_t promise;
    promise_group_t* group;
};

struct promise_group_s
{
    promise_manager_handle_t manager;
    promise_handle_t promise;
    int length;
    promise_group_sub_promise_ctx_t* sub_promises;
    promise_data_list_t* data_list;
    int data_count;
};

static void promise_group_free(promise_group_t* group);
static void promise_group_free_with_ctx(void* data, void* ctx);

static promise_group_t* promise_group_new(promise_manager_t* manager, int n, va_list args)
{
    promise_group_t* group = malloc(sizeof(promise_group_t));
    if(!group)
        goto error;
    memset(group,0,sizeof(promise_group_t));
    group->manager = manager;
    group->length = n;
    group->data_count = 0;  /** resolve/reject data count */
    group->data_list = malloc(sizeof(promise_data_list_t));
    if(!group)
        goto error;
    memset(group->data_list,0,sizeof(promise_data_list_t));
    group->data_list->length = n;
    group->data_list->items = malloc(sizeof(promise_data_list_item_t)*n);
    if(!group->data_list->items)
        goto error;
    memset(group->data_list->items,0,sizeof(promise_data_list_item_t)*n);
    for(int i=0;i<n;i++)
    {
        group->data_list->items[i].internal.free_ptr = NULL;
        group->data_list->items[i].internal.free_ctx = NULL;
    }
    group->sub_promises = malloc(sizeof(promise_group_sub_promise_ctx_t)*n);
    if(!group->sub_promises)
        goto error;
    memset(group->sub_promises,0,sizeof(promise_group_sub_promise_ctx_t)*n);

    for(int i=0;i<n;i++)
    {
        group->sub_promises[i].promise = va_arg(args,promise_handle_t);
        group->sub_promises[i].index = i;
        group->sub_promises[i].group = group;
    }

    group->promise = promise_new_internal(manager,group,promise_group_free_with_ctx,NULL);
    if(!group->promise)
        goto error;

    return group;
error:
    promise_group_free(group);
    return NULL;
}

/** promsie_group_t data list free */
static void promise_group_free_data_list(promise_data_list_t* list)
{
    if(list)
    {   
        if(list->items)
        {
            for(int i=0;i<list->length;i++)
            {
                if(list->items[i].internal.free_ptr)
                    list->items[i].internal.free_ptr(list->items[i].data.ptr,list->items[i].internal.free_ctx);
            }
            free(list->items);
        }
        free(list);
    }
}

static void promise_group_free_data_list_with_ctx(void* data, void* ctx)
{
    promise_group_free_data_list((promise_data_list_t*)data);
}

/** promise_group_t free */
static void promise_group_free(promise_group_t* group)
{
    if(group)
    {
        if((group->length != group->data_count) && group->data_list)   /** not all resolved/rejected, free data */
            promise_group_free_data_list(group->data_list);
        if(group->sub_promises)
        {
            for(int i=0;i<group->length;i++)    /** destroy remeaning sub promises */
                promise_destroy(group->manager,group->sub_promises[i].promise);
            free(group->sub_promises);
        }
        free(group);
    }
}

static void promise_group_free_with_ctx(void* data, void* ctx)
{
    promise_group_free((promise_group_t*)data);
}

/** promise.all ****************************************/

static void promise_all_sub_promise_then(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx);
static void promise_all_sub_promise_catch(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx);


promise_handle_t promise_all_v(promise_manager_handle_t manager, int n, va_list args)
{
    promise_group_t* all = promise_group_new(manager,n,args);
    if(!all)
        goto error;

    /** await all sub promises */
    for(int i=0;i<n;i++)
    {
        if(promise_await(
            manager,all->sub_promises[i].promise,
            promise_all_sub_promise_then,&(all->sub_promises[i]),true,
            promise_all_sub_promise_catch,&(all->sub_promises[i]),true)!=0)
        {
            goto error; 
        }  
    }

    return all->promise;
error:
    if(all)
        promise_destroy(manager,all->promise);
    return NULL;
}


promise_handle_t promise_all(promise_manager_handle_t manager, int n,...)
{
    promise_handle_t result = NULL;
    va_list args;
    va_start(args,n);
    result = promise_all_v(manager,n,args);
    va_end(args);
    return result;
}

static void promise_all_sub_promise_then(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_group_sub_promise_ctx_t* ctx = (promise_group_sub_promise_ctx_t*)user;
    ctx->group->data_count++;
    ctx->group->data_list->items[ctx->index].data = data;
    ctx->group->data_list->items[ctx->index].internal.free_ptr = free_ptr;
    ctx->group->data_list->items[ctx->index].internal.free_ctx = free_ctx;
    if(ctx->group->data_count == ctx->group->length)
    {
        /** all resolved */
        promise_data_t data_list = {.ptr = ctx->group->data_list};
        promise_resolve(ctx->group->manager,ctx->group->promise,data_list,promise_group_free_data_list_with_ctx,NULL);
    }
}

static void promise_all_sub_promise_catch(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_group_sub_promise_ctx_t* ctx = (promise_group_sub_promise_ctx_t*)user;
    /** rejct the all promise */
    promise_reject(ctx->group->manager,ctx->group->promise,data,free_ptr,free_ctx);
}


/** promise.any ****************************************/

static void promise_any_sub_promise_then(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx);
static void promise_any_sub_promise_catch(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx);


promise_handle_t promise_any_v(promise_manager_handle_t manager, int n, va_list args)
{
    promise_group_t* any = promise_group_new(manager,n,args);
    if(!any)
        goto error;

    /** await all sub promises */
    for(int i=0;i<n;i++)
    {
        if(promise_await(
            manager,any->sub_promises[i].promise,
            promise_any_sub_promise_then,&(any->sub_promises[i]),true,
            promise_any_sub_promise_catch,&(any->sub_promises[i]),true)!=0)
        {
            goto error; 
        }  
    }

    return any->promise;
error:
    if(any)
        promise_destroy(manager,any->promise);
    return NULL;
}


promise_handle_t promise_any(promise_manager_handle_t manager, int n,...)
{
    promise_handle_t result = NULL;
    va_list args;
    va_start(args,n);
    result = promise_any_v(manager,n,args);
    va_end(args);
    return result;
}

static void promise_any_sub_promise_then(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_group_sub_promise_ctx_t* ctx = (promise_group_sub_promise_ctx_t*)user;
    /** resolve the any promise */
    promise_resolve(ctx->group->manager,ctx->group->promise,data,free_ptr,free_ctx);
}

static void promise_any_sub_promise_catch(promise_data_t data, void* user, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_group_sub_promise_ctx_t* ctx = (promise_group_sub_promise_ctx_t*)user;
    ctx->group->data_count++;
    ctx->group->data_list->items[ctx->index].data = data;
    ctx->group->data_list->items[ctx->index].internal.free_ptr = free_ptr;
    ctx->group->data_list->items[ctx->index].internal.free_ctx = free_ctx;
    if(ctx->group->data_count == ctx->group->length)
    {
        /** all rejected */
        promise_data_t data_list = {.ptr = ctx->group->data_list};
        promise_reject(ctx->group->manager,ctx->group->promise,data_list,promise_group_free_data_list_with_ctx,NULL);
    }
}
