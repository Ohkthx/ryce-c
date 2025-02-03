#if defined(RYCE_IMPL) && !defined(RYCE_SIMPLEX_IMPL)
#define RYCE_SIMPLEX_IMPL
#endif
#ifndef RYCE_SIMPLEX_H
/*
    RyCE simplex - A single-header, STB-styled OpenSimplex2S implementation.
    This is just a reimplementation in C of the Java version created by Kurt Spencer.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_SIMPLEX_IMPL
       #include "simplex.h"

    2) In as many other files as you need, just #include "simplex.h"
       WITHOUT defining RYCE_SIMPLEX_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_SIMPLEX_H

#include <stdint.h>

// ---------------------------------------------------------------------//
// BEGIN VISIBILITY MACROS
#ifndef RYCE_PUBLIC_DECL
#define RYCE_PUBLIC_DECL extern
#endif // RYCE_PUBLIC

#ifndef RYCE_PUBLIC
#define RYCE_PUBLIC
#endif // RYCE_PUBLIC

#ifndef RYCE_PRIVATE
#if defined(__GNUC__) || defined(__clang__)
#define RYCE_PRIVATE __attribute__((unused)) static
#else
#define RYCE_PRIVATE static
#endif
#endif // RYCE_PRIVATE

#ifndef RYCE_UNUSED
#define RYCE_UNUSED(x) (void)(x)
#endif // RYCE_UNUSED
// END VISIBILITY MACROS
// ---------------------------------------------------------------------//

typedef double float64_t;

/*
    Public API Functions
*/

/**
 * @brief Generates a 2D OpenSimplex2S noise value.
 *
 * @param seed Seed value.
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @return float64_t Noise value.
 */
RYCE_PUBLIC_DECL float64_t ryce_simplex_noise2(int64_t seed, float64_t x, float64_t y);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_SIMPLEX_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_SIMPLEX_IMPL

const int64_t RYCE_SIMPLEX_PRIME_X = 0x5205402B9270C86F;
const int64_t RYCE_SIMPLEX_PRIME_Y = 0x598CD327003817B5;
const int64_t RYCE_SIMPLEX_HASH_MULTIPLIER = 0x53A3F72DEEC546F5;
const float64_t RYCE_SIMPLEX_SKEW_2D = 0.366025403784439;
const float64_t RYCE_SIMPLEX_UNSKEW_2D = -0.21132486540518713;
const int32_t RYCE_SIMPLEX_N_GRADS_2D_EXPONENT = 7;
const int32_t RYCE_SIMPLEX_N_GRADS_2D = 1 << RYCE_SIMPLEX_N_GRADS_2D_EXPONENT;
const float64_t RYCE_SIMPLEX_NORMALIZER_2D = 0.05481866495625118;
const float64_t RYCE_SIMPLEX_RSQUARED_2D = 2.0 / 3.0;

// NOLINTBEGIN
RYCE_PRIVATE const float64_t RYCE_SIMPLEX_GRAD2_SRC[] = {
    0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D, 0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D, 0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D, -0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D, -0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D, -0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D, -0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D, 0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.38268343236509 / RYCE_SIMPLEX_NORMALIZER_2D, 0.923879532511287 / RYCE_SIMPLEX_NORMALIZER_2D,
    //-------------------------------------//
    0.130526192220052 / RYCE_SIMPLEX_NORMALIZER_2D, 0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D, 0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D, 0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D, 0.130526192220051 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D, -0.130526192220051 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D, -0.60876142900872 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D, -0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D,
    0.130526192220052 / RYCE_SIMPLEX_NORMALIZER_2D, -0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.130526192220052 / RYCE_SIMPLEX_NORMALIZER_2D, -0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D, -0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D, -0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D, -0.130526192220052 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D, 0.130526192220051 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D, 0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.608761429008721 / RYCE_SIMPLEX_NORMALIZER_2D, 0.793353340291235 / RYCE_SIMPLEX_NORMALIZER_2D,
    -0.130526192220052 / RYCE_SIMPLEX_NORMALIZER_2D, 0.99144486137381 / RYCE_SIMPLEX_NORMALIZER_2D};
// NOLINTEND

#ifndef RYCE_MATH_FAST_FLOOR
#define RYCE_MATH_FAST_FLOOR
RYCE_PUBLIC inline int32_t ryce_math_fast_floor(float64_t x) {
    int32_t xi = (int32_t)x;
    if (x < xi) {
        return xi - 1;
    } else {
        return xi;
    }
}
#endif // RYCE_MATH_FAST_FLOOR

RYCE_PRIVATE inline int64_t ryce_i64_add_wrap_internal(int64_t x, int64_t y) {
    uint64_t ux = (uint64_t)x;
    uint64_t uy = (uint64_t)y;
    return (int64_t)((ux + uy) & UINT64_C(0xFFFFFFFFFFFFFFFF));
}

