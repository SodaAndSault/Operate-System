#include "../userprog/process.h"
#include "../thread/thread.h"
#include "stdint.h"
#include "global.h"
#include "bitmap.h"
#include "console.h"
#include "string.h"

extern void intr_exit(void);

void creat_user_vaddr_bitmap(struct task_struct* user_prog){
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START)/PG_SIZE/8,PG_SIZE);//向上取整，位图所需地址空间（位图占了多少页）
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&(user_prog->userprog_vaddr.vaddr_bitmap)); 
}
//创建用户进程的页目录表，并将页目录表的的高255项拷贝内核页目录表的高255项
uint32_t* create_page_dir(void){
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if(page_dir_vaddr==NULL){
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr+768*4),(uint32_t*)(0xfffff000 + 768 * 4),255 * 4);
    int32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);//将进程的页目录表的虚拟地址转换为物理地址
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}
//用于初始化进入进程所需要的中断栈信息，传入参数是实际要运行的函数地址（进程），这个函数是线程启动器进入的（kernel_thread）
void start_process(void* filename_){
    void* function = filename_;
    struct task_struct* cur = running_thread();
    cur->self_kstack += sizeof(struct task_struct);
}




