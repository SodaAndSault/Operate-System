#include "string.h"
#include "global.h"
#include "debug.h"
//将dst_起始的size个字节置为value
void memset(void* dst_, uint8_t value, uint32_t size){
    ASSERT(dst_!=NULL);
    uint8_t* dst = (uint8_t*)dst_;
    while(size-->0){
        *dst++ = value;
    }
}
//将src起始的size个字节复制到dst_
void memcpy(void* dst_, const void* src_, uint32_t size){
    ASSERT(dst_!=NULL && src_!=NULL);
    uint8_t* dst = dst_;
    const uint8_t* src = src_;
    while(size-- > 0)
        *dst++ = *src++;
}
//连续比较以地址a_和地址b_开头的size个字节，若相等则返回0，a_大于b_返回+1，否则返回-1
int memcmp(const void* a_, const void* b_, uint32_t size) {
    const char* a = a_;
    const char* b = b_;
    ASSERT(a != NULL || b != NULL);
    while (size-- > 0) {
        if(*a != *b) {
	        return *a > *b ? 1 : -1; 
        }
    a++;
    b++;
    }
   return 0;
}
//将字符串从src_复制到dst_
char* strcpy(char* dst_, const char* src_) {
    ASSERT(dst_ != NULL && src_ != NULL);
    char* r = dst_;		       // 用来返回目的字符串起始地址
    while((*dst_++ = *src_++));  //1、*dst=*src  2、判断*dst是否为'\0'，然后决定是否执行循环体，本步骤真假值不影响3   3、dst++与scr++，谁先谁后不知道
    return r;                    //上面多出来的一对括号，是为了告诉编译器，我这里的=就是自己写的，而不是将==错误写成了=
}
//返回字符串长度
uint32_t strlen(const char* str){
    ASSERT(str!=NULL);
    const char* p = str;
    while(*p++);
    return p-str-1;
  }
//比较两个字符串，若a_中的字符大于b_中的字符，则返回1，a_小于b_返回-1，相等则返回0；
uint8_t strcmp(const char* a,const char* b){
    ASSERT(a!=NULL && b!=NULL);
    while(*a!=0 && *a == *b){
        a++;
        b++;
    }
    return *a<*b ? -1:*a>*b;
}
//从左到右查找字符串str中首次出现字符ch的地址
char* strchr(const char* str,const int8_t ch){
    ASSERT(str!=NULL);
    while(*str!=0){
        if(*str == ch)
            return (char*)str;
        str++;
    }
    return NULL;
}
//从后往前查找字符串str中首次出现字符ch的地址
char* strrchr(const char* str,const uint8_t ch){
    ASSERT(str!=NULL);
    const char* last_str = NULL;
    while(*str!=NULL){
        if(*str == ch)
            last_str = str;
        str++;
    }
    return (char*)last_str;
}
//将字符串strc_拼接到dst_之后，返回拼接的串地址
char* strcat(char* dst_,char* src_){
    ASSERT(dst_!=NULL && src_!=NULL);
    char* str = dst_;
    while(*str++);
    --str;
    while((*str++ = *src_++));
    return dst_;
}
//在字符串str中查找字符ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch){
    ASSERT(str!=NULL);
    uint32_t ch_cnt = 0;
    const char* p = str;
    while(*p!=0){
        if(*p==ch)
            ch_cnt++;
        p++;
    }
    return ch_cnt;
}