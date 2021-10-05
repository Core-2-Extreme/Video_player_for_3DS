.data      
.align 4
.global memcpy_asm_4b
.global memcpy_asm
.type memcpy_asm, "function"
.type memcpy_asm_4b, "function"

.text
memcpy_asm:
    push { r4-r11 }
    mov r11, r0
    add r11, r2
    sub r11, #36

    cmp r0, r11
    cpy_loop:
    bgt cpy_end
    ldm r1, { r2-r10 }
    stm r0, { r2-r10 }
    add r0, #36
    add r1, #36
    cmp r0, r11
    b cpy_loop
    cpy_end:

    add r11, r11, #36

    cmp r0, r11
    cpy_loop_:
    bge cpy_end_
    ldrb r2, [r1], #1
    strb r2, [r0], #1
    cmp r0, r11
    b cpy_loop_
    cpy_end_:

    pop { r4-r11 }
    bx lr

memcpy_asm_4b:
    ldr r2, [r1]
    str r2, [r0]
    bx lr
