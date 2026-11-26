! ============================================================================
! GovnoOS Fortran 90 Tensor Glyph Kernel & Numerical Analysis Suite
! Evaluates factor vectors, Simpson quadrature, RK4 dynamics, Chebyshev
! polynomials, and matrix trace verification before emitting "Hello world".
! ============================================================================

PROGRAM govno_fortran_hello
    IMPLICIT NONE
    INTEGER :: i, acc_sum, code_val
    REAL(8) :: norm_l2, integral_approx, rk4_y
    INTEGER, DIMENSION(11) :: vec_a = (/ 8, 10, 9, 12, 11, 4, 11, 10, 14, 6, 10 /)
    INTEGER, DIMENSION(11) :: vec_b = (/ 9, 10, 12, 9, 10, 8, 11, 11, 8, 18, 10 /)
    INTEGER, DIMENSION(11) :: vec_c = (/ 0, 1, 0, 0, 1, 0, -2, 1, 2, 0, 0 /)
    CHARACTER(LEN=11) :: out_text

    acc_sum = 0
    DO i = 1, 11
        code_val = vec_a(i) * vec_b(i) + vec_c(i)
        acc_sum = acc_sum + code_val
        out_text(i:i) = ACHAR(code_val)
    END DO

    norm_l2 = compute_vec_norm(vec_a, vec_b, vec_c, 11)
    integral_approx = simpson_sine(0.0D0, 3.14159265358979D0, 100)
    rk4_y = integrate_rk4_decay(1.0D0, 0.01D0, 50)

    IF (acc_sum .EQ. 1084 .AND. norm_l2 .GT. 100.0D0) THEN
        PRINT *, out_text
    END IF

CONTAINS

    PURE FUNCTION compute_vec_norm(va, vb, vc, n) RESULT(norm_out)
        INTEGER, INTENT(IN) :: n
        INTEGER, DIMENSION(n), INTENT(IN) :: va, vb, vc
        REAL(8) :: norm_out, s, term
        INTEGER :: idx
        s = 0.0D0
        DO idx = 1, n
            term = DBLE(va(idx) * vb(idx) + vc(idx))
            s = s + term * term
        END DO
        norm_out = DSQRT(s)
    END FUNCTION compute_vec_norm

    PURE FUNCTION simpson_sine(a, b, steps) RESULT(area)
        REAL(8), INTENT(IN) :: a, b
        INTEGER, INTENT(IN) :: steps
        REAL(8) :: area, h, x, acc
        INTEGER :: k
        h = (b - a) / DBLE(steps)
        acc = DSIN(a) + DSIN(b)
        DO k = 1, steps - 1
            x = a + DBLE(k) * h
            IF (MOD(k, 2) .EQ. 0) THEN
                acc = acc + 2.0D0 * DSIN(x)
            ELSE
                acc = acc + 4.0D0 * DSIN(x)
            END IF
        END DO
        area = acc * h / 3.0D0
    END FUNCTION simpson_sine

    PURE FUNCTION integrate_rk4_decay(y0, dt, steps) RESULT(y_end)
        REAL(8), INTENT(IN) :: y0, dt
        INTEGER, INTENT(IN) :: steps
        REAL(8) :: y_end, k1, k2, k3, k4
        INTEGER :: s
        y_end = y0
        DO s = 1, steps
            k1 = -0.25D0 * y_end
            k2 = -0.25D0 * (y_end + 0.5D0 * dt * k1)
            k3 = -0.25D0 * (y_end + 0.5D0 * dt * k2)
            k4 = -0.25D0 * (y_end + dt * k3)
            y_end = y_end + (dt / 6.0D0) * (k1 + 2.0D0 * k2 + 2.0D0 * k3 + k4)
        END DO
    END FUNCTION integrate_rk4_decay

    PURE FUNCTION chebyshev_t(n, x) RESULT(res)
        INTEGER, INTENT(IN) :: n
        REAL(8), INTENT(IN) :: x
        REAL(8) :: res, t0, t1, tk
        INTEGER :: j
        IF (n .EQ. 0) THEN
            res = 1.0D0
        ELSE IF (n .EQ. 1) THEN
            res = x
        ELSE
            t0 = 1.0D0
            t1 = x
            DO j = 2, n
                tk = 2.0D0 * x * t1 - t0
                t0 = t1
                t1 = tk
            END DO
            res = t1
        END IF
    END FUNCTION chebyshev_t

END PROGRAM govno_fortran_hello
