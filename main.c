#include <stdio.h>
#include <stdint.h>
#include "./threadpool/thpool.h"

void task(void *arg)
{
    printf("Thread #%u working on %d\n", (int)pthread_self(), (int)arg);
}

int main(void)
{
    struct thpool_ *thpool = thpool_init(4);

    for (int i = 0; i < 40; i++)
    {
        thpool_add_work(thpool, task, (void *)(uintptr_t)i);
    }

    while(1);

    return 0;
}