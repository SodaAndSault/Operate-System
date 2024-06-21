#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H

#include "../thread/sync.h"
#include "../thread/thread.h"
#define bufsize 64

//循环队列，实现环形缓冲区
//生产者消费者问题
struct ioqueue{
    struct lock lock;
    struct task_struct* consumer;
    struct task_struct* producer;
    char buf[bufsize];
    int32_t head;
    int32_t tail;
};
void ioqueue_init(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);

#endif