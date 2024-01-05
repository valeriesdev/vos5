; Load 'dh' sectors from drive 'dl' into ES:BX
disk_load:
    pusha
    push dx

    mov si, 0
    mov es, si
    mov ah, 0x02
    mov al, dh
    mov cl, 0x02
    mov ch, 0x00
    mov dh, 0x00

    int 0x13
    jc disk_error

    pop dx
    popa
    ret

disk_error:
    mov bx, DISK_ERROR
    call print
    call print_nl
    mov dh, ah
    call print_hex ; error code at ahttp://stanislavs.org/helppc/int_13-1.html
    jmp disk_loop

disk_loop:
    jmp $

DISK_ERROR: db "Disk read error", 0