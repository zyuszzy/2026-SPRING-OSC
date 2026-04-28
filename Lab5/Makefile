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

INCLUDES = -I$(SHARED_DIR)/include -I$(KERN_DIR)/include

# ------------------------------------------------------------------
# 編譯目標
# ------------------------------------------------------------------

all: bootloader.bin kernel.bin

# Shared
shared_objs = $(SHARED_DIR)/src/fdt.o $(SHARED_DIR)/src/string.o
$(SHARED_DIR)/src/%.o: $(SHARED_DIR)/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Bootloader
boot_shared_objs = $(SHARED_DIR)/src/fdt_boot.o $(SHARED_DIR)/src/string_boot.o
boot_objs = $(BOOT_DIR)/start.o $(BOOT_DIR)/main.o $(BOOT_DIR)/uart.o

$(SHARED_DIR)/src/%_boot.o: $(SHARED_DIR)/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DBOOTLOADER -c $< -o $@

$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DBOOTLOADER -c $< -o $@

bootloader.elf: $(boot_objs) $(boot_shared_objs)
	$(LD) -T $(BOOT_DIR)/link.ld -o $@ $^

bootloader.bin: bootloader.elf
	$(OBJCOPY) -O binary $< $@

# Kernel
kern_objs = $(KERN_DIR)/src/start.o $(KERN_DIR)/src/main.o $(KERN_DIR)/src/uart.o $(KERN_DIR)/src/shell.o $(KERN_DIR)/src/trap.o $(KERN_DIR)/src/initrd.o $(KERN_DIR)/src/mm.o $(KERN_DIR)/src/timer.o $(KERN_DIR)/src/sbi.o $(KERN_DIR)/src/uart.o $(KERN_DIR)/src/plic.o $(KERN_DIR)/src/thread.o $(KERN_DIR)/src/task.o
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
	$(QEMU) -M virt -m 256M -kernel bootloader.bin -initrd initramfs.cpio -display none -serial pty

clean:
	rm -f $(BOOT_DIR)/*.o $(KERN_DIR)/src/*.o $(SHARED_DIR)/src/*.o *.elf *.bin