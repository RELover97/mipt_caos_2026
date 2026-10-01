 .intel_syntax noprefix
    .global main

main:

    call readi64  # read 64-bit integer
    xor rdi, rdi  # xor <- 0
    test rax, 1   # (rax & 1) -> update eflags
    jnz is_odd    # if zf is 1 in eflags then jump to is_odd label
    jmp end       # jump to end

is_odd:

    mov rdi, 1 # rdi <- 1

end:

    call writei64 # display 64-bit integer
    call finish
