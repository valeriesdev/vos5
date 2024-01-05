print_hex:
    pusha
    mov cx, 0

; Convert DX to ASCII
hex_loop:
    cmp cx, 4 ; Loop 4 times, for each byte in DX
    je end
    
    ; Convert last char in DX to ASCII
    mov ax, dx ; Using AX as working register
    and ax, 0x000f ; mask all but last byte
    add al, 0x30
    cmp al, 0x39 ; if > 9, add extra 8 to represent 'A' to 'F'
    jle step2
    add al, 7

step2:
    ; Place ASCII char in char[]
    mov bx, HEX_OUT + 5
    sub bx, cx
    mov [bx], al
    ror dx, 4 ; Shift DX to complete next char

    add cx, 1
    jmp hex_loop

end:
    mov bx, HEX_OUT
    call print

    popa
    ret

HEX_OUT:
    db '0x0000',0 ; reserve memory for our new string
