

add:
    push ebp
    mov esp ebp
    
    mov r1 [ebp + 2]
    mov r2 [ebp + 1]
    add r3 r1 r2
    

    pop ebp
    
_start:
    push 2
    push 3
    call add