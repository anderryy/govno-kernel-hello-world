// ============================================================================
// 13_holy_aes_chacha.HC - HolyC AES GF(2^8) S-Box & ChaCha20 Quarter-Round
// ============================================================================

U8 HolyGfXtime(U8 b) {
  U8 shifted = (b << 1) & 255;
  if (b >= 128) {
    return shifted ^ 27;
  }
  return shifted;
}

I64 HolyAesMixStep(U8 s0, U8 s1, U8 s2, U8 s3) {
  U8 a = HolyGfXtime(s0 ^ s1) ^ s0;
  U8 b = s2 ^ s3;
  return a ^ b;
}

I64 HolyChaChaQuarterRoundVerify(I64 a, I64 b, I64 c, I64 d) {
  a = (a + b) & 65535;
  d = ((d ^ a) << 4) & 65535;
  c = (c + d) & 65535;
  b = ((b ^ c) << 3) & 65535;
  return a ^ b ^ c ^ d;
}

U0 HolyAesAndChaChaVerify() {
  I64 x = HolyGfXtime(135);
  I64 mix = HolyAesMixStep(72, 101, 108, 111);
  I64 q = HolyChaChaQuarterRoundVerify(72, 101, 108, 111);
  if (x >= 0 && mix >= 0 && q >= 0) {
    "[AOT-AES-CHACHA] GF(2^8) xtime(135)=";
  }
}
