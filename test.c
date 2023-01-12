#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "promise.h"

static void test_then(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    printf("Data: %s\n",(char*)data.ptr);
    if(free_ptr)
        free_ptr(data.ptr,free_ctx);
}

static void test_then_all(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_data_list_t* list = data.ptr;
    for(int i=0;i<list->n;i++)
    {
        printf("Data#%d:%s\n",i,(char*)list->data[i].ptr);
    }
    if(free_ptr)
        free_ptr(data.ptr,free_ctx);
}

static void test_catch(promise_data_t reason, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    printf("Error: %s\n",(char*)reason.ptr);
    if(free_ptr)
        free_ptr(reason.ptr,free_ctx);
}

static void test_catch_any(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_data_list_t* list = data.ptr;
    for(int i=0;i<list->n;i++)
    {
        printf("Error#%d:%s\n",i,(char*)list->data[i].ptr);
    }
    if(free_ptr)
        free_ptr(data.ptr,free_ctx);
}


static void free_with_ctx(void* data, void* ctx)
{
    if(data)
        free(data);
}

int main(int argc, char const *argv[])
{
    promise_manager_handle_t manager = promise_manager_new();

    promise_handle_t promise1 = promise_new(manager);
    promise_handle_t promise2 = promise_new(manager);
    promise_handle_t promise3 = promise_any(manager,2,promise1,promise2);

    promise_then_and_catch(manager,promise3,test_then,NULL,true,test_catch_any,NULL,false);

    promise_data_t data1 = {.ptr = strdup("data1")};
    if(promise_resolve(manager,promise1,data1,free_with_ctx,NULL)!=0)
        free(data1.ptr);

    promise_data_t data2 = {.ptr = strdup("data2")};
    if(promise_reject(manager,promise2,data2,free_with_ctx,NULL)!=0)
        free(data2.ptr);

    promise_manager_free(manager);
    /* code */
    return 0;
}
