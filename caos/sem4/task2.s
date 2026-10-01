    .intel_syntax noprefix
    .global main

main:

    xor rdi, rdi # array counter
    mov rsi, [rip + len] # array length
    lea r11, [rip + array] # least effective address, r11 <- address of array
    mov r10, [rip + array] # maximum array element, 0th array element
    inc rdi # rdi <- 1

traverse_array:

    cmp rsi, rdi # sub rsi, rdi; rflags is updated
    jz end # if ZF = 1 then jump to end
    lea rax, [r11 + 8 * rdi] # rax <- address of array element with index = rdi
    mov rax, [rax] # rax <- array
    inc rdi # rdi += 1
    cmp r10, rax # sub r10, rax, rflags update
    jg traverse_array # jump to traverse_array if r10 > rax

update_max:

    mov r10, rax
    jmp traverse_array

end:

    mov rdi, r10 # mov maximum array element to rdi
    call writei64
        call finish


.data

array: .quad -1, -2, 6, 8, 4, 0, 7, 5

.equ elem_size, 8

len: .quad ($ - array) / elem_size
