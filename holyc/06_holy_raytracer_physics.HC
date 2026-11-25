$FG,LTGREEN$// ============================================================================
//  06_holy_raytracer_physics.HC — Fixed-Point Raytracer, Rigid-Body & Matrices
//  Computes sphere intersections, 3x3 determinants & gravity bounces in HolyC.
// ============================================================================$FG$

Class CHolyRigidBody {
  I64 pos_x;
  I64 pos_y;
  I64 vel_x;
  I64 vel_y;
  I64 mass;
  I64 bounces;
};

Class CHolyMat3x3 {
  I64 m00; I64 m01; I64 m02;
  I64 m10; I64 m11; I64 m12;
  I64 m20; I64 m21; I64 m22;
};

Class CHolyRayHit {
  I64 hit;
  I64 dist_fixed;
  I64 shade;
};

#exe {
  StreamPrint("I64 HOLY_PHYS_GRAVITY = -98;\n");
  StreamPrint("I64 HOLY_PHYS_SCALE   = 1000;\n");
  StreamPrint("I64 HOLY_SPHERE_R2    = 360000;\n");
}

I64 HolyISqrt(I64 val) {
  if (val <= 0) {
    return 0;
  }
  I64 x = val;
  I64 y = (x + 1) / 2;
  I64 iter = 0;
  while (y < x && iter < 48) {
    x = y;
    y = (x + val / x) / 2;
    iter = iter + 1;
  }
  return x;
}

I64 HolyMat3Determinant(CHolyMat3x3 m) {
  I64 c0 = m.m00 * (m.m11 * m.m22 - m.m12 * m.m21);
  I64 c1 = m.m01 * (m.m10 * m.m22 - m.m12 * m.m20);
  I64 c2 = m.m02 * (m.m10 * m.m21 - m.m11 * m.m20);
  return c0 - c1 + c2;
}

I64 HolyValueNoiseHash(I64 x, I64 y) {
  I64 h = (x * 374761393 + y * 668265263) ^ 1442695041;
  if (h < 0) {
    h = -h;
  }
  h = (h * 1274126177) % 1000003;
  return h % 256;
}

CHolyRayHit HolyTraceRayAtSphere(I64 rdx, I64 rdy, I64 rdz) {
  CHolyRayHit out;
  out.hit = 0;
  out.dist_fixed = 0;
  out.shade = 0;

  // Camera at (0, 0, -2500), Sphere at (0, 0, 500) -> oc = (0, 0, -3000)
  I64 ocx = 0;
  I64 ocy = 0;
  I64 ocz = -3000;

  I64 b = (ocx * rdx + ocy * rdy + ocz * rdz) / HOLY_PHYS_SCALE;
  I64 c = (ocx * ocx + ocy * ocy + ocz * ocz) / HOLY_PHYS_SCALE - (HOLY_SPHERE_R2 / HOLY_PHYS_SCALE);
  I64 disc = b * b - c;

  if (disc > 0) {
    I64 sq = HolyISqrt(disc);
    I64 t = -b - sq;
    if (t > 10) {
      out.hit = 1;
      out.dist_fixed = t;
      out.shade = 100 + (sq % 155);
    }
  }
  return out;
}

I64 HolySimulateRigidBodyPhysics() {
  CHolyRigidBody body;
  body.pos_x = 0;
  body.pos_y = 5000;
  body.vel_x = 120;
  body.vel_y = 0;
  body.mass = 2;
  body.bounces = 0;

  I64 step = 0;
  I64 energy_sum = 0;
  while (step < 60) {
    body.vel_y = body.vel_y + HOLY_PHYS_GRAVITY;
    body.pos_x = body.pos_x + body.vel_x;
    body.pos_y = body.pos_y + body.vel_y;
    if (body.pos_y < 0) {
      body.pos_y = 0;
      body.vel_y = (-body.vel_y * 80) / 100;
      body.bounces = body.bounces + 1;
    }
    energy_sum = energy_sum + body.pos_y + body.bounces * 10;
    step = step + 1;
  }
  return energy_sum + body.bounces;
}

I64 HolyRaytracerAndPhysicsVerify() {
  "$FG,CYAN$[HolyC Physics/RT] Tracing 16x8 Rays & Simulating 60 Rigid-Body Steps...$FG$\n";
  CHolyMat3x3 mat;
  mat.m00 = 2; mat.m01 = 1; mat.m02 = 0;
  mat.m10 = 0; mat.m11 = 3; mat.m12 = 1;
  mat.m20 = 1; mat.m21 = 0; mat.m22 = 4;
  I64 det = HolyMat3Determinant(mat);

  I64 hits = 0;
  I64 shade_sum = 0;
  I64 py = 0;
  while (py < 8) {
    I64 px = 0;
    while (px < 16) {
      I64 rdx = (px - 8) * 120;
      I64 rdy = (py - 4) * 120;
      I64 rdz = 1000;
      CHolyRayHit rh = HolyTraceRayAtSphere(rdx, rdy, rdz);
      if (rh.hit) {
        hits = hits + 1;
        shade_sum = shade_sum + rh.shade;
      }
      px = px + 1;
    }
    py = py + 1;
  }

  I64 phys = HolySimulateRigidBodyPhysics();
  I64 noise = HolyValueNoiseHash(13, 37);
  return det + hits + shade_sum + (phys % 10000) + noise;
}
