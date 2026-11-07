# Bare-metal RISC-V starter for QEMU
# Usage:
#   make            build app.elf (32-bit by default)
#   make run        run in QEMU
#   make debug      start QEMU paused, waiting for GDB on port 1234
#   make gdb        attach GDB (run this in a second terminal)
#   make dump       disassemble the ELF
#   make XLEN=64    build/run the 64-bit variant instead

XLEN   ?= 32
PREFIX ?= riscv64-elf-

ifeq ($(XLEN),32)
  ARCH = rv32imac
  ABI  = ilp32
  QEMU = qemu-system-riscv32
else
  ARCH = rv64imac
  ABI  = lp64
  QEMU = qemu-system-riscv64
endif

CC      = $(PREFIX)gcc
GDB     = $(PREFIX)gdb
OBJDUMP = $(PREFIX)objdump

CFLAGS  = -march=$(ARCH) -mabi=$(ABI) -mcmodel=medany \
          -ffreestanding -nostdlib -static -O0 -g -Wall
LDFLAGS = -T linker.ld

QEMUFLAGS = -machine virt -nographic -bios none -kernel app.elf

all: app.elf

app.elf: start.S main.c linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) start.S main.c -o $@

run: app.elf
	$(QEMU) $(QEMUFLAGS)

debug: app.elf
	$(QEMU) $(QEMUFLAGS) -S -gdb tcp::1234

gdb: app.elf
	$(GDB) app.elf -ex "target remote :1234" -ex "break main"

dump: app.elf
	$(OBJDUMP) -d app.elf

clean:
	rm -f app.elf

.PHONY: all run debug gdb dump clean
