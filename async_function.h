#ifndef __ASYNC_FUNCTION_H
#define __ASYNC_FUNCTION_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "promise.h"

typedef struct async_data_list_s
{
    struct async_data_list_s* next;
    void* ptr;
    void(*free_ptr)(void*, void*);
    void* free_ctx;
} async_data_list_t;

typedef struct async_ctx_s
{
    void* variables;
    /** internal */
    void(*func)(struct async_ctx_s*);
    int step;
    bool is_error;
    bool has_catch;
    promise_handle_t promise;
    promise_manager_handle_t manager;
    promise_data_t last_async_data;
    void(*last_async_data_free)(void*, void*);
    void* last_async_data_ctx;
    /** all async data taken over during the process with a free_ptr */
    async_data_list_t* async_data_list;
} async_ctx_t;

static void async_then(promise_data_t data, void* ctx, void(*free_ptr)(void*,void*), void* free_ctx)
{
    async_ctx_t* async_ctx = (async_ctx_t*)ctx;
    async_ctx->is_error = false;
    async_ctx->last_async_data = data;
    async_ctx->last_async_data_free = free_ptr;
    async_ctx->last_async_data_ctx = free_ctx;
    async_ctx->func(async_ctx);
}

static void async_catch(promise_data_t reason, void* ctx, void(*free_ptr)(void*,void*), void* free_ctx)
{
    async_ctx_t* async_ctx = (async_ctx_t*)ctx;
    async_ctx->is_error = true;
    async_ctx->last_async_data = reason;
    async_ctx->last_async_data_free = free_ptr;
    async_ctx->last_async_data_ctx = free_ctx;
    async_ctx->func(async_ctx);
}

static int async_push_async_data(async_ctx_t* ctx)
{
    if(ctx->last_async_data_free)
    {
        async_data_list_t* node = (async_data_list_t*)malloc(sizeof(async_data_list_t));
        if(!node)
            return -1;
        memset(node,0,sizeof(async_data_list_t));
        node->ptr = ctx->last_async_data.ptr;
        node->free_ptr = ctx->last_async_data_free;
        node->free_ctx = ctx->last_async_data_ctx;
        node->next = ctx->async_data_list;
        ctx->async_data_list = node;
    }
    return 0;
}

#define _UNIQUE_PROMISE_NAME2(x,y) x ## y
#define _UNIQUE_PROMISE_NAME(x,y) _UNIQUE_PROMISE_NAME2(x,y)
#define UNIQUE_PROMISE_NAME _UNIQUE_PROMISE_NAME(promise_,__LINE__)

/**
 * @brief Init an argument
 */
#define ARG_INIT(v) do{ variables->v = v; }while(0)

/**
 * @brief Use this to access the context data in an async function
 */
#define VAR(v) (variables_545bb8c->v)

/**
 * @brief Define an async function
 * 
 * @param name function name
 * @param params function parameters
 * @param var_list context data, NEEDS to include function parameters
 * @param arg_init_script copy function parameters into context data using ARG_INIT() macro
 * 
 * @return promise_handle_t
 */
#define ASYNC(name,params,var_list,arg_init_script) \
static void _##name (async_ctx_t* ctx); \
promise_handle_t name params\
{\
    async_ctx_t* ctx = malloc(sizeof(async_ctx_t));\
    if(!ctx) return NULL;\
    memset(ctx,0,sizeof(async_ctx_t));\
    promise_handle_t promise = promise_new(GLOBAL_PROMISE_MANAGER);\
    if(!promise)\
    {\
        free(ctx);\
        return NULL;\
    }\
    ctx->manager = GLOBAL_PROMISE_MANAGER;\
    ctx->promise = promise;\
    ctx->step = 0;\
    ctx->func = _##name;\
    struct\
    {\
        int dummy_545bb8c;\
        var_list\
    }* variables = malloc(sizeof(*variables));\
    if(!variables)\
    {\
        promise_destroy(ctx->manager,promise);\
        free(ctx);\
        return NULL;\
    }\
    memset(variables,0,sizeof(*variables));\
    arg_init_script\
    ctx->variables = variables;\
    _##name(ctx);\
    return promise;\
};\
static void _##name(async_ctx_t* ctx_545bb8c)\
{\
    struct\
    {\
        int dummy_545bb8c;\
        var_list\
    }* variables_545bb8c = ctx_545bb8c->variables;\
    switch(ctx_545bb8c->step)\
    {\
    case 0:

/**
 * @brief End an async function.
 */
#define ASYNC_END()\
    }}\
final:\
    while(ctx_545bb8c->async_data_list)\
    {\
        ctx_545bb8c->async_data_list->free_ptr(ctx_545bb8c->async_data_list->ptr,ctx_545bb8c->async_data_list->free_ctx);\
        async_data_list_t* next = ctx_545bb8c->async_data_list->next;\
        free(ctx_545bb8c->async_data_list);\
        ctx_545bb8c->async_data_list = next;\
    }\
    free(ctx_545bb8c->variables);\
    free(ctx_545bb8c);\
    return;

