.section .multiboot2, "a"
.align 8

.set MB2_MAGIC, 0xe85250d6
.set MB2_ARCH, 0
.set MB2_LENGTH, mb2_header_end - mb2_header_start
.set MB2_CHECKSUM, -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)

mb2_header_start:
    .long MB2_MAGIC
    .long MB2_ARCH
    .long MB2_LENGTH
    .long MB2_CHECKSUM

    /* framebuffer request: 1024x768x32 */
    .align 8
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32

    .align 8
    .short 0
    .short 0
    .long 8

mb2_header_end:

.section .text, "ax"
.global _start
.type _start, @function

_start:
    mov $stack_top, %esp
    push %ebx
    push %eax
    call kernel_main
    add $8, %esp

.hang:
    cli
    hlt
    jmp .hang

.section .bss, "aw"
.align 16
stack_bottom:
    .skip 16384
stack_top:
