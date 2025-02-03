#if defined(RYCE_IMPL) && !defined(RYCE_MAP_IMPL)
#define RYCE_MAP_IMPL
#endif
#ifndef RYCE_MAP_H
/*
    RyCE Map - A single-header, STB-styled map controller.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_MAP_IMPL
       #include "map.h"

    2) In as many other files as you need, just #include "map.h"
       WITHOUT defining RYCE_MAP_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_MAP_H

#include "vec.h"
#include <stddef.h>

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
typedef enum RYCE_MapError {
    RYCE_MAP_ERR_NONE,           ///< No error.
    RYCE_MAP_INVALID_DIMENSIONS, ///< Invalid map dimensions.
    RYCE_MAP_INVALID_DATA,       ///< Invalid map data.
    RYCE_MAP_INVALID_PLACEMENT,  ///< Invalid entity placement.
    RYCE_MAP_ENTITY_NOT_FOUND,   ///< Entity not found.
} RYCE_MapError;

/*
    Public API Structs
*/

#ifndef RYCE_ENTITY
#define RYCE_ENTITY
typedef size_t RYCE_EntityID;
#define RYCE_ENTITY_NONE 0;
#endif // RYCE_ENTITY

typedef struct RYCE_3dTextMap {
    struct {
        int64_t min; //< Minimum value on axis.
        int64_t max; //< Maximum value on axis.
    } x, y, z;
    size_t length;       //< Length of the 3D space.
    size_t width;        //< Width of the 3D space.
    size_t height;       //< Height of the 3D space.
    RYCE_EntityID *data; //< 3D map data.
} RYCE_3dTextMap;

/*
    Public API Functions
*/

/**
 * @brief Initializes a 3D map.
 *
 * @param map Map to initialize.
 * @param length Length of the 3D space.
 * @param width Width of the 3D space.
 * @param height Height of the 3D space.
 * @return RYCE_MapError RYCE_MAP_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_MapError ryce_init_3d_map(RYCE_3dTextMap *map, size_t length, size_t width, size_t height);

/**
 * @brief Maps an entity to a 3D coordinate.
 *
 * @param map Map to place the entity.
 * @param vec 3D coordinates to place the entity.
 * @param entity Entity to place.
 * @return RYCE_MapError RYCE_MAP_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_MapError ryce_map_add_entity(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec,
                                                   RYCE_EntityID entity);

/**
 * @brief Unmaps an entity from a 3D coordinate.
 *
 * @param map Map to remove the entity from.
 * @param vec 3D coordinates to remove the entity from.
 * @param entity Entity to remove.
 * @return RYCE_MapError RYCE_MAP_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_MapError ryce_map_remove_entity(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec,
                                                      RYCE_EntityID entity);

/**
 * @brief Gets the entity at a 3D coordinate.
 *
 * @param map Map to get the entity from.
 * @param vec 3D coordinates to get the entity from.
 * @return RYCE_EntityID Entity at the 3D coordinates.
 */
RYCE_PUBLIC_DECL RYCE_EntityID ryce_map_get_entity(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_MAP_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_MAP_IMPL

#include <stdlib.h>

#ifndef RYCE_MATH_CLAMP
#define RYCE_MATH_CLAMP
RYCE_PUBLIC inline int64_t ryce_math_clamp(int64_t val, int64_t min, int64_t max) {
    return (val < min) ? min : (val > max) ? max : val;
}
#endif // RYCE_MATH_CLAMP

RYCE_PRIVATE inline size_t ryce_translate_vec_internal(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec) {
    // Determine the offset needed to shift user coordinates into 0-based indices.
    // Since map->x.min == -(length/2), we have:
    const int64_t x_offset = -(map->x.min); // equivalent to map->length/2
    const int64_t y_offset = -(map->y.min); // equivalent to map->width/2
    const int64_t z_offset = -(map->z.min); // equivalent to map->height/2

    // Translate user coordinates to internal indices.
    const int64_t internal_x = ryce_math_clamp(vec->x + x_offset, 0, (int64_t)map->length - 1);
    const int64_t internal_y = ryce_math_clamp(vec->y + y_offset, 0, (int64_t)map->width - 1);
    const int64_t internal_z = ryce_math_clamp(vec->z + z_offset, 0, (int64_t)map->height - 1);

    // Convert 3D indices to a 1D index.
    RYCE_Vec3 internal_vec = {.x = internal_x, .y = internal_y, .z = internal_z};
    return ryce_vec3_to_idx(&internal_vec, (int64_t)map->length, (int64_t)map->width);
}

RYCE_PUBLIC RYCE_MapError ryce_init_3d_map(RYCE_3dTextMap *map, size_t length, size_t width, size_t height) {
    if (length == 0 || width == 0 || height == 0) {
        return RYCE_MAP_INVALID_DIMENSIONS;
    }

    // To have a centered 0, we need an odd map size.
    length = (length % 2 == 0) ? length + 1 : length;
    width = (width % 2 == 0) ? width + 1 : width;
    height = (height % 2 == 0) ? height + 1 : height;

    *map = (RYCE_3dTextMap){
        .x.min = -(int)(length / 2),
        .x.max = (int)(length / 2),
        .y.min = -(int)(width / 2),
        .y.max = (int)(width / 2),
        .z.min = -(int)(height / 2),
        .z.max = (int)(height / 2),
        .length = length,
        .width = width,
        .height = height,
    };

    // Allocate the map data to be empty (0).
    map->data = (RYCE_EntityID *)calloc(length * width * height, sizeof(RYCE_EntityID));
    if (!map->data) {
        return RYCE_MAP_INVALID_DATA;
    };

    return RYCE_MAP_ERR_NONE;
}

RYCE_PUBLIC RYCE_MapError ryce_map_add_entity(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec, RYCE_EntityID entity) {
    if (!map || !vec) {
        return RYCE_MAP_INVALID_DATA;
    }

    // Translate the 3D coordinates to a 1D index.
    size_t idx = ryce_translate_vec_internal(map, vec);
    idx = ryce_math_clamp((int64_t)idx, 0, map->length * map->width * map->height);

    if (map->data[idx] != 0) {
        return RYCE_MAP_INVALID_PLACEMENT;
    }

    map->data[idx] = entity;
    return RYCE_MAP_ERR_NONE;
}

RYCE_PUBLIC RYCE_MapError ryce_map_remove_entity(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec,
                                                 RYCE_EntityID entity) {
    if (!map || !vec) {
        return RYCE_MAP_INVALID_DATA;
    }

    // Translate the 3D coordinates to a 1D index.
    const size_t idx = ryce_translate_vec_internal(map, vec);
    if (idx >= map->length * map->width * map->height || idx < 0) {
        return RYCE_MAP_INVALID_PLACEMENT;
    }

    if (map->data[idx] != entity) {
        return RYCE_MAP_ENTITY_NOT_FOUND;
    }

    map->data[idx] = 0;
    return RYCE_MAP_ERR_NONE;
}

RYCE_PUBLIC RYCE_EntityID ryce_map_get_entity(const RYCE_3dTextMap *map, const RYCE_Vec3 *vec) {
    if (!map || !vec) {
        return RYCE_ENTITY_NONE;
    }

    // Translate the 3D coordinates to a 1D index.
    const size_t idx = ryce_translate_vec_internal(map, vec);
    if (idx >= map->length * map->width * map->height || idx < 0) {
        return RYCE_ENTITY_NONE;
    }

    return map->data[idx];
}

#endif // RYCE_MAP_IMPL
#endif // RYCE_MAP_H
