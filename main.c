#include <stdio.h>
#include "./threadpool/thpool.h"

int main(void)
{
    thpool_init(4);
    return 0;
}