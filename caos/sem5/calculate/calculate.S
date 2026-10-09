// Implement function 
// int64_t calculate(int64_t a, int64_t b, ..., int64_t y) {
//     return (x * (y + 1)) + (a + b + c + d + e + f);
// }

.intel_syntax noprefix
.global calculate

calculate:

        push rbp // save callee-saved rbp
        mov rbp, rsp

        mov     rax, [rbp + 16] # rax <- x
        mov     r10, [rbp + 24] # r10 <- y
        add     r10, 1          # r10 <- y + 1
        imul    rax, r10        # rax <- x * (y + 1)

        add     rdi, rsi        # rdi <- a + b
        add     rdi, rdx        # rdi <- a + b + c
        add     rdi, rcx        # rdi <- a + b + c + d
        add     rdi, r8         # rdi <- a + b + c + d + e
        add     rdi, r9         # rdi <- a + b + c + d + e + f

        add     rax, rdi        # rax <- x * (y + 1) + (a + b + ... + f)

        mov rsp, rbp
        pop     rbp             # return previous value of callee-saved rbp

        ret                     # get return address from stack and return to caller function