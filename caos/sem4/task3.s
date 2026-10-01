	.intel_syntax noprefix
        .global main

main:

    call readi64 # read number in rax
    mov rbx, rax # rbx - number of slagaemyx
    xor rcx, rcx # counter
    xor rdi, rdi # sum

sum:

    cmp rbx, rcx
    jz end
    inc rcx
    add rdi, rcx
    jmp sum

end:

    call writei64
    call finish
