#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "promise.h"

static void test_then(promise_data_t data, void* ctx)
{
    printf("%d\n",(int)data.number);
}

static void test_catch(promise_data_t reason, void* ctx)
{
    printf("Error: %s\n",(char*)reason.ptr);
}

static void free_with_ctx(void* data, void* ctx)
{
    if(data)
        free(data);
}

int main(int argc, char const *argv[])
{
    promise_manager_handle_t manager = promise_manager_new();

    promise_handle_t promise = promise_new(manager);

    promise_then_and_catch(manager,promise,test_then,NULL,false,test_catch,NULL,false);

    promise_data_t data;
    data.number = 100;
    promise_resolve(manager,promise,data,NULL,NULL);

    promise_data_t reason;
    reason.ptr = strdup("Test reason");
    if(promise_reject(manager,promise,reason,free_with_ctx,NULL)!=0)
        free(reason.ptr);

    promise_manager_free(manager);
    /* code */
    return 0;
}
