[BITS 64]

section .text

global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext, kHlt, kTestAndSet
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS
global kInPortWord, kOutPortWord

; Keyboard

kInPortByte:
    push rdx

    mov rdx, rdi
    mov rax, 0
    in al, dx

    pop rdx
    ret

kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, al

    pop rax
    pop rdx
    ret

; GDT, IDT

kLoadGDTR:
    lgdt [rdi]
    ret

kLoadTR:
    ltr di
    ret

kLoadIDTR:
    lidt [rdi]
    ret

; Interrupt

kEnableInterrupt:
    sti
    ret

kDisableInterrupt:
    cli
    ret

kReadRFLAGS:
    pushfq
    pop rax
    ret

kReadTSC:
    push rdx

    rdtsc
    shl rdx, 32
    or rax, rdx

    pop rdx
    ret

; Task

%macro KSAVECONTEXT 0
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov ax, ds
    push rax
    mov ax, es
    push rax
    push fs
    push gs
%endmacro

%macro KLOADCONTEXT 0
    pop gs
    pop fs
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
%endmacro

kSwitchContext:
    push rbp
    mov rbp, rsp

    pushfq
    cmp rdi, 0
    je .LoadContext
    popfq

    push rax

    mov ax, ss
    mov qword[rdi + (23 * 8)], rax
    mov rax, rbp
    add rax, 16

    mov rax, rbp
    add rax, 16
    mov qword[rdi + (22 * 8)], rax

    pushfq
    pop rax
    mov qword[rdi + (21 * 8)], rax

    mov ax, cs
    mov qword[rdi + (20 * 8)], rax

    mov rax, qword[rbp + 8]
    mov qword[rdi + (19 * 8)], rax

    pop rax
    pop rbp

    add rdi, (19 * 8)
    mov rsp, rdi
    sub rdi, (19 * 8)

    KSAVECONTEXT

.LoadContext:
    mov rsp, rsi

    KLOADCONTEXT
    iretq

kHlt:
    hlt
    hlt
    ret

kTestAndSet:
    mov rax, rsi
    lock cmpxchg byte [rdi], dl
    je .SUCCESS
.NOTSAME:
    mov rax, 0x00
    ret
.SUCCESS:
    mov rax, 0x01
    ret

; FPU

kInitializeFPU:
    finit
    ret

kSaveFPUContext:
    fxsave [rdi]
    ret

kLoadFPUContext:
    fxrstor [rdi]
    ret

kSetTS:
    push rax

    mov rax, cr0
    or rax, 0x08
    mov cr0, rax

    pop rax
    ret

kClearTS:
    clts
    ret

kInPortWord:
    push rdx

    mov rdx, rdi
    mov rax, 0
    in ax, dx

    pop rdx
    ret

kOutPortWord:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, ax

    pop rax
    pop rdx
    ret
