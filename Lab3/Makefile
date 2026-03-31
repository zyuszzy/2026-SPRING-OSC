RISCV_GNU ?= riscv64-unknown-elf
CC = $(RISCV_GNU)-gcc
LD = $(RISCV_GNU)-ld
OBJCOPY = $(RISCV_GNU)-objcopy
CFLAGS = -mcmodel=medany -ffreestanding -nostdlib -g -Wall
QEMU = qemu-system-riscv64

ifeq ($(PLATFORM), QEMU)
     CFLAGS += -DUSE_QEMU
endif

# 目錄定義
SHARED_DIR = shared
BOOT_DIR = bootloader
KERN_DIR = kernel

# Include 路徑 (讓 gcc 知道去哪裡找 type.h, uart.h 等)
INCLUDES = -I$(SHARED_DIR)/include -I$(KERN_DIR)/include

# ------------------------------------------------------------------
# 編譯目標
# ------------------------------------------------------------------

all: bootloader.bin kernel.bin

# 1. 編譯 Shared 模組 (UART, FDT, String)
shared_objs = $(SHARED_DIR)/src/uart.o $(SHARED_DIR)/src/fdt.o $(SHARED_DIR)/src/string.o
$(SHARED_DIR)/src/%.o: $(SHARED_DIR)/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 2. 編譯 Bootloader
boot_objs = $(BOOT_DIR)/start.o $(BOOT_DIR)/main.o
bootloader.elf: $(boot_objs) $(shared_objs)
	$(LD) -T $(BOOT_DIR)/link.ld -o $@ $^

bootloader.bin: bootloader.elf
	$(OBJCOPY) -O binary $< $@

# 3. 編譯 Kernel
kern_objs = $(KERN_DIR)/src/start.o $(KERN_DIR)/src/main.o $(KERN_DIR)/src/shell.o $(KERN_DIR)/src/initrd.o $(KERN_DIR)/src/mm.o
kernel.elf: $(kern_objs) $(shared_objs)
	$(LD) -T $(KERN_DIR)/link.ld -o $@ $^
kernel.elf: $(kern_objs) $(shared_objs)
	$(LD) -T $(KERN_DIR)/link.ld -o $@ $^

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

# 通用編譯規則
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------------------
# 指令區
# ------------------------------------------------------------------

run: clean all
	$(QEMU) -M virt -m 256M -kernel bootloader.bin -display none -serial pty

clean:
	rm -f $(BOOT_DIR)/*.o $(KERN_DIR)/src/*.o $(SHARED_DIR)/src/*.o *.elf *.bin