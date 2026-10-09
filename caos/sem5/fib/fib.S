        .intel_syntax noprefix
        .global fib

fib:
        push rbp
        mov rbp, rsp
        push rbx
        push r12

        # case n = 0
        cmp rdi, 0
        je return_zero

        # case n = 1
        cmp rdi, 1
        je return_one

        # F(n - 1)
        mov rbx, rdi # rbx <- n
        dec rdi # rdi <- n - 1
        call fib

        # F(n - 2)
        mov r12, rax # rcx <- F(n - 1)
        mov rdi, rbx
        sub rdi, 2 # rdi <- n - 2
        call fib

        # F(n) = F(n - 1) + F(n - 2)
        add rax, r12 # rax <- F(n - 2) + F(n - 1) 
        pop r12
        pop rbx
        pop rbp
        ret


return_zero:
        pop r12
        pop rbx
        pop rbp
        mov rax, 0
        ret

return_one:
        pop r12
        pop rbx
        pop rbp
        mov rax, 1
        ret
