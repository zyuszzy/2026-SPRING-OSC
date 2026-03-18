RISCV_GNU ?= riscv64-unknown-elf
CC = $(RISCV_GNU)-gcc
LD = $(RISCV_GNU)-ld
OBJCOPY = $(RISCV_GNU)-objcopy
CFLAGS = -mcmodel=medany -ffreestanding -nostdlib -g -Wall
QEMU ?= qemu-system-riscv64
TARGET = kernel

ifeq ($(PLATFORM), QEMU)
    CFLAGS += -DUSE_QEMU
endif

build: clean
	$(CC) $(CFLAGS) -c *.S *.c
	$(LD) -T link.ld -o $(TARGET).elf *.o
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET)

run: build $(TARGET)
	$(QEMU) -M virt -m 256M -kernel $(TARGET) -display none -serial pty

clean:
	rm -f $(TARGET) $(TARGET).elf *.o
