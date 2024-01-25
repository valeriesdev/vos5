EXTERN current_process_pid
EXTERN live_processes

GLOBAL yield
yield:
	push ebp
	push esp

	mov eax, cr3
	push eax
	
	push ebp
	pusha

	mov ebp, esp

	popa

	; ebp
	mov ebx, [current_process_pid]
	mov eax, [live_processes + ebx*4]
	add eax, 8
	pop ebx
	mov [eax], ebx

	pop ebx
	mov cr3, ebx
	;pop eip
	pop esp

	pop ebp
	pop eax  ; eax holds the 
	jmp eax