RYCE_PRIVATE inline int64_t ryce_i64_mul_wrap_internal(int64_t x, int64_t y) {
    uint64_t ux = (uint64_t)x;
    uint64_t uy = (uint64_t)y;
    return (int64_t)((ux * uy) & UINT64_C(0xFFFFFFFFFFFFFFFF));
}

RYCE_PRIVATE inline int64_t ryce_safe_hash(int64_t hash) {
    return (hash ^ (hash >> (64 - RYCE_SIMPLEX_N_GRADS_2D_EXPONENT + 1))) & UINT64_C(0xFFFFFFFFFFFFFFFF);
}

RYCE_PRIVATE inline float64_t ryce_simplex_grad2_internal(int64_t seed, int64_t x, int64_t y, float64_t dx,
                                                          float64_t dy) {
    int64_t hash = seed ^ x ^ y;
    hash = ryce_safe_hash(ryce_i64_mul_wrap_internal(hash, RYCE_SIMPLEX_HASH_MULTIPLIER));
    int32_t gi = (int32_t)(hash & ((sizeof(RYCE_SIMPLEX_GRAD2_SRC) / sizeof(RYCE_SIMPLEX_GRAD2_SRC[0])) - 1) & ~1);
    return (RYCE_SIMPLEX_GRAD2_SRC[gi] * dx) + (RYCE_SIMPLEX_GRAD2_SRC[gi + 1] * dy);
}

