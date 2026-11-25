$FG,LTGREEN$// ============================================================================
//  02_holy_math_crypto.HC — Pure HolyC Number Theory, RSA, CRC32 & Primes
//  Implements modular exponentiation, Euclid GCD, RSA crack, Collatz, CRC32.
// ============================================================================$FG$

Class CRsaKeyPair {
  I64 p;
  I64 q;
  I64 n;
  I64 phi;
  I64 e;
  I64 d;
};

#exe {
  StreamPrint("I64 HOLY_RSA_E = 17;\n");
  StreamPrint("I64 HOLY_RSA_P = 257;\n");
  StreamPrint("I64 HOLY_RSA_Q = 263;\n");
  StreamPrint("I64 HOLY_FNV_PRIME = 16777619;\n");
}

I64 HolyModPow(I64 base, I64 exp, I64 mod) {
  I64 res = 1;
  I64 b = base % mod;
  I64 e = exp;
  while (e > 0) {
    if ((e % 2) == 1) {
      res = (res * b) % mod;
    }
    b = (b * b) % mod;
    e = e / 2;
  }
  return res;
}

I64 HolyGcd(I64 a, I64 b) {
  I64 x = a;
  I64 y = b;
  I64 t = 0;
  while (y != 0) {
    t = x % y;
    x = y;
    y = t;
  }
  return x;
}

I64 HolyModInverse(I64 a_in, I64 m_in) {
  I64 m0 = m_in;
  I64 a = a_in;
  I64 m = m_in;
  I64 x0 = 0;
  I64 x1 = 1;
  I64 q = 0;
  I64 t = 0;

  while (a > 1) {
    q = a / m;
    t = m;
    m = a % m;
    a = t;
    t = x0;
    x0 = x1 - q * x0;
    x1 = t;
  }
  if (x1 < 0) {
    x1 = x1 + m0;
  }
  return x1;
}

I64 HolyIsPrime(I64 n) {
  if (n < 2) {
    return 0;
  }
  if (2 <= n <= 3) {
    return 1;
  }
  if ((n % 2) == 0) {
    return 0;
  }
  I64 d = 3;
  while (d * d <= n) {
    if ((n % d) == 0) {
      return 0;
    }
    d = d + 2;
  }
  return 1;
}

I64 HolyCountPrimesUpTo(I64 limit) {
  I64 n = 2;
  I64 cnt = 0;
  while (n <= limit) {
    if (HolyIsPrime(n)) {
      cnt = cnt + 1;
    }
    n = n + 1;
  }
  return cnt;
}

I64 HolyCollatzSteps(I64 seed) {
  I64 x = seed;
  I64 steps = 0;
  while (x > 1 && steps < 256) {
    if ((x % 2) == 0) {
      x = x / 2;
    } else {
      x = 3 * x + 1;
    }
    steps = steps + 1;
  }
  return steps;
}

I64 HolyFibIter(I64 n) {
  if (n <= 1) {
    return n;
  }
  I64 a = 0;
  I64 b = 1;
  I64 i = 2;
  I64 nxt = 0;
  while (i <= n) {
    nxt = a + b;
    a = b;
    b = nxt;
    i = i + 1;
  }
  return b;
}

I64 HolyCrc32Step(I64 crc_in, U8 b) {
  I64 crc = crc_in ^ b;
  I64 bit = 0;
  while (bit < 8) {
    if ((crc % 2) != 0) {
      crc = (crc / 2) ^ 3988292384;
    } else {
      crc = crc / 2;
    }
    bit = bit + 1;
  }
  return crc;
}

I64 HolyFnvHashHello() {
  I64 msg[11];
  msg[0] = 72;  msg[1] = 101; msg[2] = 108; msg[3] = 108; msg[4] = 111;
  msg[5] = 32;  msg[6] = 119; msg[7] = 111; msg[8] = 114; msg[9] = 108; msg[10] = 100;

  I64 h = 2166136261;
  I64 i = 0;
  while (i < 11) {
    h = h ^ msg[i];
    h = (h * HOLY_FNV_PRIME) % 2147483647;
    i = i + 1;
  }
  return h;
}

I64 HolyRsaSelfTest() {
  CRsaKeyPair rsa;
  rsa.p = HOLY_RSA_P;
  rsa.q = HOLY_RSA_Q;
  rsa.n = rsa.p * rsa.q;
  rsa.phi = (rsa.p - 1) * (rsa.q - 1);
  rsa.e = HOLY_RSA_E;
  rsa.d = 0;

  // Deliberately factor rsa.n in HolyC and recover private key
  I64 cand = 3;
  I64 fp = 0;
  I64 fq = 0;
  while (cand * cand <= rsa.n) {
    if ((rsa.n % cand) == 0) {
      fp = cand;
      fq = rsa.n / cand;
      cand = rsa.n;
    }
    cand = cand + 2;
  }

  I64 recovered_phi = (fp - 1) * (fq - 1);
  rsa.d = HolyModInverse(rsa.e, recovered_phi);

  I64 plain[11];
  plain[0] = 72;  plain[1] = 101; plain[2] = 108; plain[3] = 108; plain[4] = 111;
  plain[5] = 32;  plain[6] = 119; plain[7] = 111; plain[8] = 114; plain[9] = 108; plain[10] = 100;

  I64 i = 0;
  I64 ok_count = 0;
  I64 cipher = 0;
  I64 dec = 0;
  while (i < 11) {
    cipher = HolyModPow(plain[i], rsa.e, rsa.n);
    dec = HolyModPow(cipher, rsa.d, rsa.n);
    if (dec == plain[i]) {
      ok_count = ok_count + 1;
    }
    i = i + 1;
  }
  return ok_count;
}

I64 HolyCryptoSelfTest() {
  "$FG,CYAN$[HolyC Crypto] Running HolyC RSA, Primality, Collatz & FNV1a...$FG$\n";
  I64 rsa_ok = HolyRsaSelfTest();
  I64 primes = HolyCountPrimesUpTo(200);
  I64 collatz = HolyCollatzSteps(27);
  I64 fib20 = HolyFibIter(20);
  I64 fnv = HolyFnvHashHello();
  I64 gcd_chk = HolyGcd(17, 67072);
  I64 crc = HolyCrc32Step(4294967295, 72);
  return rsa_ok + primes + collatz + fib20 + gcd_chk + (fnv % 1000) + (crc % 1000);
}
