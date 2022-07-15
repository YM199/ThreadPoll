#include "thpool.h"
#include <unistd.h>
#include <stdlib.h>

static void *thread_do(struct thread* thread_p)
{
    printf("1234\n");
    return NULL;
}

/**
 * @brief 初始化线程池里面的线程
 * 
 * @param thpool_p 线程池结构体
 * @param thread_p 线程结构体指针
 * @param id 线程的序号
 * @return int 
 */
static int thread_init(struct thpool_ *thpool_p, struct thread **thread_p, int id)
{
    /* *thread_p代表结构体 */
    *thread_p = (struct thread *)malloc(sizeof(struct thread));
    if((*thread_p) == NULL)
    {
        err();
        return -1;
    }

    (*thread_p)->thpool_p = thpool_p;
    (*thread_p)->id = id;

    pthread_create(&(*thread_p)->pthread, NULL, (void *(*)(void *))thread_do, NULL); /*创建线程*/
    pthread_detach((*thread_p)->pthread); /*分离线程*/

    return 0;
}

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

    thpool_p->threads = (struct thread **)malloc(sizeof(struct thread *) * num_threads);
    if (thpool_p->threads == NULL)
    {
        err();
        return NULL;
    }

    for (int i = 0; i < num_threads; i++)
    {
        thread_init(thpool_p, &thpool_p->threads[i], i);
    }

    return NULL;
}
