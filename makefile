#定义变量，将多次用到的语句变为变量方便替换
BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
HD60M_PATH = /home/hwz/Desktop/bochs/hd60M.img
 
AS = nasm
CC = gcc-4.4
LD = ld 
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/ -I userprog/
ASFLAGS = -f elf -g
CFLAGS = -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -m32 -g
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map -m elf_i386
OBJS=$(BUILD_DIR)/main.o $(BUILD_DIR)/init.o \
	$(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o \
	$(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o \
	$(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o \
	$(BUILD_DIR)/tss.o

boot:$(BUILD_DIR)/mbr.o $(BUILD_DIR)/loader.o
$(BUILD_DIR)/mbr.o:boot/mbr.S
	$(AS) -I boot/include/ -o build/mbr.o boot/mbr.S
$(BUILD_DIR)/loader.o:boot/loader.S
	$(AS) -I boot/include/ -o build/loader.o boot/loader.S

#编译c内核代码
$(BUILD_DIR)/main.o:kernel/main.c 
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/init.o:kernel/init.c 
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/interrupt.o:kernel/interrupt.c 
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/timer.o:device/timer.c 
	$(CC) $(CFLAGS) -o $@ $<
	
$(BUILD_DIR)/debug.o:kernel/debug.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/string.o:lib/kernel/string.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/bitmap.o:lib/kernel/bitmap.c
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/memory.o:kernel/memory.c
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/thread.o:thread/thread.c
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/list.o:lib/kernel/list.c
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/sync.o:thread/sync.c
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/console.o:device/console.c 
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/keyboard.o:device/keyboard.c 
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/ioqueue.o:device/ioqueue.c 
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD_DIR)/tss.o:userprog/tss.c 
	$(CC) $(CFLAGS) -o $@ $<
#编译汇编内核代码
$(BUILD_DIR)/kernel.o:kernel/kernel.S
	$(AS) $(ASFLAGS) -o $@ $<
$(BUILD_DIR)/print.o:lib/kernel/print.S
	$(AS) $(ASFLAGS) -o $@ $<
$(BUILD_DIR)/switch.o:thread/switch.S
	$(AS) $(ASFLAGS) -o $@ $<
#链接所有目标内核文件
$(BUILD_DIR)/kernel.bin:$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
#定义6个伪目标，不需要对比文件的修改时间即可执行命令语句
.PHONY:mk_dir hd clean build all boot gdb_symbol
mk_dir:
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi 
hd:
	dd if=build/mbr.o of=$(HD60M_PATH) count=1 bs=512 conv=notrunc && \
	dd if=build/loader.o of=$(HD60M_PATH) count=4 bs=512 seek=2 conv=notrunc && \
	dd if=$(BUILD_DIR)/kernel.bin of=$(HD60M_PATH) bs=512 count=200 seek=9 conv=notrunc
clean:
	@cd $(BUILD_DIR) && rm -f ./* && echo "remove ./build all done"

build:$(BUILD_DIR)/kernel.bin
gdb_symbol:
	objcopy --only-keep-debug $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.sym

all:mk_dir boot build hd gdb_symbol
