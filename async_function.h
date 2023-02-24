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


#define ARG_INIT(v) variables->v = v

#define VAR(v) (variables->v)

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
        int dummy;\
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
static void _##name(async_ctx_t* ctx)\
{\
    struct\
    {\
        int dummy;\
        var_list\
    }* variables = ctx->variables;\
    switch(ctx->step)\
    {\
    case 0:


#define ASYNC_END()\
    }}\
final:\
    while(ctx->async_data_list)\
    {\
        ctx->async_data_list->free_ptr(ctx->async_data_list->ptr,ctx->async_data_list->free_ctx);\
        async_data_list_t* next = ctx->async_data_list->next;\
        free(ctx->async_data_list);\
        ctx->async_data_list = next;\
    }\
    free(ctx->variables);\
    free(ctx);\
    return;


#define RETURN(type,value,free_ptr,free_ctx)\
do{\
    promise_resolve(ctx->manager,ctx->promise,(promise_data_t){.type=value},free_ptr,free_ctx);\
    goto final;\
}while(0);


#define THROW(type,value,free_ptr,free_ctx)\
do{\
    if(ctx->has_catch)\
    {\
        ctx->is_error=true;\
        ctx->last_async_data=(promise_data_t){.type=value};\
        ctx->last_async_data_free=free_ptr;\
        ctx->last_async_data_ctx=free_ctx;\
    }\
    else\
    {\
        promise_reject(ctx->manager,ctx->promise,(promise_data_t){.type=value},free_ptr,free_ctx);\
        goto final;\
    }\
}while(0);


/** DO NOT nest TRY CATCH !!! */
#define TRY \
do{\
    ctx->has_catch=true;\
}while(0);


#define CATCH(error)\
    for(promise_data_t error = ctx->last_async_data;\
    ctx->has_catch?(((ctx->has_catch=false)||ctx->is_error)?(((ctx->is_error=false),async_push_async_data(ctx))||true):false):false;\
    )


#define SYNC_IN_TRY(expr) \
do{\
    if(!ctx->is_error)\
    {\
        expr;\
    }\
}while(0);


#define AWAIT(expr)\
do{\
    if(!ctx->is_error)\
    {\
        ctx->step = __LINE__;\
        promise_await(ctx->manager,expr,async_then,ctx,false,async_catch,ctx,true);\
        return;\
    case __LINE__:\
        if(ctx->is_error && (!ctx->has_catch))\
        {\
            promise_reject(ctx->manager, ctx->promise, ctx->last_async_data, ctx->last_async_data_free, ctx->last_async_data_ctx);\
            goto final;\
        }\
    }\
}while(0);


#define AWAIT_RESULT(type,dst,expr)\
do{\
    if(!ctx->is_error)\
    {\
        ctx->step = __LINE__;\
        promise_await(ctx->manager,expr,async_then,ctx,true,async_catch,ctx,true);\
        return;\
    case __LINE__:\
        if(!ctx->is_error)\
        {\
            dst = ctx->last_async_data.type;\
            async_push_async_data(ctx);\
        }\
        else if(!ctx->has_catch)\
        {\
            promise_reject(ctx->manager, ctx->promise, ctx->last_async_data, ctx->last_async_data_free, ctx->last_async_data_ctx);\
            goto final;\
        }\
    }\
}while(0);



#endif
