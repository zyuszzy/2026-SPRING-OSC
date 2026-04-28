import struct
import sys

if len(sys.argv) < 2:
    print("How to use: python3 send_kernel.py <device path /dev/pts/X>")
    sys.exit(1)

kernel_file_path = "kernel.bin"
tty_device = sys.argv[1]

with open(kernel_file_path, "rb") as f:
    kernel_data = f.read()
    
header = struct.pack('<II', 0x544F4F42, len(kernel_data))

with open(tty_device, "wb", buffering = 0) as tty:
    tty.write(header)
    tty.write(kernel_data)