/**
 * @brief Resolve the promise and end the async function
 * 
 * @param type promise_data_t type, number, boolean or ptr
 * @param value return value
 * @param free_ptr free function for value if it is allocated
 * @param free_ctx free context for value
 */
#define RETURN(type,value,free_ptr,free_ctx)\
do{\
    promise_resolve(ctx_545bb8c->manager,ctx_545bb8c->promise,(promise_data_t){.type=value},free_ptr,free_ctx);\
    goto final;\
}while(0);

/**
 * @brief Throw an error insdie the async function. If it is not catched, the promise is rejected.
 * 
 * @param type promise_data_t type, number, boolean or ptr
 * @param value return value
 * @param free_ptr free function for value if it is allocated
 * @param free_ctx free context for value
 */
#define THROW(type,value,free_ptr,free_ctx)\
do{\
    if(ctx_545bb8c->has_catch)\
    {\
        ctx_545bb8c->is_error=true;\
        ctx_545bb8c->last_async_data=(promise_data_t){.type=value};\
        ctx_545bb8c->last_async_data_free=free_ptr;\
        ctx_545bb8c->last_async_data_ctx=free_ctx;\
    }\
    else\
    {\
        promise_reject(ctx_545bb8c->manager,ctx_545bb8c->promise,(promise_data_t){.type=value},free_ptr,free_ctx);\
        goto final;\
    }\
}while(0);


/**
 * @brief Start TRY {} CATCH(e) {} structure
 * @attention DO NOT nest TRY CATCH !!!
 */
#define TRY \
do{\
    ctx_545bb8c->has_catch=true;\
}while(0);

/**
 * @brief Complete TRY {} CATCH(e) {} structure
 * @param error name of the error, a promise_data_t
 */
#define CATCH(error)\
    for(promise_data_t error = ctx_545bb8c->last_async_data;\
    ctx_545bb8c->has_catch?\
    (((ctx_545bb8c->has_catch=false)||ctx_545bb8c->is_error)?\
    (((ctx_545bb8c->is_error=false),async_push_async_data(ctx_545bb8c))||true):\
    false):\
    false;\
    )

/**
 * @brief Run sync code in try catch
 * @attention MUST use this for try catch to work properly
 * 
 * @param expr the sync code to run.
 */
#define SYNC_IN_TRY(expr) \
do{\
    if(!ctx_545bb8c->is_error)\
    {\
        expr;\
    }\
}while(0);


/**
 * @brief Await a promise
 * 
 * @param expr the code to generate a promise
 */
#define AWAIT(expr)\
do{\
    if(!ctx_545bb8c->is_error)\
    {\
        ctx_545bb8c->step = __LINE__;\
        if(promise_await(ctx_545bb8c->manager,expr,async_then,ctx_545bb8c,false,async_catch,ctx_545bb8c,true)!=0)\
        {\
            ctx_545bb8c->is_error=true;\
            ctx_545bb8c->last_async_data=(promise_data_t){.ptr=NULL};\
            ctx_545bb8c->last_async_data_free=NULL;\
            ctx_545bb8c->last_async_data_ctx=NULL;\
            ctx_545bb8c->func(ctx_545bb8c);\
        }\
        return;\
    case __LINE__:\
        if(ctx_545bb8c->is_error && (!ctx_545bb8c->has_catch))\
        {\
            promise_reject(ctx_545bb8c->manager, ctx_545bb8c->promise, ctx_545bb8c->last_async_data, ctx_545bb8c->last_async_data_free, ctx_545bb8c->last_async_data_ctx);\
            goto final;\
        }\
    }\
}while(0);

/**
 * @brief Await a promise and assign the result to dst
 * 
 * @param type type of data to copy 
 * @param dst double, booelan or pointer
 * @param expr the code to generate a promise
 */
#define AWAIT_RESULT(type,dst,expr)\
do{\
    if(!ctx_545bb8c->is_error)\
    {\
        ctx_545bb8c->step = __LINE__;\
        if(promise_await(ctx_545bb8c->manager,expr,async_then,ctx_545bb8c,true,async_catch,ctx_545bb8c,true)!=0)\
        {\
            ctx_545bb8c->is_error=true;\
            ctx_545bb8c->last_async_data=(promise_data_t){.ptr=NULL};\
            ctx_545bb8c->last_async_data_free=NULL;\
            ctx_545bb8c->last_async_data_ctx=NULL;\
            ctx_545bb8c->func(ctx_545bb8c);\
        }\
        return;\
    case __LINE__:\
        if(!ctx_545bb8c->is_error)\
        {\
            dst = ctx_545bb8c->last_async_data.type;\
            async_push_async_data(ctx_545bb8c);\
        }\
        else if(!ctx_545bb8c->has_catch)\
        {\
            promise_reject(ctx_545bb8c->manager, ctx_545bb8c->promise, ctx_545bb8c->last_async_data, ctx_545bb8c->last_async_data_free, ctx_545bb8c->last_async_data_ctx);\
            goto final;\
        }\
    }\
}while(0);



#endif
