#include "../thread/sync.h"
#include "print.h"
#include "stdint.h"

static struct lock console_lock;
void console_init(){
    lock_init(&console_lock);
}
void console_acquire(){
    lock_acpuire(&console_lock);
}
void console_release(){
    lock_release(&console_lock);
}
void console_put_str(char* str){
    console_acquire();
    put_str(str);
    console_release();
}
void console_put_char(uint8_t char_asic){
    console_acquire();
    put_char(char_asic);
    console_release();
}
void console_put_int(uint32_t num){
    console_acquire();
    put_int(num);
    console_release();
}