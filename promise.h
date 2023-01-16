#ifndef __PROMISE_H
#define __PROMISE_H

#include <stdbool.h>
#include <stdarg.h>

typedef void* promise_manager_handle_t;

typedef void* promise_handle_t;

promise_manager_handle_t promise_manager_new();
void promise_manager_free(promise_manager_handle_t manager);

/**
 * @brief Create a new promise
 * 
 * @param manager 
 * @return promise_handle_t 
 */
promise_handle_t promise_new(promise_manager_handle_t manager);

/**
 * @brief Destroy a promise. The promise can be at any state.
 * Neither then nor catch will be called after this.
 * If the proimse is resovled or rejected and the free_data function is set.
 * The data will be freed to prevent any untracked data.
 * 
 * @param manager 
 * @param promise 
 */
void promise_destroy(promise_manager_handle_t manager, promise_handle_t promise);

typedef union
{
    void* ptr;
    double number;
    bool boolean;
} promise_data_t;

/**
 * @brief Resolve a promise. A promise can only be resolved or rejected once.
 * 
 * @param manager 
 * @param promise 
 * @param data User of this should know the type of it.
 * @param free_data Called when the promise is destroyed or is handled w/o data taken over. 
 * @param ctx ctx for free_data
 * @return int 0 on success, 1 on error
 */
int promise_resolve(
    promise_manager_handle_t manager, promise_handle_t promise, 
    promise_data_t data, void(*free_data)(void*,void*), void* ctx);
/**
 * @brief Reject a promise. A promise can only be resolved or rejected once.
 * 
 * @param manager 
 * @param promise 
 * @param reason User of this should know the type of it.
 * @param free_reason Called when the promise is destroed or is handled w/o data taken over.
 * @param ctx ctx for free_reason
 * @return int 0 on success, 1 on error
 */
int promise_reject(
    promise_manager_handle_t manager, promise_handle_t promise, 
    promise_data_t reason, void(*free_reason)(void*,void*), void* ctx);

typedef void(*promise_then_handler_t)(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx);
typedef void(*promise_catch_handler_t)(promise_data_t reason, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx);
/**
 * @brief Assign then and catch handler. 
 * The promise will be freed after all the handlers assigned to it is called. 
 * The then handler will be called under the following conditions:
 * 1. If the promise is resolved later.
 * 2. If the promise is already resolved but not assigned with a then handler.
 * The catch handler will be called under the following conditions:
 * 1. If the promise is rejected later.
 * 2. If the promise is already rejected but not assigned with a catch handler.
 * 
 * @param manager 
 * @param promise 
 * @param then not nullable
 * @param then_ctx
 * @param takeover_data If this is set. The data will not be freed when the promise is freed
 * @param catch not nullable
 * @param catch_ctx 
 * @param takeover_reason If this is set. The reason will not be freed when the promise is freed
 * @return int 0 on success, 1 on error
 */
int promise_await(
    promise_manager_handle_t manager, promise_handle_t promise, 
    promise_then_handler_t then, void* then_ctx, bool takeover_data,
    promise_catch_handler_t catch, void* catch_ctx, bool takeover_reason);

typedef struct
{
    promise_data_t data;
    struct
    {
        void(*free_ptr)(void*,void*);
        void* free_ctx;
    } internal;
} promise_data_list_item_t;

typedef struct
{
    int length;
    promise_data_list_item_t* items;
} promise_data_list_t;
/**
 * @brief Create a new promise. Which:
 * will be resolved if all of the promises are resolved.
 * will be rejected if any of the promises is rejected. 
 * The resolved value is a list of all the sub promises' resolve values.
 * The rejected reason is the first rejected sub promise's reject reason.
 * @attention Sub promises are strongly linked to this promise. DO NOT use them for other purposes.
 * @attention Sub promises' data are taken over by default.
 * @attention User MUST handle the data list content
 * @attention User MUST NOT use the data list outside and free_data of then
 * 
 * @param manager 
 * @param n number of promises
 * @param ... promises 
 * @return promise_handle_t or NULL on error
 */
promise_handle_t promise_all(promise_manager_handle_t manager, int n,...);
promise_handle_t promise_all_v(promise_manager_handle_t manager, int n, va_list args);

/**
 * @brief Create a new promise. Which:
 * will be resolved if any of the promises is resolved.
 * will be rejected if all of the promises are rejected.
 * The resolved value is the first resovled sub promise's resolve value
 * The rejected reason is a list of all the sub promises' reject reasons
 * @attention Sub promises are strongly linked to this promise. DO NOT use them for other purposes.
 * @attention Sub promises' reject reason are taken over by default
 * @attention Sub promises MUST have their free_data set if it is allocated.
 * 
 * @param manager 
 * @param n number of promises
 * @param ... promises
 * @return promise_handle_t or NULL on error
 */
promise_handle_t promise_any(promise_manager_handle_t manager, int n,...);
promise_handle_t promise_any_v(promise_manager_handle_t manager, int n, va_list args);

#endif

