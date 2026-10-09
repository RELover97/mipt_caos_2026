.intel_syntax noprefix
    .global main

main:

    call readi64

    mov rdi, rax
    call fib

    mov rdi, rax
    call writei64

    call finish
