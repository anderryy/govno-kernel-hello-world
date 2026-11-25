$FG,LTGREEN$// ============================================================================
//  05_god_oracle_dict.HC — TempleOS Oracle LCG, 64-Word Lexicon & DolDoc Lore
//  Authentic TempleOS word selection, DolDoc streams & Morse weights in HolyC.
// ============================================================================$FG$

Class COraPsalm {
  I64 seed;
  I64 word_count;
  I64 checksum;
  U8  blessed;
};

#exe {
  StreamPrint("I64 ORACLE_LCG_MULT = 1103515245;\n");
  StreamPrint("I64 ORACLE_LCG_ADD  = 12345;\n");
  StreamPrint("I64 ORACLE_DICT_LEN = 64;\n");
}

I64 HolyNextRand(I64 state) {
  I64 next = (state * ORACLE_LCG_MULT + ORACLE_LCG_ADD) % 2147483647;
  if (next < 0) {
    next = -next;
  }
  return next;
}

U0 HolySpeakOracleWord(I64 idx) {
  I64 w = idx % ORACLE_DICT_LEN;
  if (w == 0)  {
    "TEMPLE ";
  }
  if (w == 1)  {
    "RING0 ";
  }
  if (w == 2)  {
    "ASSEMBLY ";
  }
  if (w == 3)  {
    "CHARIOT ";
  }
  if (w == 4)  {
    "LIGHTNING ";
  }
  if (w == 5)  {
    "DIVINE ";
  }
  if (w == 6)  {
    "COMPILER ";
  }
  if (w == 7)  {
    "ELEPHANT ";
  }
  if (w == 8)  {
    "PSALM ";
  }
  if (w == 9)  {
    "RAINBOW ";
  }
  if (w == 10) {
    "TRUMPET ";
  }
  if (w == 11) {
    "ORACLE ";
  }
  if (w == 12) {
    "BLESSED ";
  }
  if (w == 13) {
    "VGA16 ";
  }
  if (w == 14) {
    "KERNEL ";
  }
  if (w == 15) {
    "DOLDOC ";
  }
  if (w == 16) {
    "SERAPHIM ";
  }
  if (w == 17) {
    "PROPHET ";
  }
  if (w == 18) {
    "HYMN ";
  }
  if (w == 19) {
    "SCROLL ";
  }
  if (w == 20) {
    "MONOLITH ";
  }
  if (w == 21) {
    "CATHEDRAL ";
  }
  if (w == 22) {
    "DRAGON ";
  }
  if (w == 23) {
    "SHIELD ";
  }
  if (w == 24) {
    "FALCON ";
  }
  if (w == 25) {
    "CEDAR ";
  }
  if (w == 26) {
    "SOLOMON ";
  }
  if (w == 27) {
    "RIVER ";
  }
  if (w == 28) {
    "CROWN ";
  }
  if (w == 29) {
    "THUNDER ";
  }
  if (w == 30) {
    "SCEPTRE ";
  }
  if (w == 31) {
    "COVENANT ";
  }
  if (w == 32) {
    "TABERNACLE ";
  }
  if (w == 33) {
    "ARCHITECT ";
  }
  if (w == 34) {
    "BEHEMOTH ";
  }
  if (w == 35) {
    "LEVIATHAN ";
  }
  if (w == 36) {
    "CHALICE ";
  }
  if (w == 37) {
    "BANNER ";
  }
  if (w == 38) {
    "PILLAR ";
  }
  if (w == 39) {
    "FLAME ";
  }
  if (w == 40) {
    "SAPPHIRE ";
  }
  if (w == 41) {
    "EMERALD ";
  }
  if (w == 42) {
    "CITADEL ";
  }
  if (w == 43) {
    "VANGUARD ";
  }
  if (w == 44) {
    "SANCTUARY ";
  }
  if (w == 45) {
    "SENTINEL ";
  }
  if (w == 46) {
    "ANCHOR ";
  }
  if (w == 47) {
    "COMPASS ";
  }
  if (w == 48) {
    "HARBINGER ";
  }
  if (w == 49) {
    "PHOENIX ";
  }
  if (w == 50) {
    "SOLSTICE ";
  }
  if (w == 51) {
    "ECLIPSE ";
  }
  if (w == 52) {
    "ZENITH ";
  }
  if (w == 53) {
    "HORIZON ";
  }
  if (w == 54) {
    "PARAGON ";
  }
  if (w == 55) {
    "AEGIS ";
  }
  if (w == 56) {
    "RELIC ";
  }
  if (w == 57) {
    "CODEX ";
  }
  if (w == 58) {
    "CIPHER ";
  }
  if (w == 59) {
    "MATRIX ";
  }
  if (w == 60) {
    "PRISM ";
  }
  if (w == 61) {
    "VERTEX ";
  }
  if (w == 62) {
    "SPARROW ";
  }
  if (w == 63) {
    "GLORY ";
  }
}

I64 HolyMorseWeight(U8 ch) {
  if (ch == 72)  { return 4; }
  if (ch == 101) { return 1; }
  if (ch == 108) { return 4; }
  if (ch == 111) { return 3; }
  if (ch == 32)  { return 1; }
  if (ch == 119) { return 3; }
  if (ch == 114) { return 3; }
  if (ch == 100) { return 3; }
  return 5;
}

I64 HolyAStarManhattan(I64 x0, I64 y0, I64 x1, I64 y1) {
  I64 dx = x1 - x0;
  I64 dy = y1 - y0;
  if (dx < 0) { dx = -dx; }
  if (dy < 0) { dy = -dy; }
  return dx + dy;
}

I64 HolyVerifyDolDocTags() {
  "$FG,YELLOW$[DolDoc] TempleOS Hypertext Markup Verification:$FG$\n";
  "$FG,CYAN$  +------------------------------------------------+$FG$\n";
  "$FG,LTGREEN$  |  640x480 16-Color VGA  |  Flat Ring-0 Memory   |$FG$\n";
  "$FG,CYAN$  +------------------------------------------------+$FG$\n";
  return 16 + 640 + 480;
}

I64 HolyGodOracleRun() {
  COraPsalm ps;
  ps.seed = 7771337;
  ps.word_count = 8;
  ps.checksum = 0;
  ps.blessed = 1;

  "$FG,YELLOW$[HolyC Oracle] GOD SAYS: $FG$";
  I64 rng = ps.seed;
  I64 i = 0;
  while (i < ps.word_count) {
    rng = HolyNextRand(rng);
    I64 idx = (rng / 65536) % ORACLE_DICT_LEN;
    HolySpeakOracleWord(idx);
    ps.checksum = ps.checksum + idx * (i + 1);
    i = i + 1;
  }
  "\n";

  I64 w_sum = HolyMorseWeight(72) + HolyMorseWeight(101) + HolyMorseWeight(108) +
              HolyMorseWeight(111) + HolyMorseWeight(119) + HolyMorseWeight(114) +
              HolyMorseWeight(100);
  I64 dist = HolyAStarManhattan(1, 1, 23, 9);
  I64 dd = HolyVerifyDolDocTags();
  return ps.checksum + w_sum + dist + dd;
}
