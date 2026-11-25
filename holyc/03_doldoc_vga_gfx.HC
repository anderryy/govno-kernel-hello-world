$FG,LTGREEN$// ============================================================================
//  03_doldoc_vga_gfx.HC — TempleOS 16-Color VGA, DolDoc & 3D LUT Module
//  Implements 16-color palette checks, fixed-point trig LUT & raster math.
// ============================================================================$FG$

Class CVgaColorEntry {
  I64 index;
  I64 r;
  I64 g;
  I64 b;
};

Class CVec3Fixed {
  I64 x;
  I64 y;
  I64 z;
};

Class CTriangle3D {
  CVec3Fixed v0;
  CVec3Fixed v1;
  CVec3Fixed v2;
  I64 color_idx;
};

#exe {
  StreamPrint("I64 VGA_SCREEN_W = 640;\n");
  StreamPrint("I64 VGA_SCREEN_H = 480;\n");
  StreamPrint("I64 VGA_COLORS   = 16;\n");
  StreamPrint("I64 TRIG_SCALE   = 1000;\n");
}

I64 HolyAbs(I64 v) {
  if (v < 0) {
    return -v;
  }
  return v;
}

I64 HolyClamp(I64 v, I64 lo, I64 hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

I64 HolySinApprox(I64 deg) {
  I64 d = deg % 360;
  if (d < 0) {
    d = d + 360;
  }
  I64 sign = 1;
  if (d >= 180) {
    d = d - 180;
    sign = -1;
  }
  // Bhaskara I sine approximation scaled by 1000
  I64 num = 4 * d * (180 - d) * 1000;
  I64 den = 40500 - d * (180 - d);
  if (den == 0) {
    return 0;
  }
  return sign * (num / den);
}

I64 HolyCosApprox(I64 deg) {
  return HolySinApprox(deg + 90);
}

I64 HolyBuild16ColorVgaPalette() {
  CVgaColorEntry pal[16];
  pal[0].index = 0;   pal[0].r = 0;    pal[0].g = 0;    pal[0].b = 0;
  pal[1].index = 1;   pal[1].r = 0;    pal[1].g = 0;    pal[1].b = 170;
  pal[2].index = 2;   pal[2].r = 0;    pal[2].g = 170;  pal[2].b = 0;
  pal[3].index = 3;   pal[3].r = 0;    pal[3].g = 170;  pal[3].b = 170;
  pal[4].index = 4;   pal[4].r = 170;  pal[4].g = 0;    pal[4].b = 0;
  pal[5].index = 5;   pal[5].r = 170;  pal[5].g = 0;    pal[5].b = 170;
  pal[6].index = 6;   pal[6].r = 170;  pal[6].g = 85;   pal[6].b = 0;
  pal[7].index = 7;   pal[7].r = 170;  pal[7].g = 170;  pal[7].b = 170;
  pal[8].index = 8;   pal[8].r = 85;   pal[8].g = 85;   pal[8].b = 85;
  pal[9].index = 9;   pal[9].r = 85;   pal[9].g = 85;   pal[9].b = 255;
  pal[10].index = 10; pal[10].r = 85;  pal[10].g = 255; pal[10].b = 85;
  pal[11].index = 11; pal[11].r = 85;  pal[11].g = 255; pal[11].b = 255;
  pal[12].index = 12; pal[12].r = 255; pal[12].g = 85;  pal[12].b = 85;
  pal[13].index = 13; pal[13].r = 255; pal[13].g = 85;  pal[13].b = 255;
  pal[14].index = 14; pal[14].r = 255; pal[14].g = 255; pal[14].b = 85;
  pal[15].index = 15; pal[15].r = 255; pal[15].g = 255; pal[15].b = 255;

  I64 i = 0;
  I64 sum = 0;
  while (i < VGA_COLORS) {
    if (0 <= pal[i].r <= 255) {
      sum = sum + pal[i].r + pal[i].g + pal[i].b;
    }
    i = i + 1;
  }
  return sum;
}

I64 HolyBresenhamSteps(I64 x0, I64 y0, I64 x1, I64 y1) {
  I64 dx = HolyAbs(x1 - x0);
  I64 dy = -HolyAbs(y1 - y0);
  I64 sx = (x0 < x1) ? 1 : -1;
  I64 sy = (y0 < y1) ? 1 : -1;
  I64 err = dx + dy;
  I64 e2 = 0;
  I64 steps = 0;
  I64 x = x0;
  I64 y = y0;

  while (steps < 2048) {
    steps = steps + 1;
    if (x == x1 && y == y1) {
      return steps;
    }
    e2 = 2 * err;
    if (e2 >= dy) {
      err = err + dy;
      x = x + sx;
    }
    if (e2 <= dx) {
      err = err + dx;
      y = y + sy;
    }
  }
  return steps;
}

I64 HolyMandelbrotFixed(I64 cx, I64 cy) {
  // Fixed point scaled by 1000
  I64 zx = 0;
  I64 zy = 0;
  I64 iter = 0;
  I64 nx = 0;
  I64 ny = 0;
  while (iter < 32) {
    if ((zx * zx + zy * zy) > 4000000) {
      return iter;
    }
    nx = (zx * zx - zy * zy) / 1000 + cx;
    ny = (2 * zx * zy) / 1000 + cy;
    zx = nx;
    zy = ny;
    iter = iter + 1;
  }
  return iter;
}

I64 HolyDot3Fixed(CVec3Fixed a, CVec3Fixed b) {
  return (a.x * b.x + a.y * b.y + a.z * b.z) / TRIG_SCALE;
}

I64 HolyRotate3DCubeChecksum() {
  CVec3Fixed cube[8];
  cube[0].x = -1000; cube[0].y = -1000; cube[0].z = -1000;
  cube[1].x = 1000;  cube[1].y = -1000; cube[1].z = -1000;
  cube[2].x = 1000;  cube[2].y = 1000;  cube[2].z = -1000;
  cube[3].x = -1000; cube[3].y = 1000;  cube[3].z = -1000;
  cube[4].x = -1000; cube[4].y = -1000; cube[4].z = 1000;
  cube[5].x = 1000;  cube[5].y = -1000; cube[5].z = 1000;
  cube[6].x = 1000;  cube[6].y = 1000;  cube[6].z = 1000;
  cube[7].x = -1000; cube[7].y = 1000;  cube[7].z = 1000;

  I64 s = HolySinApprox(45);
  I64 c = HolyCosApprox(45);
  I64 i = 0;
  I64 acc = 0;
  while (i < 8) {
    I64 rx = (cube[i].x * c + cube[i].z * s) / TRIG_SCALE;
    I64 rz = (-cube[i].x * s + cube[i].z * c) / TRIG_SCALE;
    acc = acc + HolyAbs(rx) + HolyAbs(rz);
    i = i + 1;
  }
  return acc;
}

I64 HolyGfxVerify() {
  "$FG,YELLOW$[HolyC VGA] Verifying 640x480 16-Color VGA, Trig LUT & Mandelbrot...$FG$\n";
  I64 pal_sum = HolyBuild16ColorVgaPalette();
  I64 line_steps = HolyBresenhamSteps(0, 0, VGA_SCREEN_W / 4, VGA_SCREEN_H / 4);
  I64 mandel = HolyMandelbrotFixed(-750, 100);
  I64 cube_chk = HolyRotate3DCubeChecksum();
  return pal_sum + line_steps + mandel + cube_chk;
}
