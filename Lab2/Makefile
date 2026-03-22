RISCV_GNU ?= riscv64-unknown-elf
CC = $(RISCV_GNU)-gcc
LD = $(RISCV_GNU)-ld
OBJCOPY = $(RISCV_GNU)-objcopy
CFLAGS = -mcmodel=medany -ffreestanding -nostdlib -g -Wall
QEMU ?= qemu-system-riscv64
TARGET = bootloader

ifeq ($(PLATFORM), QEMU)
     CFLAGS += -DUSE_QEMU
endif

build: clean
#	$(CC) $(CFLAGS) -c *.S *.c
#	$(LD) -T link.ld -o $(TARGET).elf *.o
#	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET)
	$(CC) $(CFLAGS) -c start.S uart.c bootloader.c
	$(LD) -T link.ld -o $(TARGET).elf start.o uart.o bootloader.o
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin

	$(CC) $(CFLAGS) -c ker_start.S -o ker_start.o
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o
	$(LD) -T ker_link.ld -o kernel.elf ker_start.o kernel.o uart.o
	$(OBJCOPY) -O binary kernel.elf kernel.bin

run: build
	$(QEMU) -M virt -m 256M -kernel $(TARGET).bin -initrd initramfs.cpio -display none -serial pty

clean:
	rm -f *.bin *.elf *.o
