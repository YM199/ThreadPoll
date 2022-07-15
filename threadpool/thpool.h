#ifndef __THPOOL_H__
#define __THPOOL_H__

#include <stdio.h>
#include <pthread.h>

#define err() fprintf(stderr, "%s %d", __FILE__, __LINE__)

/*线程*/
struct thread
{
    int id;             /*线程序号*/
    pthread_t pthread;  /*线程*/
    struct thpool_ *thpool_p;
};

/*线程池*/
struct thpool_
{
    struct thread **threads; /*线程数组*/
}; 


struct thpool_ *thpool_init(int num_threads);

#endif