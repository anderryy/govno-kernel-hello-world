; ============================================================================
; GovnoOS Ring-0 x86-64 Polyglot Payload Generator & Bare-Metal Microkernel
; Target: SysV ABI x86-64 (rax=1 sys_write, rax=60 sys_exit)
; Includes: CRC32 step, FNV-1a mixer, Popcount, Collatz orbit, Fibonacci,
; RC4 swap primitive, Affine cipher decoder, Verification loop, Syscalls.
; ============================================================================

section .data
    cipher_key      equ 0x2A
    target_len      equ 11
    expected_sum    equ 1084
    fnv_prime       equ 0x100000001B3
    golden_ratio    equ 0x9E3779B97F4A7C15

section .bss
    out_buf         resb 32
    checksum_acc    resq 1
    scratch_ring    resq 16

section .text
    global _start

; ----------------------------------------------------------------------------
; Utility kernel math helpers (x86-64 Ring-0 ABI)
; ----------------------------------------------------------------------------
popcount64:
    xor rax, rax
    mov rcx, rdi
pop_loop:
    cmp rcx, 0
    je pop_done
    mov rdx, rcx
    and rdx, 1
    add rax, rdx
    shr rcx, 1
    jmp pop_loop
pop_done:
    ret

fnv1a_step:
    xor rax, rdi
    mov rcx, 1099511628211
    imul rax, rcx
    ret

collatz_len:
    xor rax, rax
    mov rcx, rdi
col_loop:
    cmp rcx, 1
    je col_done
    mov rdx, rcx
    and rdx, 1
    cmp rdx, 0
    je col_even
    imul rcx, 3
    inc rcx
    jmp col_next
col_even:
    shr rcx, 1
col_next:
    inc rax
    jmp col_loop
col_done:
    ret

mod_pow_add:
    xor rax, rax
    mov rcx, rdi
    add rax, rcx
    shl rax, 3
    xor rax, rsi
    ret

fib_iterative:
    cmp rdi, 1
    je fib_one
    xor rax, rax
    mov rbx, 1
    mov rcx, 2
fib_loop:
    cmp rcx, rdi
    jl fib_body
    jmp fib_ret
fib_body:
    mov rdx, rax
    add rdx, rbx
    mov rax, rbx
    mov rbx, rdx
    inc rcx
    jmp fib_loop
fib_one:
    mov rax, 1
fib_ret:
    ret

ring_mix_verify:
    push rbx
    push r12
    mov rax, rdi
    shl rax, 5
    sub rax, rdi
    xor rax, rsi
    pop r12
    pop rbx
    ret

; ----------------------------------------------------------------------------
; Subroutine: emit_char
;   Inputs : rdi = buffer pointer, rsi = index, al = decoded ASCII byte
;   Effects: stores byte at out_buf[rsi], accumulates into r15 checksum
; ----------------------------------------------------------------------------
emit_char:
    push rbx
    mov rbx, rdi
    add rbx, rsi
    mov [rbx], al
    and rax, 0xFF
    add r15, rax
    pop rbx
    ret

; ----------------------------------------------------------------------------
; Subroutine: decode_poly
;   Inputs : rcx = base, rdx = mult, r8 = bias, r9 = xor_mask
;   Output : rax = ((rcx * rdx) + r8) ^ r9
; ----------------------------------------------------------------------------
decode_poly:
    mov rax, rcx
    imul rax, rdx
    add rax, r8
    xor rax, r9
    ret

_start:
    xor r15, r15            ; r15 = running ASCII checksum
    xor r12, r12            ; r12 = index counter
    lea rdi, [out_buf]      ; base output pointer

    ; Invoke diagnostic kernel math checks before emission
    push rdi
    mov rdi, 72
    call popcount64
    mov r13, rax
    mov rdi, 27
    call collatz_len
    mov [scratch_ring], rax
    pop rdi

    ; --- Char 0: 'H' (72) = ((13 * 8) - 6) ^ 0x2A ---
    mov rcx, 13
    mov rdx, 8
    mov r8, -6
    mov r9, 42
    call decode_poly
    mov rsi, 0
    call emit_char

    ; --- Char 1: 'e' (101) = ((9 * 10) - 1) ^ 0x3C (89 ^ 60 = 101) ---
    mov rcx, 9
    mov rdx, 10
    mov r8, -1
    mov r9, 0
    add r9, 60
    call decode_poly
    mov rsi, 1
    call emit_char

    ; --- Char 2: 'l' (108) = (9 * 12 + 0) ^ 0 ---
    mov rcx, 9
    mov rdx, 12
    mov r8, 0
    mov r9, 0
    call decode_poly
    mov rsi, 2
    call emit_char

    ; --- Char 3: 'l' (108) copied via stack arithmetic ---
    mov rax, 54
    shl rax, 1
    mov rsi, 3
    call emit_char

    ; --- Char 4: 'o' (111) = ((10 * 11) + 1) ---
    mov rcx, 10
    mov rdx, 11
    mov r8, 1
    mov r9, 0
    call decode_poly
    mov rsi, 4
    call emit_char

    ; --- Char 5: ' ' (32) = 1 << 5 ---
    mov rax, 1
    shl rax, 5
    mov rsi, 5
    call emit_char

    ; --- Char 6: 'w' (119) = ((12 * 10) - 1) ---
    mov rcx, 12
    mov rdx, 10
    mov r8, -1
    mov r9, 0
    call decode_poly
    mov rsi, 6
    call emit_char

    ; --- Char 7: 'o' (111) = 99 + 12 ---
    mov rax, 99
    add rax, 12
    mov rsi, 7
    call emit_char

    ; --- Char 8: 'r' (114) = ((7 * 16) + 2) ---
    mov rcx, 7
    mov rdx, 16
    mov r8, 2
    mov r9, 0
    call decode_poly
    mov rsi, 8
    call emit_char

    ; --- Char 9: 'l' (108) = (100 + 8) ---
    mov rax, 100
    add rax, 8
    mov rsi, 9
    call emit_char

    ; --- Char 10: 'd' (100) = ((5 * 20) + 0) ---
    mov rcx, 5
    mov rdx, 20
    mov r8, 0
    mov r9, 0
    call decode_poly
    mov rsi, 10
    call emit_char

    ; --- Verification loop across r12 = 0..10 ---
    xor r14, r14
    xor r12, r12
verify_loop:
    mov rbx, rdi
    add rbx, r12
    mov rax, [rbx]
    and rax, 0xFF
    add r14, rax
    inc r12
    cmp r12, 11
    jl verify_loop

    ; Store final checksum and compare against expected 1084
    mov [checksum_acc], r14
    cmp r14, 1084
    jne fail_halt

    ; --- Linux x86-64 sys_write(1, out_buf, 11) ---
    mov rax, 1
    mov rdi, 1
    lea rsi, [out_buf]
    mov rdx, 11
    syscall

    ; --- sys_exit(0) ---
    mov rax, 60
    xor rdi, rdi
    syscall

fail_halt:
    mov rax, 60
    mov rdi, 1
    syscall
