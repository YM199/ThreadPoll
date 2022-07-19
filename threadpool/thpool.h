#ifndef __THPOOL_H__
#define __THPOOL_H__

#include <stdio.h>
#include <pthread.h>

#define err() fprintf(stderr, "%s %d", __FILE__, __LINE__)

/*二值信号量，确保工作队列有工作*/
struct bsem
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
};

/*工作结构体*/
struct job
{
    struct job *prev;            /*队列上一个节点*/
    void (*function)(void *arg); /*工作的函数*/
    void *arg;                   /*函数的参数*/
};

/*工作队列结构体*/
struct jobqueue
{
    pthread_mutex_t rwmutex;
    struct job *front;     /*指向队列头指针*/
    struct job *rear;      /*指向队列尾指针*/
    struct bsem *has_jobs; /*二值信号量 1代表有工作 0代表无工作*/
    int len;               /*队列的长度*/
};

/*线程结构体*/
struct thread
{
    int id;            /*线程序号*/
    pthread_t pthread; /*线程*/
    struct thpool_ *thpool_p;
};

/*线程池结构体*/
struct thpool_
{
    struct thread **threads;          /*线程数组*/
    volatile int num_threads_alive;   /*活着的线程数量*/
    volatile int num_threads_working; /*工作中的线程数量*/
    pthread_mutex_t thcount_lock;     /*互斥锁*/
    pthread_cond_t threads_all_idle;  /*条件变量*/
    struct jobqueue jobqueue;         /*工作队列*/
};

struct thpool_ *thpool_init(int num_threads);
int thpool_add_work(struct thpool_ *thpool_p, void (*function_p)(void *), void *arg_p);
void thpool_wait(struct thpool_ * thpool_p);
void thpool_destroy(struct thpool_ *thpool_p);

#endif