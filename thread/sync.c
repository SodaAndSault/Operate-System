#include "sync.h"
#include "list.h"
#include "interrupt.h"
#include "thread.h"
#include "debug.h"
#include "global.h"
// //初始化信号量
void sema_init(struct semaphore* psema, uint8_t value) {
   psema->value = value;       // 为信号量赋初值
   list_init(&psema->waiters); //初始化信号量的等待队列
}
//初始化锁plock
void lock_init(struct lock* plock){
    plock->holder=NULL;
    plock->holder_repeat_nr=0;
    sema_init(&plock->semaphore,1);//信号量初值为1
}


//信号量down操作
void sema_down(struct semaphore* psema){
    enum intr_status old_status = intr_disable();//关中断保持原子操作
    while(psema->value==0){
        //资源被别人持有，且当前进程不在waiter阻塞队列
        ASSERT(!elem_find(&psema->waiters,&running_thread()->general_tag));
        if(elem_find(&psema->waiters,&running_thread()->general_tag))
            PANIC("sema_down:thread blocked has been in waiters_list\n");
        //若当前信号量为0，则当前线程把自己加入该锁的等待队列，然后阻塞自己
        list_append(&psema->waiters,&running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    psema->value--;
    ASSERT(psema->value==0);
    intr_set_status(old_status);
}



//信号量up操作
void sema_up(struct semaphore* psema){
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value==0);
    if(!list_empty(&psema->waiters)){
        struct task_struct* thread_blocked = elem2entry(struct task_struct,general_tag,list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value==1);
    intr_set_status(old_status);
}
//获取锁
void lock_acpuire(struct lock* plock){
    //排除自己曾经申请锁但是未释放的情况
    if(plock->holder!=running_thread()){
        sema_down(&plock->semaphore);//对信号量p操作
        plock->holder=running_thread();
        ASSERT(plock->holder_repeat_nr==0);
        plock->holder_repeat_nr=1;
    }else{
        plock->holder_repeat_nr++;
    }
}
//释放锁
void lock_release(struct lock* plock){
    ASSERT(plock->holder==running_thread());
    if(plock->holder_repeat_nr > 1){
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr==1);
    plock->holder=NULL;
    plock->holder_repeat_nr=0;
    sema_up(&plock->semaphore);
}

