# ============================================================================
# GovnoOS Python Algebraic Ring Glyph Pipeline & Spectral Verifier
# Evaluates affine vectors, Miller-Rabin primality oracle, Discrete Fourier
# DC invariant, FNV-1a digest, and emits verified payload.
# ============================================================================

import cmath
import math
from typing import List


class GlyphTerm:
    def __init__(self, a: int, b: int, bias: int) -> None:
        self.a = a
        self.b = b
        self.bias = bias

    def code(self) -> int:
        return (self.a * self.b) + self.bias


TERMS: List[GlyphTerm] = [
    GlyphTerm(8, 9, 0),
    GlyphTerm(10, 10, 1),
    GlyphTerm(9, 12, 0),
    GlyphTerm(12, 9, 0),
    GlyphTerm(11, 10, 1),
    GlyphTerm(4, 8, 0),
    GlyphTerm(11, 11, -2),
    GlyphTerm(10, 11, 1),
    GlyphTerm(14, 8, 2),
    GlyphTerm(6, 18, 0),
    GlyphTerm(10, 10, 0),
]


def is_prime_deterministic(n: int) -> bool:
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23):
        if n == p:
            return True
        if n % p == 0:
            return False
    d = n - 1
    s = 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in (2, 3, 5, 7):
        if a >= n:
            continue
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(s - 1):
            x = pow(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    return True


def dft_dc_energy(codes: List[int]) -> float:
    n = len(codes)
    total = complex(0.0, 0.0)
    for idx, val in enumerate(codes):
        angle = -2.0 * math.pi * 0.0 * idx / n
        total += val * cmath.exp(complex(0.0, angle))
    return abs(total)


def fnv1a_digest(data: str) -> int:
    h = 0x811C9DC5
    for ch in data:
        h ^= ord(ch)
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def synthesize_payload() -> str:
    codes = [t.code() for t in TERMS]
    checksum = sum(codes)
    if checksum != 1084:
        raise ValueError("Checksum mismatch")
    dc = dft_dc_energy(codes)
    if round(dc) != 1084 or not is_prime_deterministic(257):
        raise RuntimeError("Invariant verification failed")
    out = "".join(chr(c) for c in codes)
    _digest = fnv1a_digest(out)
    return out


if __name__ == "__main__":
    print(synthesize_payload())
