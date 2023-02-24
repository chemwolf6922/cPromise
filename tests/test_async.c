#include <stdio.h>
#include <assert.h>
#include "promise.h"
#include "async_function.h"

static promise_manager_handle_t manager = NULL;

void free_with_ctx(void* data , void* ctx)
{
    if(data)
        free(data);
}

promise_handle_t async_process1(int a, int b)
{
    promise_handle_t promise = promise_new(manager);
    int* result = malloc(sizeof(int));
    *result = a + b;
    promise_resolve(manager,promise,(promise_data_t){.ptr=result},free_with_ctx,NULL);
    return promise;
}

promise_handle_t async_process2(int a, int b)
{
    promise_handle_t promise = promise_new(manager);
    int* result = malloc(sizeof(int));
    *result = a - b;
    promise_resolve(manager,promise,(promise_data_t){.ptr=result},free_with_ctx,NULL);
    return promise;
}

promise_handle_t async_process3(int a, int b)
{
    promise_handle_t promise = promise_new(manager);
    int* code = malloc(sizeof(int));
    *code = -1;
    promise_reject(manager,promise,(promise_data_t){.ptr=code},free_with_ctx,NULL);
    return promise;
    // return NULL;
}

#define GLOBAL_PROMISE_MANAGER (manager)

ASYNC(test,(int a, int b),
    int a;int b;int* c;,
    ARG_INIT(a);
    ARG_INIT(b);)
{
    AWAIT_RESULT(ptr,VAR(c),async_process1(VAR(a),VAR(b)));
    *VAR(c) *= VAR(b);
    if(VAR(a)<VAR(b))
    {
        TRY
        {
            AWAIT(async_process3(VAR(a),*VAR(c)));
            SYNC_IN_TRY(
                *VAR(c)*=2;
                THROW(ptr,NULL,NULL,NULL);
            );
            AWAIT_RESULT(ptr,VAR(c),async_process2(VAR(a),VAR(b)));
        }
        CATCH(error)
        {
            printf("c: %d\n",*VAR(c));
            THROW(ptr,NULL,NULL,NULL);
        }
    }
    RETURN(number,*VAR(c),NULL,NULL);
    ASYNC_END();
}

static void test_then(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    printf("Result:%d\n",(int)data.number);
}

static void test_catch(promise_data_t reason, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    printf("Error\n");
}


int main(int argc, char const *argv[])
{
    manager = promise_manager_new();
    assert(manager);
    promise_await(manager,test(1,2),test_then,NULL,false,test_catch,NULL,false);
    promise_manager_free(manager);
    return 0;
}
