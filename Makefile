CC      ?= i686-elf-gcc
INC      = -Ikernel/include -Ikernel/arch/i386 -Ikernel/drivers -Ikernel/wm
CFLAGS   = -m32 -march=i686 -mno-mmx -mno-sse -mno-sse2 -ffreestanding -fno-pie -fno-stack-protector -O2 -Wall -Wextra $(INC)
LDFLAGS  = -m32 -nostdlib -T kernel/linker.ld -Wl,--build-id=none -no-pie

ASM_SRC  = kernel/arch/i386/boot.S
C_SRC    = $(wildcard kernel/core/*.c kernel/drivers/*.c kernel/wm/*.c)
OBJ      = $(ASM_SRC:.S=.o) $(C_SRC:.c=.o)

all: web/kernel.bin

kernel/kernel.elf: $(OBJ) kernel/linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

web/kernel.bin: kernel/kernel.elf
	objcopy -O binary $< $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	cd web && python3 -m http.server 8001

clean:
	rm -f $(OBJ) kernel/kernel.elf web/kernel.bin