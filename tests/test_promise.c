#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "promise.h"

static promise_manager_handle_t manager = NULL;

static void test_then(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    printf("Data: %s\n",(char*)data.ptr);
    if(free_ptr)
        free_ptr(data.ptr,free_ctx);
}

static void test_then_all(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_data_list_t* list = data.ptr;
    for(int i=0;i<list->length;i++)
    {
        printf("Data#%d:%s\n",i,(char*)list->items[i].data.ptr);
    }
    if(free_ptr)
        free_ptr(data.ptr,free_ctx);
}

static void test_then_all3(promise_data_t data, void* ctx, void(*free_ptr)(void*, void*), void* free_ctx)
{
    promise_data_list_t* layer1 = data.ptr;
    for(int i=0;i<layer1->length;i++)
    {
        promise_data_list_t* layer2 = layer1->items[i].data.ptr;
        for(int j=0;j<layer2->length;j++)
        {
            promise_data_list_t* layer3 = layer2->items[j].data.ptr;
            for(int k=0;k<layer3->length;k++)
            {
                printf("Data#%d-%d-%d:%s\n",i,j,k,(char*)layer3->items[k].data.ptr);
            }
        }
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
    for(int i=0;i<list->length;i++)
    {
        printf("Error#%d:%s\n",i,(char*)list->items[i].data.ptr);
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
    manager = promise_manager_new();

    promise_handle_t promise1 = promise_new(manager);
    promise_handle_t promise2 = promise_new(manager);
    promise_handle_t promise3 = promise_new(manager);
    promise_handle_t promise4 = promise_new(manager);
    promise_handle_t promise5 = promise_new(manager);
    promise_handle_t promise6 = promise_new(manager);
    promise_handle_t promise7 = promise_new(manager);
    promise_handle_t promise8 = promise_new(manager);

    promise_handle_t promise2_1 = promise_all(manager,2,promise1,promise2);
    promise_handle_t promise2_2 = promise_all(manager,2,promise3,promise4);
    promise_handle_t promise2_3 = promise_all(manager,2,promise5,promise6);
    promise_handle_t promise2_4 = promise_all(manager,2,promise7,promise8);

    promise_handle_t promise3_1 = promise_all(manager,2,promise2_1,promise2_2);
    promise_handle_t promise3_2 = promise_all(manager,2,promise2_3,promise2_4);

    promise_handle_t promise4_1 = promise_all(manager,2,promise3_1,promise3_2);

    promise_await(manager,promise4_1,test_then_all3,NULL,true,test_catch,NULL,true);

    promise_data_t data1 = {.ptr = strdup("data1")};
    if(promise_resolve(manager,promise1,data1,free_with_ctx,NULL)!=0)
        free(data1.ptr);

    promise_data_t data2 = {.ptr = strdup("data2")};
    if(promise_resolve(manager,promise2,data2,free_with_ctx,NULL)!=0)
        free(data2.ptr);

    promise_data_t data3 = {.ptr = strdup("data3")};
    if(promise_resolve(manager,promise3,data3,free_with_ctx,NULL)!=0)
        free(data3.ptr);

    promise_data_t data4 = {.ptr = strdup("data4")};
    if(promise_resolve(manager,promise4,data4,free_with_ctx,NULL)!=0)
        free(data4.ptr);

    promise_data_t data5 = {.ptr = strdup("data5")};
    if(promise_resolve(manager,promise5,data5,free_with_ctx,NULL)!=0)
        free(data5.ptr);

    promise_data_t data6 = {.ptr = strdup("data6")};
    if(promise_resolve(manager,promise6,data6,free_with_ctx,NULL)!=0)
        free(data6.ptr);

    promise_data_t data7 = {.ptr = strdup("data7")};
    if(promise_resolve(manager,promise7,data7,free_with_ctx,NULL)!=0)
        free(data7.ptr);

    promise_data_t data8 = {.ptr = strdup("data8")};
    if(promise_resolve(manager,promise8,data8,free_with_ctx,NULL)!=0)
        free(data8.ptr);
    

    promise_manager_free(manager);
    manager = NULL;
    /* code */
    return 0;
}
