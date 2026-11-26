\ ============================================================================
\ GovnoOS Polyglot Stack Forth Dictionary (ANS-Forth Compatible)
\ Computes and prints "Hello world" via word definitions, return-stack loops,
\ stack permutations, number-theory primitives, and ASCII telemetry.
\ ============================================================================

32 CONSTANT ASCII_SPACE
72 CONSTANT GLYPH_H
100 CONSTANT GLYPH_D
1084 CONSTANT EXPECTED_ASCII_SUM
65536 CONSTANT KERNEL_PAGE_LIMIT

VARIABLE CHECKSUM
VARIABLE RING_ACC
VARIABLE CRC_REG
VARIABLE CYCLE_COUNT

0 CHECKSUM !
0 RING_ACC !
0 CRC_REG !
0 CYCLE_COUNT !

: SQUARE ( n -- n^2 )
    DUP *
;

: CUBE ( n -- n^3 )
    DUP SQUARE *
;

: DOUBLE_STEP ( n -- 2n )
    DUP +
;

: TRIPLE_STEP ( n -- 3n )
    DUP DUP + +
;

: AVG2 ( a b -- avg )
    + 2 /
;

: CLAMP_BYTE ( n -- byte )
    255 SWAP DROP 0 MAX_ZERO
;

: MAX_ZERO ( a b -- max )
    OVER OVER - DROP SWAP DROP
;

: XOR_STEP ( acc x -- acc' )
    XOR 31 * +
;

: INC_CYCLE ( -- )
    CYCLE_COUNT @ 1 + CYCLE_COUNT !
;

: POLY_HASH_BYTE ( byte -- )
    CRC_REG @ 31 * + CRC_REG !
    INC_CYCLE
;

: ADD_CHK ( c -- c )
    DUP CHECKSUM @ + CHECKSUM !
    DUP POLY_HASH_BYTE
;

: EMIT_TRACKED ( c -- )
    ADD_CHK
    EMIT
;

: COMPUTE_E ( -- c )
    5 20 * 1 +
;

: COMPUTE_L ( -- c )
    9 12 *
;

: COMPUTE_O ( -- c )
    COMPUTE_L 3 +
;

: EMIT_HELLO_PREFIX ( -- )
    GLYPH_H EMIT_TRACKED
    COMPUTE_E EMIT_TRACKED
    COMPUTE_L DUP EMIT_TRACKED
    EMIT_TRACKED
    COMPUTE_O EMIT_TRACKED
;

: COMPUTE_W ( -- c )
    11 11 * 2 -
;

: COMPUTE_R ( -- c )
    COMPUTE_O 3 +
;

: EMIT_WORLD_SUFFIX ( -- )
    ASCII_SPACE EMIT_TRACKED
    COMPUTE_W EMIT_TRACKED
    COMPUTE_O EMIT_TRACKED
    COMPUTE_R EMIT_TRACKED
    COMPUTE_L EMIT_TRACKED
    GLYPH_D EMIT_TRACKED
;

: VERIFY_SUM_LOOP ( -- sum )
    0
    11 0 DO
        I +
    LOOP
    DROP
    CHECKSUM @
;

: ROTATE_STACK_DEMO ( a b c -- b c a )
    ROT
;

: TRIANGULAR_SUM ( -- 55 )
    0
    11 0 DO
        I +
    LOOP
;

: FACTORIAL_SMALL ( -- 120 )
    1
    6 1 DO
        I *
    LOOP
;

: MAIN_FORTH ( -- )
    0 CHECKSUM !
    0 CRC_REG !
    0 CYCLE_COUNT !
    TRIANGULAR_SUM DROP
    FACTORIAL_SMALL DROP
    EMIT_HELLO_PREFIX
    EMIT_WORLD_SUFFIX
    VERIFY_SUM_LOOP
    DROP
;

MAIN_FORTH
