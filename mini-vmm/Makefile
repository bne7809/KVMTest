all: vmm guest.bin
guest.bin: guest.S
	as --32 -o guest.o guest.S
	ld -m elf_i386 -Ttext=0x1000 -e _start --oformat binary -o guest.bin guest.o
vmm: vmm.c
	gcc -Wall -Wextra -O2 -o vmm vmm.c
clean:
	rm -f vmm guest.o guest.bin
