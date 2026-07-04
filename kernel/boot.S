.set MAGIC, 0xe85250d6
.set ARCH, 0
.set LENGTH, header_end - header_start
.set CHECKSUM, -(MAGIC + ARCH + LENGTH)

.section .multiboot2, "a"
.align 8
header_start:
.long MAGIC
.long ARCH
.long LENGTH
.long CHECKSUM

.align 8
.short 0
.short 0
.long 8
header_end:

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp

    /*
     * Multiboot2 handoff:
     * eax = magic
     * ebx = multiboot2 information structure address
     *
     * C cdecl args are pushed right-to-left:
     * kernel_main(uint32_t magic, uint32_t mbi_addr)
     */
    push %ebx
    push %eax
    call kernel_main
    add $8, %esp

.hang:
    cli
    hlt
    jmp .hang
