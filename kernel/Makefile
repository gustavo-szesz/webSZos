CC      ?= i686-elf-gcc
CFLAGS   = -m32 -march=i686 -mno-mmx -mno-sse -mno-sse2 -ffreestanding -fno-pie -fno-stack-protector -O2 -Wall -Wextra
LDFLAGS  = -m32 -nostdlib -T linker.ld -Wl,--build-id=none -no-pie

all: kernel.bin

kernel.elf: boot.o kernel.o linker.ld
	$(CC) $(LDFLAGS) -o $@ boot.o kernel.o

kernel.bin: kernel.elf
	objcopy -O binary kernel.elf kernel.bin

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o kernel.elf kernel.bin
