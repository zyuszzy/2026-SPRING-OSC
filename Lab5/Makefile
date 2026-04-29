# ------------------------------------------------------------
# 工具鏈定義
# ------------------------------------------------------------
RISCV_GNU ?= riscv64-unknown-elf
CC      = $(RISCV_GNU)-gcc
LD      = $(RISCV_GNU)-ld
OBJCOPY = $(RISCV_GNU)-objcopy
QEMU    = qemu-system-riscv64

# ------------------------------------------------------------
# 目錄定義
# ------------------------------------------------------------
SHARED_DIR = shared
BOOT_DIR   = bootloader
KERN_DIR   = kernel

# ------------------------------------------------------------
# 編譯選項
# ------------------------------------------------------------
# -MMD 會產生 .d 檔來追蹤標頭檔依賴
CFLAGS = -mcmodel=medany -ffreestanding -nostdlib -g -Wall -MMD
INCLUDES = -I$(SHARED_DIR)/include -I$(KERN_DIR)/include

ifeq ($(PLATFORM), QEMU)
    CFLAGS += -DUSE_QEMU
endif

# ------------------------------------------------------------
# 自動偵測檔案 (核心邏輯)
# ------------------------------------------------------------

# 1. Kernel 檔案
KERN_C_SRCS   = $(wildcard $(KERN_DIR)/src/*.c)
KERN_S_SRCS   = $(wildcard $(KERN_DIR)/src/*.S)
KERN_OBJS     = $(KERN_C_SRCS:.c=.o) $(KERN_S_SRCS:.S=.o)

# 2. Shared 檔案 (供 Kernel 使用)
SHARED_C_SRCS = $(wildcard $(SHARED_DIR)/src/*.c)
SHARED_OBJS   = $(SHARED_C_SRCS:.c=.o)

# 3. Bootloader 檔案
BOOT_C_SRCS   = $(wildcard $(BOOT_DIR)/*.c)
BOOT_S_SRCS   = $(wildcard $(BOOT_DIR)/*.S)
BOOT_OBJS     = $(BOOT_C_SRCS:.c=.o) $(BOOT_S_SRCS:.S=.o)

# 4. Shared 檔案 (供 Bootloader 使用，檔名加上 _boot 避免衝突)
BOOT_SHARED_OBJS = $(patsubst $(SHARED_DIR)/src/%.c, $(SHARED_DIR)/src/%_boot.o, $(SHARED_C_SRCS))

ALL_DEPS = $(KERN_OBJS:.o=.d) $(SHARED_OBJS:.o=.d) $(BOOT_OBJS:.o=.d) $(BOOT_SHARED_OBJS:.o=.d)

# ------------------------------------------------------------
# 主要目標
# ------------------------------------------------------------

.PHONY: all clean run

all: bootloader.bin kernel.bin

# --- Bootloader 連結 ---
bootloader.elf: $(BOOT_OBJS) $(BOOT_SHARED_OBJS)
	$(LD) -T $(BOOT_DIR)/link.ld -o $@ $^

bootloader.bin: bootloader.elf
	$(OBJCOPY) -O binary $< $@

# --- Kernel 連結 ---
kernel.elf: $(KERN_OBJS) $(SHARED_OBJS)
	$(LD) -T $(KERN_DIR)/link.ld -o $@ $^

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

# ------------------------------------------------------------
# 編譯規則
# ------------------------------------------------------------

# Bootloader
$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DBOOTLOADER -c $< -o $@

$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.S
	$(CC) $(CFLAGS) $(INCLUDES) -DBOOTLOADER -c $< -o $@

$(SHARED_DIR)/src/%_boot.o: $(SHARED_DIR)/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DBOOTLOADER -c $< -o $@

# Kernel
$(KERN_DIR)/src/%.o: $(KERN_DIR)/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(KERN_DIR)/src/%.o: $(KERN_DIR)/src/%.S
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SHARED_DIR)/src/%.o: $(SHARED_DIR)/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------------
# 指令區
# ------------------------------------------------------------

run: all
	$(QEMU) -M virt -m 256M -kernel bootloader.bin -initrd initramfs.cpio -display none -serial pty

clean:
	rm -f $(BOOT_DIR)/*.o $(BOOT_DIR)/*.d
	rm -f $(KERN_DIR)/src/*.o $(KERN_DIR)/src/*.d
	rm -f $(SHARED_DIR)/src/*.o $(SHARED_DIR)/src/*.d
	rm -f *.elf *.bin

-include $(ALL_DEPS)