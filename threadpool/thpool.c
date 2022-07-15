#include "thpool.h"
#include <unistd.h>
#include <stdlib.h>

static volatile int threads_keepalive = 1; /*让线程一直活着*/

static void *thread_do(struct thread *thread_p);
static int thread_init(struct thpool_ *thpool_p, struct thread **thread_p, int id);

static int jobqueue_init(struct jobqueue *jobqueue_p);
static struct job *jobqueue_pull(struct jobqueue *jobqueue_p);
static void jobqueue_push(struct jobqueue *jobqueue_p, struct job *newjob);
/**
 * @brief 线程池初始化
 *
 * @param num_threads 线程的数量
 * @return struct thpool_*
 */
struct thpool_ *thpool_init(int num_threads)
{
    if (num_threads < 0)
        num_threads = 0;

    struct thpool_ *thpool_p;
    thpool_p = (struct thpool_ *)malloc(sizeof(struct thpool_));
    if (thpool_p == NULL)
    {
        err();
        return NULL;
    }

    thpool_p->num_threads_alive = 0;
    thpool_p->num_threads_working = 0;

    if (jobqueue_init(&thpool_p->jobqueue) == -1)
    {
        err();
        return NULL;
    }

    thpool_p->threads = (struct thread **)malloc(sizeof(struct thread *) * num_threads);
    if (thpool_p->threads == NULL)
    {
        err();
        return NULL;
    }

    pthread_mutex_init(&(thpool_p->thcount_lock), NULL);
    pthread_cond_init(&(thpool_p->threads_all_idle), NULL);

    for (int i = 0; i < num_threads; i++)
    {
        thread_init(thpool_p, &thpool_p->threads[i], i);
    }

    /*等待线程初始化完毕*/
    while (thpool_p->num_threads_alive != num_threads)
        ;

    return thpool_p;
}

/**
 * @brief  添加工作到线程池
 *
 * @param thpool_p 线程池结构体
 * @param function_p 工作的函数
 * @param arg_p 函数参数
 * @return int
 */
int thpool_add_work(struct thpool_ *thpool_p, void (*function_p)(void *), void *arg_p)
{
    struct job *newjob;
    newjob = (struct job *)malloc(sizeof(struct job));
    if (newjob == NULL)
    {
        err();
        return -1;
    }

    newjob->function = function_p;
    newjob->arg = arg_p;

    jobqueue_push(&thpool_p->jobqueue, newjob);

    return 0;
}

/**
 * @brief 执行工作函数
 *
 * @param thread_p 线程结构体指针
 * @return void*
 */
static void *thread_do(struct thread *thread_p)
{
    struct thpool_ *thpool_p = thread_p->thpool_p;

    pthread_mutex_lock(&thpool_p->thcount_lock);
    thpool_p->num_threads_alive--;
    pthread_mutex_unlock(&thpool_p->thcount_lock);

    while (threads_keepalive)
    {
        pthread_mutex_lock(&thpool_p->thcount_lock);
        thpool_p->num_threads_working++;
        pthread_mutex_unlock(&thpool_p->thcount_lock);

        void (*func_buff)(void *); /*定义函数指针*/
        void *arg_buff;            /*函数的参数*/

        struct job *job_p = jobqueue_pull(&thpool_p->jobqueue);
        if (job_p)
        {
            func_buff = job_p->function;
            arg_buff = job_p->arg;
            func_buff(arg_buff); /*执行工作函数*/
            free(job_p);
        }

        pthread_mutex_lock(&thpool_p->thcount_lock);
        thpool_p->num_threads_working--;
        if (!thpool_p->num_threads_working)
        {
            pthread_cond_signal(&thpool_p->threads_all_idle);
        }
        pthread_mutex_unlock(&thpool_p->thcount_lock);
    }

    pthread_mutex_lock(&thpool_p->thcount_lock);
    thpool_p->num_threads_alive--;
    pthread_mutex_unlock(&thpool_p->thcount_lock);

    return NULL;
}

/**
 * @brief 初始化线程池里面的线程
 *
 * @param thpool_p 线程池结构体指针
 * @param thread_p 线程结构体指针
 * @param id 线程的序号
 * @return int
 */
static int thread_init(struct thpool_ *thpool_p, struct thread **thread_p, int id)
{
    /* *thread_p代表结构体 */
    *thread_p = (struct thread *)malloc(sizeof(struct thread));
    if ((*thread_p) == NULL)
    {
        err();
        return -1;
    }

    (*thread_p)->thpool_p = thpool_p;
    (*thread_p)->id = id;

    pthread_create(&(*thread_p)->pthread, NULL, (void *(*)(void *))thread_do, *thread_p); /*创建线程*/
    pthread_detach((*thread_p)->pthread);                                                 /*分离线程*/

    return 0;
}

/**
 * @brief 初始化工作队列
 *
 * @param jobqueue_p 工作队列
 * @return int
 */
static int jobqueue_init(struct jobqueue *jobqueue_p)
{
    jobqueue_p->len = 0;
    jobqueue_p->front = NULL;
    jobqueue_p->rear = NULL;

    pthread_mutex_init(&(jobqueue_p->rwmutex), NULL);

    return 0;
}

/**
 * @brief 添加工作到工作队列
 *
 * @param jobqueue_p 工作队列结构体指针
 * @param newjob 工作结构体指针
 */
static void jobqueue_push(struct jobqueue *jobqueue_p, struct job *newjob)
{
    pthread_mutex_lock(&jobqueue_p->rwmutex);

    newjob->prev = NULL;

    switch (jobqueue_p->len)
    {
    case 0: /*工作队列为空*/
        jobqueue_p->front = newjob;
        jobqueue_p->rear = newjob;
        break;
    default:
        jobqueue_p->rear->prev = newjob;
        jobqueue_p->rear = newjob;
        break;
    }

    jobqueue_p->len++;

    pthread_mutex_unlock(&jobqueue_p->rwmutex);
}

/**
 * @brief 从工作队列中拿出一个工作
 *
 * @param jobqueue_p 工作队列结构体
 * @return struct job* 工作结构体
 */
static struct job *jobqueue_pull(struct jobqueue *jobqueue_p)
{
    pthread_mutex_lock(&jobqueue_p->rwmutex);
    struct job *job_p = jobqueue_p->front;
    switch (jobqueue_p->len)
    {
    case 0:
        break;

    case 1:
        jobqueue_p->front = NULL;
        jobqueue_p->rear = NULL;
        jobqueue_p->len = 0;
        break;

    default:
        jobqueue_p->front = job_p->prev;
        jobqueue_p->len--;
        break;
    }
    pthread_mutex_unlock(&jobqueue_p->rwmutex);

    return job_p;
}