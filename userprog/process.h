#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H
#include "thread.h"
uint32_t* create_page_dir(void);
#define USER_VADDR_START 0x8048000

#endif