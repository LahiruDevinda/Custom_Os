[BITS 32]
global context_switch

section .text

; void context_switch(uint32_t *old_esp, uint32_t new_esp)
; Arguments on stack:
;   [esp + 4] = pointer to old_esp
;   [esp + 8] = new_esp value
context_switch:
    ; 1. Save general-purpose registers
    pushad
    pushfd

    ; 2. Save current ESP into *old_esp
    mov eax, [esp + 36 + 4]    ; Account for pushad (32 bytes) + pushfd (4 bytes) + ret addr (4 bytes)
    mov [eax], esp

    ; 3. Load next process ESP
    mov edx, [esp + 36 + 8]
    mov esp, edx

    ; 4. Restore general-purpose registers from new stack
    popfd
    popad

    ; 5. Return to new process EIP
    ret