#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#include "list.h"
#include "interrupt.h"
#include "debug.h"
#include "print.h"
#define PG_SIZE 4096

struct task_struct* main_thread;//主线程
struct list thread_ready_list; //就绪态队列
struct list thread_all_list;//所有任务队列
static struct list_elem* thread_tag;//用于保存队列中的线程节点

extern void switch_to(struct task_struct* cur, struct task_struct* next); // 外部汇编函数

//获取当前线程PCB指针
struct task_struct* running_thread(){
    uint32_t esp;
    asm("mov %%esp,%0":"=g"(esp));
    return (struct task_struct*)(esp & 0xfffff000);
}

static void kernel_thread(thread_func* function, void* func_arg){
    intr_enable();
    function(func_arg);
}

void init_thread(struct task_struct* pthread,char* name,int prio){
    memset(pthread,0,sizeof(*pthread));
    strcpy(pthread->name,name);
    if(pthread==main_thread) pthread->status = TASK_RUNNING;
    else pthread->status = TASK_READY;
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL; //线程没有自己的虚拟空间，指向NULL
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread+PG_SIZE);
    pthread->stack_magic = 0x19981011;
}
void thread_create(struct task_struct* pthread, thread_func function,void* func_arg){
    pthread->self_kstack = (uint32_t*)((int)(pthread->self_kstack) - sizeof(struct intr_stack));
    pthread->self_kstack = (uint32_t*)((int)(pthread->self_kstack) - sizeof(struct thread_stack));
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->func_arg = func_arg;
    kthread_stack->function = function;
    kthread_stack->ebp=kthread_stack->ebx=kthread_stack->edi=kthread_stack->esi = 0;
}

struct task_struct* thread_start(char* name,int prio,thread_func function,void* func_arg){
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread,name,prio);
    thread_create(thread,function,func_arg);
    //确保之前不在队列中
    ASSERT(!elem_find(&thread_ready_list,&thread->general_tag));
    list_append(&thread_ready_list,&thread->general_tag);
    ASSERT(!elem_find(&thread_all_list,&thread->all_list_tag));
    list_append(&thread_all_list,&thread->all_list_tag);
    // asm volatile ("movl %0,%%esp;pop %%ebp; pop %%ebx;pop %%edi;pop %%esi;ret": :"g"(thread->self_kstack): "memory");
    return thread;
}
static void make_main_thread(void){
    main_thread = running_thread();
    init_thread(main_thread,"main",31);
    ASSERT(!elem_find(&thread_all_list,&main_thread->all_list_tag));
    list_append(&thread_all_list,&main_thread->all_list_tag);
}
void schedule(){
    ASSERT(intr_get_status()== INTR_OFF);
    struct task_struct* cur = running_thread();
    if(cur->status==TASK_RUNNING){
    //若此线程只是cpu时间到了，将其加入就绪队列
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list,&cur->general_tag);
    cur->status=TASK_READY;
    }else{}
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;
    //将thread_ready_list队列中的第一个就绪栈取出，准备调度其上cpu
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct,general_tag,thread_tag);
    next->status=TASK_RUNNING;
    switch_to(cur,next);
}
void thread_init(void){
    put_str("thread_init start!");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("thread_init done!");
}

//将当前进程阻塞，标志其状态为stat.
void thread_block(enum task_status stat){
    //stat 取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING
    ASSERT((stat==TASK_BLOCKED) ||(stat==TASK_WAITING) ||(stat==TASK_HANGING));
    enum intr_status old_status = intr_disable();
    struct task_struct* cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();//将当前线程换下处理器
    intr_set_status(old_status);
}
//将线程pthread解除阻塞
void thread_unblock(struct task_struct* pthread){
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if(pthread->status!=TASK_READY){
        ASSERT(!elem_find(&thread_ready_list,&pthread->general_tag));
        if(elem_find(&thread_ready_list,&pthread->general_tag)){
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list,&pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}
