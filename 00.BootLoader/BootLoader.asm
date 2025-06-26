[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07c0:START

START:
    mov ax, 0x07c0
    mov ds, ax
    mov ax, 0xb800
    mov es, ax

    mov si, 0

.SCREENCLEARLOOP:
    mov byte [es: si], 0
    mov byte [es: si + 1], 0x0a

    add si, 2

    cmp si, 80 * 25 * 2

    jl .SCREENCLEARLOOP

    mov si, 0
    mov di, 0

.MESSAGELOOP:
    mov cl, byte [si + MESSAGE1]

    cmp cl, 0
    je .MESSAGEEND

    mov byte [es: di], cl

    add si, 1
    add di, 2

    jmp .MESSAGELOOP
.MESSAGEEND:

RESETDISK:
    mov ax, 0
    mov dl, 0
    int 0x13
    jc HANDLEDISKERROR

    mov si, 0x1000
    mov es, si

    mov bx, 0x0000

    mov di, word [TOTALSECTORCOUNT]

READDATA:
    cmp di, 0
    je READEND
    sub di, 0x1

    mov ah, 0x02
    mov al, 0x1
    mov ch, byte [TRACKNUMBER]
    mov cl, byte [SECTORNUMBER]
    mov dh, byte [HEADNUMBER]
    mov dl, 0x00
    int 0x13
    jc HANDLEDISKERROR

    add si, 0x0020
    mov es, si

    mov al, byte [SECTORNUMBER]
    add al, 0x01
    mov byte [SECTORNUMBER], al
    cmp al, 19
    jl READEND

    xor byte [HEADNUMBER], 0x01
    mov byte [SECTORNUMBER], 0x01

    cmp byte [HEADNUMBER], 0x00
    jne READDATA

    add byte [TRACKNUMBER], 0x01
    jmp READDATA
READEND:
;    push LOADINGCOMPLETEMESSAGE
;    push 1
;    push 20
;    call PRINTMESSAGE
;    add sp, 6

    jmp 0x1000:0x0000

HANDLEDISKERROR:
;    push DISKERRORMESSAGE
;    push 1
;    push 20
;    call PRINTMESSAGE

    jmp $

jmp $

MESSAGE1: db 'Boot Loader Start', 0;

TOTALSECTORCOUNT: dw 1024

SECTORNUMBER: db 0x02
HEADNUMBER: db 0x00
TRACKNUMBER: db 0x00

times 510 - ($ - $$) db 0x00

db 0x55
db 0xaa
