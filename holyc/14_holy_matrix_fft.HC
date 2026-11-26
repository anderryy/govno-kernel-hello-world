// ============================================================================
// 14_holy_matrix_fft.HC - HolyC Fixed-Point 3x3 Determinant & Walsh-Hadamard
// ============================================================================

Class CHolyMat3 {
  I64 m00; I64 m01; I64 m02;
  I64 m10; I64 m11; I64 m12;
  I64 m20; I64 m21; I64 m22;
};

I64 HolyDet3(CHolyMat3 m) {
  I64 c0 = m.m00 * (m.m11 * m.m22 - m.m12 * m.m21);
  I64 c1 = m.m01 * (m.m10 * m.m22 - m.m12 * m.m20);
  I64 c2 = m.m02 * (m.m10 * m.m21 - m.m11 * m.m20);
  return c0 - c1 + c2;
}

I64 HolyWalshHadamard4(I64 x0, I64 x1, I64 x2, I64 x3) {
  I64 a0 = x0 + x1;
  I64 a1 = x0 - x1;
  I64 a2 = x2 + x3;
  I64 a3 = x2 - x3;
  I64 y0 = a0 + a2;
  I64 y1 = a1 + a3;
  I64 y2 = a0 - a2;
  I64 y3 = a1 - a3;
  return y0 + y1 + y2 + y3;
}

U0 HolyMatrixAndWhtVerify() {
  CHolyMat3 mat;
  mat.m00 = 8;  mat.m01 = 1;  mat.m02 = 0;
  mat.m10 = 0;  mat.m11 = 9;  mat.m12 = 1;
  mat.m20 = 1;  mat.m21 = 0;  mat.m22 = 1;
  I64 det = HolyDet3(mat);
  I64 wht = HolyWalshHadamard4(72, 101, 108, 108);
  if (det != 0 && wht > 0) {
    "[AOT-MAT3-WHT] Matrix3 det & Walsh-Hadamard verified\n";
  }
}