RYCE_PRIVATE float64_t ryce_simplex_noise2_unskewed_base_internal(int64_t seed, float64_t xs, float64_t ys) {
    // Get base points and offsets.
    int32_t xb = ryce_math_fast_floor(xs);
    int32_t yb = ryce_math_fast_floor(ys);
    float64_t xi = xs - (float64_t)xb;
    float64_t yi = ys - (float64_t)yb;

    // Prime pre-multiplication for hash.
    int64_t xbp = ryce_i64_mul_wrap_internal((int64_t)xb, RYCE_SIMPLEX_PRIME_X);
    int64_t ybp = ryce_i64_mul_wrap_internal((int64_t)yb, RYCE_SIMPLEX_PRIME_Y);

    // Unskew.
    float64_t t = (xi + yi) * RYCE_SIMPLEX_UNSKEW_2D;
    float64_t dx0 = xi + t;
    float64_t dy0 = yi + t;

    // First vertex.
    float64_t a0 = RYCE_SIMPLEX_RSQUARED_2D - (dx0 * dx0) - (dy0 * dy0);
    float64_t value = (a0 * a0) * (a0 * a0) * ryce_simplex_grad2_internal(seed, xbp, ybp, dx0, dy0);

    // Second vertex.
    float64_t a1 = ((2.0 * (1.0 + 2.0 * RYCE_SIMPLEX_UNSKEW_2D) * (1.0 / RYCE_SIMPLEX_UNSKEW_2D + 2.0)) * t) +
                   ((-2.0 * (1.0 + 2.0 * RYCE_SIMPLEX_UNSKEW_2D) * (1.0 + 2.0 * RYCE_SIMPLEX_UNSKEW_2D)) + a0);
    float64_t dx1 = dx0 - (1.0 + (2.0 * RYCE_SIMPLEX_UNSKEW_2D));
    float64_t dy1 = dy0 - (1.0 + (2.0 * RYCE_SIMPLEX_UNSKEW_2D));
    value += (a1 * a1) * (a1 * a1) *
             ryce_simplex_grad2_internal(seed, ryce_i64_add_wrap_internal(xbp, RYCE_SIMPLEX_PRIME_X),
                                         ryce_i64_add_wrap_internal(ybp, RYCE_SIMPLEX_PRIME_Y), dx1, dy1);

    // Third and fourth vertices.
    float64_t xmyi = xi - yi;
    if (t < RYCE_SIMPLEX_UNSKEW_2D) {
        if (xi + xmyi > 1.0) {
            float64_t dx2 = dx0 - ((3.0 * RYCE_SIMPLEX_UNSKEW_2D) + 2.0);
            float64_t dy2 = dy0 - ((3.0 * RYCE_SIMPLEX_UNSKEW_2D) + 1.0);
            float64_t a2 = RYCE_SIMPLEX_RSQUARED_2D - (dx2 * dx2) - (dy2 * dy2);
            if (a2 > 0.0) {
                value += (a2 * a2) * (a2 * a2) *
                         ryce_simplex_grad2_internal(
                             seed, ryce_i64_add_wrap_internal(xbp, (int64_t)(((uint64_t)RYCE_SIMPLEX_PRIME_X) << 1)),
                             ryce_i64_add_wrap_internal(ybp, RYCE_SIMPLEX_PRIME_Y), dx2, dy2);
            }
        } else {
            float64_t dx2 = dx0 - RYCE_SIMPLEX_UNSKEW_2D;
            float64_t dy2 = dy0 - (RYCE_SIMPLEX_UNSKEW_2D + 1.0);
            float64_t a2 = RYCE_SIMPLEX_RSQUARED_2D - (dx2 * dx2) - (dy2 * dy2);
            if (a2 > 0.0) {
                value += (a2 * a2) * (a2 * a2) *
                         ryce_simplex_grad2_internal(seed, xbp, ryce_i64_add_wrap_internal(ybp, RYCE_SIMPLEX_PRIME_Y),
                                                     dx2, dy2);
            }
        }

        if (yi - xmyi > 1.0) {
            float64_t dx3 = dx0 - ((3.0 * RYCE_SIMPLEX_UNSKEW_2D) + 1.0);
            float64_t dy3 = dy0 - ((3.0 * RYCE_SIMPLEX_UNSKEW_2D) + 2.0);
            float64_t a3 = RYCE_SIMPLEX_RSQUARED_2D - (dx3 * dx3) - (dy3 * dy3);
            if (a3 > 0.0) {
                value +=
                    (a3 * a3) * (a3 * a3) *
                    ryce_simplex_grad2_internal(
                        seed, ryce_i64_add_wrap_internal(xbp, RYCE_SIMPLEX_PRIME_X),
                        ryce_i64_add_wrap_internal(ybp, (int64_t)(((uint64_t)RYCE_SIMPLEX_PRIME_Y) << 1)), dx3, dy3);
            }
        } else {
            float64_t dx3 = dx0 - (RYCE_SIMPLEX_UNSKEW_2D + 1.0);
            float64_t dy3 = dy0 - RYCE_SIMPLEX_UNSKEW_2D;
            float64_t a3 = RYCE_SIMPLEX_RSQUARED_2D - (dx3 * dx3) - (dy3 * dy3);
            if (a3 > 0.0) {
                value += (a3 * a3) * (a3 * a3) *
                         ryce_simplex_grad2_internal(seed, ryce_i64_add_wrap_internal(xbp, RYCE_SIMPLEX_PRIME_X), ybp,
                                                     dx3, dy3);
            }
        }
    } else {
        if (xi + xmyi < 0.0) {
            float64_t dx2 = dx0 + (RYCE_SIMPLEX_UNSKEW_2D + 1.0);
            float64_t dy2 = dy0 + RYCE_SIMPLEX_UNSKEW_2D;
            float64_t a2 = RYCE_SIMPLEX_RSQUARED_2D - (dx2 * dx2) - (dy2 * dy2);
            if (a2 > 0.0) {
                value += (a2 * a2) * (a2 * a2) *
                         ryce_simplex_grad2_internal(seed, ryce_i64_add_wrap_internal(xbp, -RYCE_SIMPLEX_PRIME_X), ybp,
                                                     dx2, dy2);
            }

        } else {
            float64_t dx2 = dx0 - (RYCE_SIMPLEX_UNSKEW_2D + 1.0);
            float64_t dy2 = dy0 - RYCE_SIMPLEX_UNSKEW_2D;
            float64_t a2 = RYCE_SIMPLEX_RSQUARED_2D - (dx2 * dx2) - (dy2 * dy2);
            if (a2 > 0.0) {
                value += (a2 * a2) * (a2 * a2) *
                         ryce_simplex_grad2_internal(seed, ryce_i64_add_wrap_internal(xbp, RYCE_SIMPLEX_PRIME_X), ybp,
                                                     dx2, dy2);
            }
        }

        if (yi < xmyi) {
            float64_t dx2 = dx0 + RYCE_SIMPLEX_UNSKEW_2D;
            float64_t dy2 = dy0 + (RYCE_SIMPLEX_UNSKEW_2D + 1.0);
            float64_t a2 = RYCE_SIMPLEX_RSQUARED_2D - (dx2 * dx2) - (dy2 * dy2);
            if (a2 > 0.0) {
                value += (a2 * a2) * (a2 * a2) *
                         ryce_simplex_grad2_internal(seed, xbp, ryce_i64_add_wrap_internal(ybp, -RYCE_SIMPLEX_PRIME_Y),
                                                     dx2, dy2);
            }

        } else {
            float64_t dx2 = dx0 - RYCE_SIMPLEX_UNSKEW_2D;
            float64_t dy2 = dy0 - (RYCE_SIMPLEX_UNSKEW_2D + 1.0);
            float64_t a2 = RYCE_SIMPLEX_RSQUARED_2D - (dx2 * dx2) - (dy2 * dy2);
            if (a2 > 0.0) {
                value += (a2 * a2) * (a2 * a2) *
                         ryce_simplex_grad2_internal(seed, xbp, ryce_i64_add_wrap_internal(ybp, RYCE_SIMPLEX_PRIME_Y),
                                                     dx2, dy2);
            }
        }
    }

    return value;
}

RYCE_PUBLIC float64_t ryce_simplex_noise2(const int64_t seed, const float64_t x, const float64_t y) {
    float64_t s = RYCE_SIMPLEX_SKEW_2D * (x + y);
    float64_t xs = x + s;
    float64_t ys = y + s;

    return ryce_simplex_noise2_unskewed_base_internal(seed, xs, ys);
}

#endif // RYCE_SIMPLEX_IMPL
#endif // RYCE_SIMPLEX_H
