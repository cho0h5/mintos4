[ORG 0x00]
[BITS 16]

SECTION .text

START:
    mov ax, 0x1000

    mov ds, ax
    mov es, ax

    cli

    lgdt [GDTR]

    mov eax, 0x4000003b
    mov cr0, eax

    jmp dword 0x08: (PROTECTEDMODE - $$ + 0x10000)

[BITS 32]
PROTECTEDMODE:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    mov esp, 0xFFFE
    mov ebp, 0xFFFE

    push (SWITCHSUCCESSMESSAGE - $$ + 0x10000)
    push 2
    push 0
    call PRINTMESSAGE
    add esp, 12

    jmp $

PRINTMESSAGE:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ecx
    push edx

    mov eax, dword [ebp + 12]
    mov esi, 160
    mul esi
    mov edi, eax

    mov eax, dword [ebp + 8]
    mov esi, 2
    mul esi
    add edi, eax

    mov esi, dword [ebp + 16]

.MESSAGELOOP:
    mov cl, byte [esi]

    cmp cl, 0
    je .MESSAGELOOP

    mov byte [edi + 0xb8000], cl

    add esi, 1
    add edi, 2

    jmp .MESSAGELOOP

.MESSAGEEND:
    pop edx
    pop ecx
    pop eax
    pop edi
    pop esi
    pop ebp
    ret

align 8, db 0

dw 0x0000

GDTR:
    dw GDTEND - GDT - 1
    dd (GDT - $$ + 0x10000)

GDT:
    NULLDESCRIPTOR:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0x00
        db 0x00
        db 0x00

    CODEDESCRIPTOR:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0x9a
        db 0xcf
        db 0x00

    DATADESCRIPTOR:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0x92
        db 0xcf
        db 0x00
GDTEND:

SWITCHSUCCESSMESSAGE: db 'Switched to Protected Mode', 0

times 512 - ($ - $$) db 0x00
