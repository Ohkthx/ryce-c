#if defined(RYCE_IMPL) && !defined(RYCE_FOV_IMPL)
#define RYCE_FOV_IMPL
#endif
#ifndef RYCE_FOV_H
/*
    RyCE fov - A single-header, STB-styled FOV Controller using Shadowcasting & Recursion.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_FOV_IMPL
       #include "fov.h"

    2) In as many other files as you need, just #include "fov.h"
       WITHOUT defining RYCE_FOV_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_FOV_H

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

#ifndef RYCE_OPAQUE_VALUE
#define RYCE_OPAQUE_VALUE 0
#endif // RYCE_OPAQUE_VALUE

// Error Codes.
typedef enum RYCE_FovError {
    RYCE_FOV_ERR_NONE, ///< No error.
} RYCE_FovError;

typedef enum {
    RYCE_FOV_UNSEEN = 0 << 0, ///< Unseen.
    RYCE_FOV_SEEN = 1 << 0,   ///< Seen.
    RYCE_FOV_VISIBLE = 1 << 1 ///< Visible.
} RYCE_FovFlags;

/*
    Public API Functions
*/

/**
 * @brief Casts light in a circular pattern from a given origin point.
 *
 * @param origin_x X-coordinate of the origin point.
 * @param origin_y Y-coordinate of the origin point.
 * @param radius Radius of the light circle.
 * @param src Pointer to the pathing / blocked map.
 * @param dst Pointer to the visibility map.
 * @param width Width of the map.
 * @param height Height of the map.
 * @return RYCE_FovError Error code indicating success or failure.
 */
RYCE_PUBLIC_DECL RYCE_FovError ryce_fov(uint32_t origin_x, uint32_t origin_y, uint16_t radius, uint8_t *src,
                                        uint8_t *dst, uint32_t width, uint32_t height);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_FOV_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_FOV_IMPL

#ifndef RYCE_MATH_FAST_FLOOR
#define RYCE_MATH_FAST_FLOOR
RYCE_PUBLIC inline int32_t ryce_math_fast_floor(double x) {
    int32_t xi = (int32_t)x;
    if (x < xi) {
        return xi - 1;
    } else {
        return xi;
    }
}
#endif // RYCE_MATH_FAST_FLOOR

RYCE_PRIVATE const int RYCE_FOV_MULTIPLERS[4][8] = {
    {1, 0, 0, -1, -1, 0, 0, 1},
    {0, 1, -1, 0, 0, -1, 1, 0},
    {0, 1, 1, 0, 0, -1, -1, 0},
    {1, 0, 0, 1, -1, 0, 0, -1},
};

RYCE_PRIVATE RYCE_FovError ryce_fov_cast_light_internal(int32_t cx, int32_t cy, int32_t radius, int32_t row,
                                                        float start_slope, float end_slope, uint8_t *map, uint8_t *out,
                                                        int32_t width, int32_t height, int32_t xx, int32_t xy,
                                                        int32_t yx, int32_t yy) {
    // If the wedge is fully closed, or we've exceeded row-based distance, stop.
    if (start_slope < end_slope || row > radius) {
        return RYCE_FOV_ERR_NONE;
    }

    int blocked = 0;
    float new_start_slope = start_slope;

    int32_t left_col = ryce_math_fast_floor(((double)row * start_slope) + 0.5F);
    int32_t right_col = ryce_math_fast_floor(((double)row * end_slope) + 0.5F);

    for (int32_t col = left_col; col >= right_col; col--) {
        // Convert (row, col) to actual map coordinates.
        int32_t map_x = cx + (col * xx) + (row * xy);
        int32_t map_y = cy + (col * yx) + (row * yy);

        float l_slope = ((float)col - 0.5F) / ((float)row + 0.5F);
        float r_slope = ((float)col + 0.5F) / ((float)row - 0.5F);

        // Skip if out of bounds.
        if (map_x < 0 || map_x >= width || map_y < 0 || map_y >= height) {
            continue;
        }

        // Check if it's within the circular radius.
        int32_t dx = map_x - cx;
        int32_t dy = map_y - cy;
        int32_t rad2 = radius * radius;
        if ((dx * dx + dy * dy) <= rad2) {
            // Mark it visible and seen.
            out[(map_y * width) + map_x] |= (RYCE_FOV_VISIBLE | RYCE_FOV_SEEN);
        }

        if (blocked) {
            // We are scanning through a shadow
            if (map[(map_y * width) + map_x] == RYCE_OPAQUE_VALUE) {
                new_start_slope = r_slope;
                continue;
            } else {
                blocked = 0;
                start_slope = new_start_slope;
            }
        } else {
            if (map[(map_y * width) + map_x] == RYCE_OPAQUE_VALUE && row < radius) {
                blocked = 1;
                // Recurse for the blocked area.
                ryce_fov_cast_light_internal(cx, cy, radius, row + 1, start_slope, r_slope, map, out, width, height, xx,
                                             xy, yx, yy);
                new_start_slope = l_slope;
            }
        }
    }

    // If still not fully blocked, continue outwards.
    if (!blocked) {
        ryce_fov_cast_light_internal(cx, cy, radius, row + 1, start_slope, end_slope, map, out, width, height, xx, xy,
                                     yx, yy);
    }

    return RYCE_FOV_ERR_NONE;
}

RYCE_PUBLIC RYCE_FovError ryce_fov(uint32_t origin_x, uint32_t origin_y, uint16_t radius, uint8_t *src, uint8_t *dst,
                                   uint32_t width, uint32_t height) {

    for (uint32_t i = 0; i < 8; i++) {
        ryce_fov_cast_light_internal(origin_x, origin_y, radius, 1, 1.0F, 0.0F, src, dst, width, height,
                                     RYCE_FOV_MULTIPLERS[0][i], RYCE_FOV_MULTIPLERS[1][i], RYCE_FOV_MULTIPLERS[2][i],
                                     RYCE_FOV_MULTIPLERS[3][i]);
    }

    return RYCE_FOV_ERR_NONE;
}

#endif // RYCE_FOV_IMPL
#endif // RYCE_FOV_H
