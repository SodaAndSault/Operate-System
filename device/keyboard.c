#include "keyboard.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "global.h"
#include "ioqueue.h"

struct ioqueue kbd_buf;

#define KBD_BUF_PORT 0x60
#define esc '\033'		    //esc 和 delete都没有\转义字符这种形式，用8进制代替
#define delete '\0177'
#define enter '\r'
#define tab '\t'
#define backspace '\b'

#define char_invisible 0    //功能性 不可见字符均设置为0
#define ctrl_l_char char_invisible
#define ctrl_r_char char_invisible 
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char char_invisible
#define alt_r_char char_invisible
#define caps_lock_char char_invisible

///定义控制字符的通码和断码
#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a

//二维数组，用于记录从0x00到0x3a通码对应的按键的两种情况（如0x02，不加shift表示1，加了shift表示！）的ascii码值
//如果没有，则用ascii0替代
char keymap[][2] = {
/* 0x00 */	{0,	0},		
/* 0x01 */	{esc,	esc},		
/* 0x02 */	{'1',	'!'},		
/* 0x03 */	{'2',	'@'},		
/* 0x04 */	{'3',	'#'},		
/* 0x05 */	{'4',	'$'},		
/* 0x06 */	{'5',	'%'},		
/* 0x07 */	{'6',	'^'},		
/* 0x08 */	{'7',	'&'},		
/* 0x09 */	{'8',	'*'},		
/* 0x0A */	{'9',	'('},		
/* 0x0B */	{'0',	')'},		
/* 0x0C */	{'-',	'_'},		
/* 0x0D */	{'=',	'+'},		
/* 0x0E */	{backspace, backspace},	
/* 0x0F */	{tab,	tab},		
/* 0x10 */	{'q',	'Q'},		
/* 0x11 */	{'w',	'W'},		
/* 0x12 */	{'e',	'E'},		
/* 0x13 */	{'r',	'R'},		
/* 0x14 */	{'t',	'T'},		
/* 0x15 */	{'y',	'Y'},		
/* 0x16 */	{'u',	'U'},		
/* 0x17 */	{'i',	'I'},		
/* 0x18 */	{'o',	'O'},		
/* 0x19 */	{'p',	'P'},		
/* 0x1A */	{'[',	'{'},		
/* 0x1B */	{']',	'}'},		
/* 0x1C */	{enter,  enter},
/* 0x1D */	{ctrl_l_char, ctrl_l_char},
/* 0x1E */	{'a',	'A'},		
/* 0x1F */	{'s',	'S'},		
/* 0x20 */	{'d',	'D'},		
/* 0x21 */	{'f',	'F'},		
/* 0x22 */	{'g',	'G'},		
/* 0x23 */	{'h',	'H'},		
/* 0x24 */	{'j',	'J'},		
/* 0x25 */	{'k',	'K'},		
/* 0x26 */	{'l',	'L'},		
/* 0x27 */	{';',	':'},		
/* 0x28 */	{'\'',	'"'},		
/* 0x29 */	{'`',	'~'},		
/* 0x2A */	{shift_l_char, shift_l_char},	
/* 0x2B */	{'\\',	'|'},		
/* 0x2C */	{'z',	'Z'},		
/* 0x2D */	{'x',	'X'},		
/* 0x2E */	{'c',	'C'},		
/* 0x2F */	{'v',	'V'},		
/* 0x30 */	{'b',	'B'},		
/* 0x31 */	{'n',	'N'},		
/* 0x32 */	{'m',	'M'},		
/* 0x33 */	{',',	'<'},		
/* 0x34 */	{'.',	'>'},		
/* 0x35 */	{'/',	'?'},
/* 0x36	*/	{shift_r_char, shift_r_char},	
/* 0x37 */	{'*',	'*'},    	
/* 0x38 */	{alt_l_char, alt_l_char},
/* 0x39 */	{' ',	' '},		
/* 0x3A */	{caps_lock_char, caps_lock_char}
};

int ctrl_status = 0;        //用于记录是否按下ctrl键
int shift_status = 0;       //用于记录是否按下shift
int alt_status = 0;         //用于记录是否按下alt键
int caps_lock_status = 0;   //用于记录是否按下大写锁定
int ext_scancode = 0;       //用于记录是否是扩展码


static void intr_keyboard_handler(void){
    int break_code;//用于判断传入值是否为断码
    uint16_t scancode = inb(KBD_BUF_PORT);
    if(scancode == 0xe0){
        ext_scancode = 1;
        return;
    }
    if(ext_scancode){
        scancode = ((0xe000)|(scancode));
        ext_scancode = 0;
    }
    break_code = ((0x80 & scancode)!=0);
    if(break_code){
        uint16_t make_code = (scancode &= 0xff7f);//将断码转为通码
        if(make_code == ctrl_l_make || make_code == ctrl_r_make)
            ctrl_status = 0;
        else if(make_code == shift_l_make || make_code == shift_r_make)
            shift_status = 0;
        else if(make_code == alt_l_make || make_code == alt_r_make)
            alt_status = 0;
        return;
    }
//不是断码，而是通码，这里的判断是保证我们只处理这些数组中定义了的键，以及右alt和ctrl。
    else if((scancode == alt_r_make) || (scancode == ctrl_r_make)||(scancode>0x00 &&scancode<0x3b)){
        int shift = 0;
        uint8_t index = (scancode & 0x00ff); //将扫描码留下数组索引
        if(scancode == ctrl_l_make || scancode == ctrl_r_make){
            ctrl_status = 1;
            return;
        }
        else if(scancode == shift_l_make || scancode == shift_r_make){
            shift_status = 1;
            return;
        }
        else if(scancode == alt_l_make || scancode == alt_r_make){
            alt_status = 1;
            return;
        }
        else if(scancode == caps_lock_make){
            caps_lock_status = !caps_lock_status;
            return;
        }
        if((scancode < 0x0e)||(scancode == 0x29)||(scancode==0x1a) ||\
           (scancode == 0x1b)||(scancode == 0x2b)||(scancode==0x27)||\
           (scancode == 0x28)||(scancode==0x33)||(scancode==0x34) || (scancode == 0x35)){
            if(shift_status) shift=true;
           }
        else{
            if(shift_status +caps_lock_status==1)
                shift=1;
           }
        char cur_char = keymap[index][shift];
        if(cur_char){
            if((ctrl_status && cur_char=='l')||(ctrl_status && cur_char=='u')){
                cur_char = cur_char - 'a';
            }
            if(!ioq_full(&kbd_buf)){
                put_char(cur_char);
                ioq_putchar(&kbd_buf,cur_char);
            }
            return;
        }
    }
    else
        put_str("unknown jey\n");
    return;
}
//键盘初始化
void keyboard_init() {
   put_str("keyboard init start\n");
   ioqueue_init(&kbd_buf);
   register_handler(0x21, intr_keyboard_handler);
   put_str("keyboard init done\n");
}