__entry:
    mov rax, 1
    mov rdi, 1
    mov rsi, message
    mov rdx, 14
    syscall
    
    mov rax, 60
    xor rdi, rdi
    syscall

message: db 'Hello, World!', 10
