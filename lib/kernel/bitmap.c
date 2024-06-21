#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

//初始化位图 bitmap
void bitmap_init(struct bitmap* btmp){
    memset(btmp->bits,0,btmp->btmp_bytes_len);
}
//判断bit——idx是否为1，若为1，则返回true，否则返回false
bool bitmap_scan_test(struct bitmap* btmp,uint32_t bit_idx){
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    if(btmp->bits[byte_idx] & BITMAP_MASK << bit_odd) return 1;
    else return 0;
}
//在位图中申请连续cnt个位，成功，则返回其起始下标，失败则返回-1
int bitmap_scan(struct bitmap* bitmap, uint32_t cnt){
    uint32_t area_start = 0, area_size = 0;    //用来存储一个连续为0区域的起始位置, 存储一个连续为0的区域大小
    while(1){                   
        while( bitmap_scan_test(bitmap, area_start) && area_start / 8 < bitmap->btmp_bytes_len) //当这个while顺利结束1、area_start就是第一个0的位置；2、area_start已经越过位图边界
            area_start++;
        if(area_start / 8 >= bitmap->btmp_bytes_len)    //上面那个循环跑完可能是area_start已经越过边界，说明此时位图中是全1，那么就没有可分配内存
            return -1;
        area_size = 1;  //来到了这一句说明找到了位图中第一个0，那么此时area_size自然就是1
        while( area_size < cnt ){
            if( (area_start + area_size) / 8 < bitmap->btmp_bytes_len ){    //确保下一个要判断的位不超过边界
                if( bitmap_scan_test(bitmap, area_start + area_size) == 0 ) //判断区域起始0的下一位是否是0
                    area_size++;
                else
                    break;  //进入else，说明下一位是1，此时area_size还没有到达cnt的要求，且一片连续为0的区域截止，break
            }
            else
                return -1;  //来到这里面，说面下一个要判断的位超过边界，且area_size<cnt，返回-1
        }
        if(area_size == cnt)    //有两种情况另上面的while结束，1、area_size == cnt；2、break；所以用需要判断
            return area_start;
        area_start += (area_size+1); //更新area_start，判断后面是否有满足条件的连续0区域
    }
}

//将位图btmp的bit_idx位设置为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
   ASSERT((value == 0) || (value == 1));
   uint32_t byte_idx = bit_idx / 8;    //确定要设置的位所在字节的偏移
   uint32_t bit_odd  = bit_idx % 8;    //确定要设置的位在某个字节中的偏移

/* 一般都会用个0x1这样的数对字节中的位操作,
 * 将1任意移动后再取反,或者先取反再移位,可用来对位置0操作。*/
   if (value) {		      // 如果value为1
      btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
   } else {		      // 若为0
      btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
   }
}