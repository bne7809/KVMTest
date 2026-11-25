.code16
.globl _start

_start:
	mov $0x3f8, %dx
	mov $msg, %si
1:	lodsb
	test %al, %al
	jz 2f
	out %al, (%dx)
	jmp 1b
2:	mov $42, %al
	out %al, $0xf4
	hlt

msg: .asciz "Hello from the guest!\n"
