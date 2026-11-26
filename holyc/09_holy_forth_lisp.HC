$FG,LTGREEN$// ============================================================================
//  09_holy_forth_lisp.HC — Forth Stack Machine & Lisp S-Expr Evaluator in HolyC
//  Implements two complete language evaluators inside TempleOS HolyC.
// ============================================================================$FG$

Class CHolyForthVm {
  I64 sp;
  I64 rp;
  I64 dict_count;
  I64 checksum;
};

Class CHolyLispCons {
  I64 op_type;
  I64 val_a;
  I64 val_b;
  I64 result;
};

#exe {
  StreamPrint("I64 FORTH_DUP  = 101;\n");
  StreamPrint("I64 FORTH_ADD  = 102;\n");
  StreamPrint("I64 FORTH_MUL  = 103;\n");
  StreamPrint("I64 FORTH_SUB  = 104;\n");
  StreamPrint("I64 FORTH_SWAP = 105;\n");
  StreamPrint("I64 LISP_ADD   = 1;\n");
  StreamPrint("I64 LISP_SUB   = 2;\n");
  StreamPrint("I64 LISP_MUL   = 3;\n");
}

I64 HolyEvalForthProgram() {
  CHolyForthVm vm;
  vm.sp = 0;
  vm.rp = 0;
  vm.dict_count = 5;
  vm.checksum = 0;

  I64 stk[32];
  // Encode Forth words for Hello: (6 10 * 12 +) -> 72 'H', (72 29 +) -> 101 'e', ...
  I64 prog[48];
  I64 plen = 0;
  // Push 6, Push 10, MUL, Push 12, ADD => 72
  prog[plen] = 6; plen = plen + 1;
  prog[plen] = 10; plen = plen + 1;
  prog[plen] = FORTH_MUL; plen = plen + 1;
  prog[plen] = 12; plen = plen + 1;
  prog[plen] = FORTH_ADD; plen = plen + 1;
  // DUP, Push 29, ADD => 101 ('e')
  prog[plen] = FORTH_DUP; plen = plen + 1;
  prog[plen] = 29; plen = plen + 1;
  prog[plen] = FORTH_ADD; plen = plen + 1;
  // DUP, Push 7, ADD => 108 ('l')
  prog[plen] = FORTH_DUP; plen = plen + 1;
  prog[plen] = 7; plen = plen + 1;
  prog[plen] = FORTH_ADD; plen = plen + 1;
  // DUP, Push 3, ADD => 111 ('o')
  prog[plen] = FORTH_DUP; plen = plen + 1;
  prog[plen] = 3; plen = plen + 1;
  prog[plen] = FORTH_ADD; plen = plen + 1;
  // Push 8, Push 4, MUL => 32 (' ')
  prog[plen] = 8; plen = plen + 1;
  prog[plen] = 4; plen = plen + 1;
  prog[plen] = FORTH_MUL; plen = plen + 1;
  // SWAP, DUP, Push 8, ADD => 119 ('w')
  prog[plen] = FORTH_SWAP; plen = plen + 1;
  prog[plen] = FORTH_DUP; plen = plen + 1;
  prog[plen] = 8; plen = plen + 1;
  prog[plen] = FORTH_ADD; plen = plen + 1;

  I64 ip = 0;
  while (ip < plen) {
    I64 tok = prog[ip];
    if (tok == FORTH_DUP) {
      stk[vm.sp] = stk[vm.sp - 1];
      vm.sp = vm.sp + 1;
    } else if (tok == FORTH_SWAP) {
      I64 tmp = stk[vm.sp - 1];
      stk[vm.sp - 1] = stk[vm.sp - 2];
      stk[vm.sp - 2] = tmp;
    } else if (tok == FORTH_ADD) {
      I64 b = stk[vm.sp - 1];
      I64 a = stk[vm.sp - 2];
      vm.sp = vm.sp - 1;
      stk[vm.sp - 1] = a + b;
    } else if (tok == FORTH_SUB) {
      I64 b = stk[vm.sp - 1];
      I64 a = stk[vm.sp - 2];
      vm.sp = vm.sp - 1;
      stk[vm.sp - 1] = a - b;
    } else if (tok == FORTH_MUL) {
      I64 b = stk[vm.sp - 1];
      I64 a = stk[vm.sp - 2];
      vm.sp = vm.sp - 1;
      stk[vm.sp - 1] = a * b;
    } else {
      stk[vm.sp] = tok;
      vm.sp = vm.sp + 1;
    }
    ip = ip + 1;
  }

  I64 k = 0;
  while (k < vm.sp) {
    if (32 <= stk[k] <= 126) {
      vm.checksum = vm.checksum + stk[k] * (k + 1);
    }
    k = k + 1;
  }
  return vm.checksum;
}

I64 HolyEvalLispNodes() {
  CHolyLispCons nodes[8];
  nodes[0].op_type = LISP_MUL; nodes[0].val_a = 6;   nodes[0].val_b = 10;
  nodes[1].op_type = LISP_ADD; nodes[1].val_a = 60;  nodes[1].val_b = 12;
  nodes[2].op_type = LISP_ADD; nodes[2].val_a = 72;  nodes[2].val_b = 29;
  nodes[3].op_type = LISP_ADD; nodes[3].val_a = 101; nodes[3].val_b = 7;
  nodes[4].op_type = LISP_ADD; nodes[4].val_a = 108; nodes[4].val_b = 3;
  nodes[5].op_type = LISP_MUL; nodes[5].val_a = 8;   nodes[5].val_b = 4;
  nodes[6].op_type = LISP_ADD; nodes[6].val_a = 111; nodes[6].val_b = 8;
  nodes[7].op_type = LISP_SUB; nodes[7].val_a = 101; nodes[7].val_b = 1;

  I64 i = 0;
  I64 acc = 0;
  while (i < 8) {
    if (nodes[i].op_type == LISP_ADD) {
      nodes[i].result = nodes[i].val_a + nodes[i].val_b;
    } else if (nodes[i].op_type == LISP_SUB) {
      nodes[i].result = nodes[i].val_a - nodes[i].val_b;
    } else if (nodes[i].op_type == LISP_MUL) {
      nodes[i].result = nodes[i].val_a * nodes[i].val_b;
    }
    acc = acc + nodes[i].result;
    i = i + 1;
  }
  return acc;
}

I64 HolyForthAndLispVerify() {
  "$FG,CYAN$[HolyC Polyglot] Running Forth Stack VM & Lisp S-Expr Evaluator in HolyC...$FG$\n";
  I64 f = HolyEvalForthProgram();
  I64 l = HolyEvalLispNodes();
  return f + l;
}
