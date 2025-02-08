#if defined(RYCE_IMPL) && !defined(RYCE_CAMERA_IMPL)
#define RYCE_CAMERA_IMPL
#endif
#ifndef RYCE_CAMERA_H
/*
    RyCE camera - A single-header, STB-styled camera controller.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_CAMERA_IMPL
       #include "camera.h"

    2) In as many other files as you need, just #include "camera.h"
       WITHOUT defining RYCE_CAMERA_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_CAMERA_H

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
typedef enum RYCE_CameraError {
    RYCE_CAMERA_ERR_NONE,               ///< No error.
    RYCE_CAMERA_ERR_INVALID_DIMENSIONS, ///< Invalid camera dimensions.
} RYCE_CameraError;

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

typedef struct RYCE_CameraContext {
    struct {
        int64_t width;  // Width of the screen (max x).
        int64_t height; // Height of the screen (max y).
    } screen;
    RYCE_Vec2 center; // Center of the camera to calculate offset.
} RYCE_CameraContext;

/*
    Public API Functions
*/

/**
 * @brief Initializes a camera context.
 *
 * @param camera Camera context to initialize.
 * @param screen_width Width of the screen.
 * @param screen_height Height of the screen.
 * @param center Center of the camera, relative to a map or some other area.
 * @return RYCE_CameraError RYCE_CAMERA_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_CameraError ryce_init_camera_ctx(RYCE_CameraContext *camera, int64_t screen_width,
                                                       int64_t screen_height, RYCE_Vec2 center);

/**
 * @brief Converts a terminal position to a map position.
 *
 * @param camera Camera context.
 * @param position Terminal position.
 * @return RYCE_Vec2 Map position.
 */
RYCE_PUBLIC_DECL RYCE_Vec2 ryce_from_terminal(RYCE_CameraContext *camera, const RYCE_Vec2 *position);

/**
 * @brief Converts a map position to a terminal position.
 *
 * @param camera Camera context.
 * @param position Map position.
 * @return RYCE_Vec2 Terminal position.
 */
RYCE_PUBLIC_DECL RYCE_Vec2 ryce_to_terminal(RYCE_CameraContext *camera, const RYCE_Vec2 *position);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_CAMERA_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_CAMERA_IMPL

RYCE_PUBLIC_DECL RYCE_CameraError ryce_init_camera_ctx(RYCE_CameraContext *camera, int64_t screen_width,
                                                       int64_t screen_height, RYCE_Vec2 center) {
    if (screen_width <= 0 || screen_height <= 0) {
        return RYCE_CAMERA_ERR_INVALID_DIMENSIONS;
    }

    *camera = (RYCE_CameraContext){
        .screen =
            {
                .width = screen_width,
                .height = screen_height,
            },
        .center = center,
    };

    return RYCE_CAMERA_ERR_NONE;
}

RYCE_PUBLIC_DECL RYCE_Vec2 ryce_from_terminal(RYCE_CameraContext *camera, const RYCE_Vec2 *position) {
    int64_t map_x = camera->center.x + (position->x - camera->screen.width / 2);
    int64_t map_y = camera->center.y + (position->y - camera->screen.height / 2);
    return (RYCE_Vec2){.x = map_x, .y = map_y};
}

RYCE_PUBLIC_DECL RYCE_Vec2 ryce_to_terminal(RYCE_CameraContext *camera, const RYCE_Vec2 *position) {
    int64_t term_x = (position->x - camera->center.x) + (camera->screen.width / 2);
    int64_t term_y = (position->y - camera->center.y) + (camera->screen.height / 2);
    return (RYCE_Vec2){.x = term_x, .y = term_y};
}

#endif // RYCE_CAMERA_IMPL
#endif // RYCE_CAMERA_H
