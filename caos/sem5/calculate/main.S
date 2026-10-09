.intel_syntax noprefix
.global main

main:
    # prologue
    push rbp
    mov rbp, rsp

    # save to stack callee-saved registers
    # only if function changes them
    push rbx
    push r12
    push r13
    push r14
    push r15

    # rsp here must be aligned with 16 bytes before call;
    # here rsp % 16 == 8, as before call of main we put
    # return address on stack;
    # reserve 8 bytes to meet alignment + 16 bytes for local
    # variables
    sub rsp, 24

    call readi64
    mov rbx, rax        # a
    # same
    call readi64
    mov r12, rax        # b

    call readi64
    mov r13, rax        # c

    call readi64
    mov r14, rax        # d

    call readi64
    mov r15, rax        # e

    call readi64
    mov  QWORD PTR [rbp - 48], rax  # f

    call readi64
    mov  QWORD PTR [rbp - 56], rax  # x

    call  readi64
    mov r10, rax        # y

    mov rdi, rbx        # a
    mov rsi, r12        # b
    mov rdx, r13        # c
    mov rcx, r14        # d
    mov r8, r15         # e
    mov r9, QWORD PTR [rbp - 48]  # f

    # № of arg increases as well as stack address
    push r10                  # y (arg 8)
    push QWORD PTR [rbp - 56] # x (arg 7)

    call    calculate

    add     rsp, 16         # clear stack from args x and y
    mov     rdi, rax
    call    writei64

    add rsp, 24

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx

    # epilogue
    
    mov rsp, rbp
    pop rbp

    ret
