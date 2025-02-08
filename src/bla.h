#if defined(RYCE_IMPL) && !defined(RYCE_BLA_IMPL)
#define RYCE_BLA_IMPL
#endif
#ifndef RYCE_BLA_H
/*
    RyCE bla - A single-header, STB-styled Bresenham's Line Algorithm.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_BLA_IMPL
       #include "bla.h"

    2) In as many other files as you need, just #include "bla.h"
       WITHOUT defining RYCE_BLA_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_BLA_H

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

/*
    Public API Structs
*/
#ifndef RYCE_VEC2
#define RYCE_VEC2
typedef struct RYCE_Vec2 {
    int64_t x; // X-coordinate.
    int64_t y; // Y-coordinate.
} RYCE_Vec2;
#endif // RYCE_VEC2

typedef union {
    int64_t e1;      // Error accumulator for the first axis.
    int64_t e2;      // Error accumulator for the second axis.
    int initialized; // 0 means uninitialized; 1 means already initialized.
} RYCE_BLA_Error;

/*
    Public API Functions
*/
RYCE_PUBLIC_DECL RYCE_Vec2 ryce_bla_2dline(const RYCE_Vec2 *current, const RYCE_Vec2 *end, RYCE_BLA_Error *error);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_BLA_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_BLA_IMPL

RYCE_PUBLIC RYCE_Vec2 ryce_bla_2dline(const RYCE_Vec2 *current, const RYCE_Vec2 *end, RYCE_BLA_Error *error) {
    // If no movement is needed, reset error state and return.
    if (current->x == end->x && current->y == end->y) {
        error->e1 = 0;
        error->e2 = 0;
        error->initialized = 0;
        return *current;
    }

    // Calculate differences.
    int64_t dx = end->x - current->x;
    int64_t dy = end->y - current->y;

    // Determine step directions.
    int64_t sx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int64_t sy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

    // Get absolute differences.
    int64_t adx = (dx >= 0) ? dx : -dx;
    int64_t ady = (dy >= 0) ? dy : -dy;

    RYCE_Vec2 next = *current;

    // Initialize error state on the first call for this line.
    if (!error->initialized) {
        if (adx >= ady) {
            // For x-dominant lines.
            error->e1 = 2 * ady - adx;
        } else {
            // For y-dominant lines.
            error->e1 = 2 * adx - ady;
        }

        error->e2 = 0;
        error->initialized = 1;
    }

    // Move to the next step.
    if (adx >= ady) {
        // X-dominant.
        if (error->e1 > 0) {
            next.y += sy;
            error->e1 -= 2 * adx;
        }
        error->e1 += 2 * ady;
        next.x += sx;
    } else {
        // Y-dominant.
        if (error->e1 > 0) {
            next.x += sx;
            error->e1 -= 2 * ady;
        }
        error->e1 += 2 * adx;
        next.y += sy;
    }

    return next;
}

#endif // RYCE_BLA_IMPL
#endif // RYCE_BLA_H
