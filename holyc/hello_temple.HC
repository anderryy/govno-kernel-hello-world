$FG,LTGREEN$// ============================================================================
//  hello_temple.HC — Authentic TempleOS HolyC Source Compiled BOTH:
//    1) AHEAD-OF-TIME by build/holyc2cpp.exe during build.bat -> C++ -> .exe!
//    2) AT RUNTIME inside the GovnoOS HolyC Bytecode VM!
// ============================================================================$FG$

Class CHolyConfig {
  I64 magic_power;
  I64 ring_level;
  U8  blessed;
};

#exe {
  StreamPrint("I64 HOLY_SHIFT = 2`3;\n");
  StreamPrint("I64 HOLY_CHECK = 10 + 3;\n");
  StreamPrint("U0 EmitAotHello() {\n");
  StreamPrint("  TempleEmitChar(64 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(93 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(100 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(100 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(103 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(24 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(111 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(103 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(106 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(100 + HOLY_SHIFT);\n");
  StreamPrint("  TempleEmitChar(92 + HOLY_SHIFT);\n");
  StreamPrint("}\n");
}

U0 BlessRingZero() {
  "$FG,YELLOW$[HolyC AOT] Ring-0 TempleOS Module Linked Into GovnoKernelHelloWorld.exe!$FG$\n";
}

I64 ValidatePrintableAscii(I64 ch) {
  // Authentic HolyC chained comparison!
  if (32 <= ch <= 126) {
    return 1;
  }
  return 0;
}

U0 TempleEmitChar(U8 ch) {
  if (ValidatePrintableAscii(ch)) {
    asm {
      MOV RAX, ch
      INT 0x80
    }
  }
}

U0 Main() {
  CHolyConfig cfg;
  cfg.magic_power = HOLY_SHIFT;
  cfg.ring_level = 0;
  cfg.blessed = 1;

  // HolyC 0-arg call without parentheses!
  BlessRingZero;
  EmitAotHello;
}
