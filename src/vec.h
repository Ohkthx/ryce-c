#if defined(RYCE_IMPL) && !defined(RYCE_VEC_IMPL)
#define RYCE_VEC_IMPL
#endif
#ifndef RYCE_VEC_H
/*
    RyCE vec - A single-header, STB-styled Vector implementaiton.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_VEC_IMPL
       #include "vec.h"

    2) In as many other files as you need, just #include "vec.h"
       WITHOUT defining RYCE_VEC_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_VEC_H

#include <stddef.h>
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

// Error Codes.
typedef enum RYCE_VecError {
    RYCE_VEC_ERR_NONE ///< No error.
} RYCE_VecError;

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

#ifndef RYCE_VEC3
#define RYCE_VEC3
typedef struct RYCE_Vec3 {
    int64_t x; // X-coordinate.
    int64_t y; // Y-coordinate.
    int64_t z; // Z-coordinate.
} RYCE_Vec3;
#endif // RYCE_VEC3

/*
    Public API Functions
*/

/**
 * @brief Converts 2D coordinates to a 1D index.
 *
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @param width Width of the 2D space.
 * @return int64_t Index in the buffer.
 */
RYCE_PUBLIC_DECL inline int64_t ryce_vec2_to_idx(const RYCE_Vec2 *vec, int64_t width);

/**
 * @brief Converts 3D coordinates to a 1D index.
 *
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @param z Z-coordinate.
 * @param width Width of the 3D space.
 * @param height Height of the 3D space.
 * @return int64_t Index in the buffer.
 */
RYCE_PUBLIC_DECL inline int64_t ryce_vec3_to_idx(const RYCE_Vec3 *vec, int64_t width, int64_t height);

/**
 * @brief Converts a 1D index to 2D coordinates.
 *
 * @param idx Index in the buffer.
 * @param width Width of the 2D space.
 * @return RYCE_Vec2 2D coordinates.
 */
RYCE_PUBLIC_DECL inline RYCE_Vec2 ryce_idx_to_vec2(int64_t idx, int64_t width);

/**
 * @brief Converts a 1D index to 3D coordinates.
 *
 * @param idx Index in the buffer.
 * @param width Width of the 3D space.
 * @param height Height of the 3D space.
 * @return RYCE_Vec3 3D coordinates.
 */
RYCE_PUBLIC_DECL inline RYCE_Vec3 ryce_idx_to_vec3(int64_t idx, int64_t width, int64_t height);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_VEC_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_VEC_IMPL

RYCE_PUBLIC inline int64_t ryce_vec2_to_idx(const RYCE_Vec2 *vec, const int64_t width) {
    return (vec->y * width) + vec->x;
}

RYCE_PUBLIC inline int64_t ryce_vec3_to_idx(const RYCE_Vec3 *vec, const int64_t width, const int64_t height) {
    return (vec->z * width * height) + (vec->y * width) + vec->x;
}

RYCE_PUBLIC inline RYCE_Vec2 ryce_idx_to_vec2(const int64_t idx, const int64_t width) {
    return (RYCE_Vec2){.x = (int64_t)(idx % width), .y = (int64_t)(idx / width)};
}

RYCE_PUBLIC inline RYCE_Vec3 ryce_idx_to_vec3(const int64_t idx, const int64_t width, const int64_t height) {
    return (RYCE_Vec3){
        .x = (int64_t)(idx % width), .y = (int64_t)((idx / width) % height), .z = (int64_t)(idx / (width * height))};
}

#endif // RYCE_VEC_IMPL
#endif // RYCE_VEC_H
