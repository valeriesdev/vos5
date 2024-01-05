[org 0x7c00]               ; Memory location is 0x7c00
KERNEL_OFFSET equ 0x1000   ; The kernel location is 0x7c00
 
    mov [BOOT_DRIVE], dl   ; The BIOS places the boot drive in DL; retrieve it
    mov bp, 0x9000         ; Place the stack at 0x9000
    mov sp, bp             ; ^
 
    call load_kernel       ; Read the kernel from disk
    call switch_to_pm      ; Switch to protected mode
    jmp $                  ; Never executed

%include "src/boot/print.asm"
%include "src/boot/print_hex.asm"
%include "src/boot/disk.asm"
%include "src/boot/gdt.asm"
%include "src/boot/switch_pm.asm"

[bits 16]                  ; This section is 16-bit
load_kernel:               ; Load the kernel from disk
    mov bx, KERNEL_OFFSET  ; Read from disk and store in 0x1000
    mov dh, 36             ; Load 32 sectors of kernel
    mov dl, [BOOT_DRIVE]   ; Select the boot drive
    call disk_load         ; Load from the disk
    ret                    ; Return
 
[bits 32]                  ; This section is 32-bit
BEGIN_PM:                  ; Here, we enter protected mode
    mov ebp, 0x8000000 ; 6. update the stack right at the top of the free space
    mov esp, ebp
    call KERNEL_OFFSET     ; Go to the kernel
    jmp $                  ; Loop if the kernel ever stops


BOOT_DRIVE db 0x81
MSG_REAL_MODE db "Started in 16-bit Real Mode", 0
MSG_PROT_MODE db "Landed in 32-bit Protected Mode", 0
MSG_LOAD_KERNEL db "Loading kernel into memory", 0
MSG_RETURNED_KERNEL db "Returned from kernel. Error?", 0


times 510 - ($-$$) db 0
dw 0xaa55