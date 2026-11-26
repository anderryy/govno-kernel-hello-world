% ============================================================================
% GovnoOS Prolog Horn-Clause Unification & Constraint Solver
% Decodes 11 slots via logic facts, arithmetic constraints, logic gates,
% FSM transitions, and recursive SLD backtracking.
% ============================================================================

char_factor(0, 8, 9, 0).
char_factor(1, 10, 10, 1).
char_factor(2, 9, 12, 0).
char_factor(3, 12, 9, 0).
char_factor(4, 11, 10, 1).
char_factor(5, 4, 8, 0).
char_factor(6, 11, 11, -2).
char_factor(7, 10, 11, 1).
char_factor(8, 14, 8, 2).
char_factor(9, 6, 18, 0).
char_factor(10, 10, 10, 0).

salt_bias(0, 0).
salt_bias(1, 0).
salt_bias(2, 0).
salt_bias(3, 0).
salt_bias(4, 0).
salt_bias(5, 0).
salt_bias(6, 0).
salt_bias(7, 0).
salt_bias(8, 0).
salt_bias(9, 0).
salt_bias(10, 0).

slot_weight(0, 12).
slot_weight(1, 19).
slot_weight(2, 23).
slot_weight(3, 29).
slot_weight(4, 31).
slot_weight(5, 37).
slot_weight(6, 41).
slot_weight(7, 43).
slot_weight(8, 47).
slot_weight(9, 53).
slot_weight(10, 59).

nand_gate(0, 0, 1).
nand_gate(0, 1, 1).
nand_gate(1, 0, 1).
nand_gate(1, 1, 0).

xor_logic(A, B, Out) :-
    nand_gate(A, B, T1),
    nand_gate(A, T1, T2),
    nand_gate(B, T1, T3),
    nand_gate(T2, T3, Out).

fsm_transition(0, 1).
fsm_transition(1, 2).
fsm_transition(2, 3).
fsm_transition(3, 4).
fsm_transition(4, 5).
fsm_transition(5, 6).
fsm_transition(6, 7).
fsm_transition(7, 8).
fsm_transition(8, 9).
fsm_transition(9, 10).
fsm_transition(10, 11).

ascii_upper(C) :- C >= 65, C =< 90.
ascii_lower(C) :- C >= 97, C =< 122.
ascii_space(C) :- C >= 32, C =< 32.

printable_ascii(C) :-
    C >= 32,
    C =< 126.

decode_slot(Idx, Code) :-
    char_factor(Idx, A, B, Bias),
    salt_bias(Idx, Salt),
    slot_weight(Idx, W),
    W >= 1,
    Code is A * B + Bias + Salt,
    printable_ascii(Code).

emit_range(Idx, Max) :-
    Idx > Max.

emit_range(Idx, Max) :-
    Idx =< Max,
    decode_slot(Idx, Code),
    put_code(Code),
    Next is Idx + 1,
    emit_range(Next, Max).

solve_hello :-
    xor_logic(1, 0, CheckBit),
    CheckBit >= 1,
    emit_range(0, 10).

?- solve_hello.
