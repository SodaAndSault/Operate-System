#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define IDT_DESC_CNT 0x30
//定义端口号
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1
//定义开中断情况下的eflags寄存器的值
#define EFLAGS_IF 0x0000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile ("pushfl;popl %0":"=g"(EFLAG_VAR)) 
//定义段描述符表项结构体
struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t  dcount;  
    uint8_t  attribute;
    uint16_t func_offset_high_word;
};
//定义中断名字数组
char* intr_name[IDT_DESC_CNT];
//定义中断程序地址数组
intr_handler idt_table[IDT_DESC_CNT];
//声明Kernel.S中的中断处理函数入口数组
extern intr_handler intr_entry_table[IDT_DESC_CNT];
//函数声明
static void make_idt_desc(struct gate_desc* p_gdesc,uint8_t attr,intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];
//创建中断门描述符
static void make_idt_desc(struct gate_desc* p_gdesc,uint8_t attr,intr_handler function){
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_low_word = (uint32_t)function & 0xFFFF0000 >> 16;
}
//初始化中断描述符
static void idt_desc_init(void){
    int i;
    for(i=0;i<IDT_DESC_CNT;i++){
        make_idt_desc(&idt[i],IDT_DESC_ATTR_DPL0,intr_entry_table[i]);
    }
    put_str("idt_desc_init done\n");
}

/* 初始化可编程中断控制器8259A */
static void pic_init(void) {

   /* 初始化主片 */
   outb (PIC_M_CTRL, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
   outb (PIC_M_DATA, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
   outb (PIC_M_DATA, 0x04);   // ICW3: IR2接从片. 
   outb (PIC_M_DATA, 0x01);   // ICW4: 8086模式, 正常EOI

   /* 初始化从片 */
   outb (PIC_S_CTRL, 0x11);	// ICW1: 边沿触发,级联8259, 需要ICW4.
   outb (PIC_S_DATA, 0x28);	// ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
   outb (PIC_S_DATA, 0x02);	// ICW3: 设置从片连接到主片的IR2引脚
   outb (PIC_S_DATA, 0x01);	// ICW4: 8086模式, 正常EOI

   /* 打开主片上IR0,也就是目前只接受时钟产生的中断 */
   //outb (PIC_M_DATA, 0xfe);
   //outb (PIC_S_DATA, 0xff);

   /* 测试键盘,只打开键盘中断，其它全部关闭 */
   //outb (PIC_M_DATA, 0xfd);   //键盘中断在主片ir1引脚上，所以将这个引脚置0，就打开了
   //outb (PIC_S_DATA, 0xff);

   //同时打开时钟中断与键盘中断
   outb (PIC_M_DATA, 0xfc);
   outb (PIC_S_DATA, 0xff);

   put_str("   pic_init done\n");
}


//通用中断处理函数
static void general_intr_handler(uint8_t vec_nr) {
   if (vec_nr == 0x27 || vec_nr == 0x2f) {	//伪中断向量，无需处理。详见书p337
      return;		
   }
   //将光标置为0，从屏幕左上角清除出来一片打印异常信息的区域，方便阅读
   set_cursor(0);
   int cursor_pos = 0;
   while(cursor_pos < 320){
        put_char(' ');
        cursor_pos++;
   }
   //重置光标为屏幕左上角
   set_cursor(0);
   put_str("!!!!!!!      excetion message begin  !!!!!!!!\n");
   //从第二行的第8个字符开始打印
   set_cursor(88);
   put_str(intr_name[vec_nr]);
   if(vec_nr==14){//若为缺页中断，将缺失的地址打印出并悬停
        int page_fault_vaddr = 0;
        asm("movl %%cr2,%0":"=r"(page_fault_vaddr));//cr2是存放造成page_fault的地址
        put_str("\npage fault addr is ");put_int(page_fault_vaddr); 
   }
   put_str("\n!!!!!!!      excetion message end    !!!!!!!!\n");
  // 能进入中断处理程序就表示已经处在关中断情况下,
  // 不会出现调度进程的情况。故下面的死循环不会再被中断。
   while(1);
}
//在中断处理函数数组第vector_num个元素中注册安装中断处理函数function
void register_handler(uint8_t vector_no,intr_handler function){
    idt_table[vector_no] = function;
}

static void exception_init(void){
    int i;
    for(i=0;i<IDT_DESC_CNT;i++){
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";  
}

void idt_init(){
    put_str("idt_init strat!\n");
    idt_desc_init();
    exception_init();
    pic_init();
    //加载到IDTR寄存器中的值
    uint64_t idt_operand = (sizeof(idt)-1) | ((uint64_t)((uint32_t)idt << 16));//IDT段界限 
    asm volatile ("lidt %0": :"m" (idt_operand));
    put_str("idt_init_done\n");
}

//获取当前中断状态
enum intr_status intr_get_status(){
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON:INTR_OFF;
}
//开中断并返回中断前的状态
enum  intr_status intr_enable(){
    enum intr_status old_status;
    if(INTR_ON == intr_get_status()){
        old_status = INTR_ON;
        return old_status;
    }else{
        old_status = INTR_OFF;
        asm volatile("sti");
        return old_status;
    }
}
//关中断并返回中断前的状态
enum intr_status intr_disable(){
    enum intr_status old_status;
    if(INTR_ON == intr_get_status()){
        old_status = INTR_ON;
        asm volatile("cli":::"memory");
        return old_status;
    }else{
        old_status = INTR_OFF;
        return old_status;
    }
}
//将中断设置为status,并返回旧中断状态
enum intr_status intr_set_status(enum intr_status status){
    return status & INTR_ON ? intr_enable():intr_disable();
}