#ifndef __PROMISE_H
#define __PROMISE_H

#include "stdbool.h"

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

typedef void(*promise_then_handler_t)(const promise_data_t data, void* ctx);
typedef void(*promise_catch_handler_t)(const promise_data_t reason, void* ctx);
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
int promise_then_and_catch(
    promise_manager_handle_t manager, promise_handle_t promise, 
    promise_then_handler_t then, void* then_ctx, bool takeover_data,
    promise_catch_handler_t catch, void* catch_ctx, bool takeover_reason);
/**
 * @brief Create a new promise. Which:
 * will be resolved if all of the promises are resolved.
 * will be rejected if any of the promises is rejected. 
 * @attention Free this will not affect the promises associated with it.
 * 
 * @param manager 
 * @param n number of promises
 * @param ... promises 
 * @return promise_handle_t or NULL on error
 */
promise_handle_t promise_all(promise_manager_handle_t manager, int n,...);
/**
 * @brief Create a new promise. Which:
 * will be resolved if any of the promises is resolved.
 * will be rejected if all of the promises are rejected.
 * @attention Free this will not affect the promises associated with it.
 * 
 * @param manager 
 * @param n number of promises
 * @param ... promises
 * @return promise_handle_t or NULL on error
 */
promise_handle_t promise_any(promise_manager_handle_t manager, int n,...);


#endif

