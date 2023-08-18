
#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    
    int rc;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // wait
    usleep(thread_func_args->wait_obtain_data * 1000);
    
    // obtain mutex
    rc = pthread_mutex_lock(thread_func_args->mutex_data);
    if (rc != 0){
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    // else
    // wait
    usleep(thread_func_args->wait_release_data * 1000);

    // release mutex
    rc = pthread_mutex_unlock(thread_func_args->mutex_data);
    if (rc != 0){
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;
    return thread_param;

}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
    **/
    int ret = -1;
    bool result = false;

    // alocated memory for thread data
    struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
    if(data == NULL){
        ERROR_LOG("Can not allocate thread data.");
        return false;
    }

    // setup mutex and wait arguments
    data->wait_obtain_data = wait_to_obtain_ms;
    data->wait_release_data = wait_to_release_ms;
    data->mutex_data = mutex;
    data->thread_complete_success = false;

    // pass thread_data to created thread
    ret = pthread_create(thread, NULL, threadfunc, data);
    
    // free if create unsuccessfully
    if(ret != 0){
        ERROR_LOG("Can not create thread.");
        result = data->thread_complete_success;
        free(data);
        return result;
    }

    return true;
}

