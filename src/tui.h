#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TUI_WIDE_CHAR_SUPPORT
#include <wchar.h>

typedef wchar_t tuichar_t;
typedef unsigned long tuipos_t;

// Wide character functions.
#define TUI_PRINTF(fmt, ...) wprintf((fmt), __VA_ARGS__)
#define TUI_SNPRINTF(dst, n, fmt, ...) swprintf((dst), (n), (fmt), __VA_ARGS__)
#define TUI_STRLEN(s) wcslen(s)
#define TUI_MEMSET(dst, c, n) wmemset((dst), (wchar_t)(c), (n))
#define TUI_MEMCPY(dst, src, n) wmemcpy((dst), (src), (n))

// Wide character format strings.
#define TUI_EMPTY_CHAR L' '
#define TUI_NULL_CHAR L'\0'
#define TUI_MOVE_ANSI L"\033[%lu;%luH"
#define TUI_FMT_MOVE_ANSI L"%ls\033[%lu;%luH"

#else

typedef char tuichar_t;
typedef size_t tuipos_t;

// Standard character functions.
#define TUI_PRINTF(fmt, ...) printf((fmt), __VA_ARGS__)
#define TUI_SNPRINTF(dst, n, fmt, ...) snprintf((dst), (n), (fmt), __VA_ARGS__)
#define TUI_STRLEN(s) strlen(s)
#define TUI_MEMSET(dst, c, n) memset((dst), (c), (n))
#define TUI_MEMCPY(dst, src, n) memcpy((dst), (src), (n))

// Standard character format strings.
#define TUI_EMPTY_CHAR ' '
#define TUI_NULL_CHAR '\0'
#define TUI_MOVE_ANSI "\033[%zu;%zuH"
#define TUI_FMT_MOVE_ANSI "%s\033[%zu;%zuH"

#endif

// Error Codes.
const int TUI_ERR_INVALID_TUI = -1;           ///< Invalid TUI pointer.
const int TUI_ERR_INVALID_PANE = -2;          ///< Invalid pane pointer.
const int TUI_ERR_MOVE_BUFFER_OVERFLOW = -3;  ///< Move buffer overflow.
const int TUI_ERR_WRITE_BUFFER_OVERFLOW = -4; ///< Write buffer overflow.

/**
 * @brief Position within a 2D terminal.
 */
typedef struct Position {
    size_t x; ///< X coordinate on terminal.
    size_t y; ///< Y coordinate on terminal.
} Position;

/**
 * @brief Stores the contents of a terminal pane to be rendered.
 */
typedef struct Pane {
    size_t width;      ///< Pane width in characters.
    size_t height;     ///< Pane height in characters.
    size_t length;     ///< Length of the update and back buffers.
    tuichar_t *update; ///< Current modified buffer.
    tuichar_t *cache;  ///< Last rendered buffer.
} Pane;

/**
 * @brief Holds all pane data and responsible for rendering to the terminal.
 */
typedef struct Tui {
    Position cursor;           ///< Current cursor position.
    tuichar_t move_buffer[32]; ///< Buffer to store move sequences.
    Pane *pane;                ///< Current pane.
    size_t max_length;         ///< Maximum length of the write_buffer.
    size_t write_length;       ///< Current length of the write_buffer.
    tuichar_t *write_buffer; ///< Buffer that stores differences to be written.
} Tui;

/**
 * @brief Initializes a pane and fills its buffers with default values.
 *
 * @param width Width of the pane.
 * @param height Height of the pane.
 * @return Pane* Pointer to the initialized pane.
 */
static inline Pane *_TUI_init_pane(size_t width, size_t height) {
    if (width == 0 || height == 0) {
        return NULL;
    }

    Pane *pane = (Pane *)malloc(sizeof(Pane));
    if (!pane) {
        return NULL;
    }

    pane->width = width;
    pane->height = height;
    pane->length = width * height;

    pane->update = (tuichar_t *)malloc(pane->length * sizeof(tuichar_t));
    if (!pane->update) {
        free(pane);
        return NULL;
    }

    pane->cache = (tuichar_t *)malloc(pane->length * sizeof(tuichar_t));
    if (!pane->cache) {
        free(pane->update);
        free(pane);
        return NULL;
    }

    // Fills the buffers with empty spaces.
    TUI_MEMSET(pane->update, TUI_EMPTY_CHAR, pane->length);
    TUI_MEMSET(pane->cache, TUI_EMPTY_CHAR, pane->length);

    return pane;
}

/**
 * @brief Checks if a pane is initialized correctly.
 */
static inline int _TUI_is_init_pane(Pane *pane) {
    return pane && pane->update && pane->cache;
}

/**
 * @brief Frees the memory allocated for a pane.
 */
static inline void _TUI_free_pane(Pane *pane) {
    if (!pane) {
        return;
    }

    free(pane->update);
    free(pane->cache);
    free(pane);
}

/**
 * @brief Initializes a TUI with a pane and a buffer to store changes.
 *
 * @param width Width of the interface to be rendered.
 * @param height Height of the interface to be rendered.
 * @return Tui* Pointer to the initialized TUI.
 */
static inline Tui *TUI_init(size_t width, size_t height) {
    if (width == 0 || height == 0) {
        return NULL;
    }

    Tui *tui = (Tui *)malloc(sizeof(Tui));
    if (!tui) {
        return NULL;
    }

    tui->cursor = (Position){width, height};

    // Create the default pane.
    tui->pane = _TUI_init_pane(width, height);
    if (!tui->pane) {
        printf("Failed to init pane.\n");
        free(tui);
        return NULL;
    }

    // Create the buffer to store differences to be written.
    int default_length = (width * height) * 3;
    tui->max_length = default_length < 1024 ? 1024 : default_length;
    tui->write_buffer =
        (tuichar_t *)malloc(tui->max_length * sizeof(tuichar_t));
    if (!tui->write_buffer) {
        _TUI_free_pane(tui->pane);
        free(tui);
        return NULL;
    }

    tui->write_length = 0;
    TUI_MEMSET(tui->write_buffer, TUI_NULL_CHAR, tui->max_length);
    TUI_MEMSET(tui->move_buffer, TUI_NULL_CHAR,
               sizeof(tui->move_buffer) / sizeof(tuichar_t));

    return tui;
}

/**
 * @brief Frees the memory allocated for a TUI.
 */
static inline void TUI_free(Tui *tui) {
    if (!tui) {
        return;
    }

    _TUI_free_pane(tui->pane);
    free(tui->write_buffer);
    free(tui);
}

/**
 * @brief Resets the TUIs write buffer and move buffer.
 */
static inline void _TUI_softreset_tui(Tui *tui) {
    if (!tui) {
        return;
    }

    tui->write_length = 0;

    // Null-terminate the buffers.
    tui->write_buffer[0] = TUI_NULL_CHAR;
    tui->move_buffer[0] = TUI_NULL_CHAR;
}

/**
 * @brief Writes a move sequence to the TUIs write buffer.
 *
 * @param x X position to move to.
 * @param y Y position to move to.
 * @return int 0 if successful, otherwise an error code.
 */
static inline int _TUI_write_move(Tui *tui, size_t x, size_t y) {
    if (!tui) {
        return TUI_ERR_INVALID_TUI;
    }

    // Write the move sequence to the buffer.
    TUI_SNPRINTF(tui->move_buffer, sizeof(tui->move_buffer) / sizeof(tuichar_t),
                 TUI_MOVE_ANSI, (tuipos_t)y + 1, (tuipos_t)x + 1);

    size_t seq_len = TUI_STRLEN(tui->move_buffer);
    if (tui->write_length + seq_len >= tui->max_length) {
        return TUI_ERR_MOVE_BUFFER_OVERFLOW;
    }

    // Append the move sequence to the write buffer.
    TUI_MEMCPY(tui->write_buffer + tui->write_length, tui->move_buffer,
               seq_len);
    tui->write_length += seq_len;
    return 0;
}

/**
 * @brief Renders the TUI and all panes to the terminal.
 *
 * @param tui TUI to render.
 * @return int 0 if successful, otherwise an error code.
 */
static inline int TUI_render(Tui *tui) {
    if (!tui) {
        return TUI_ERR_INVALID_TUI;
    } else if (!_TUI_is_init_pane(tui->pane)) {
        return TUI_ERR_INVALID_PANE;
    }

    // Reset the write and move sequence buffers.
    _TUI_softreset_tui(tui);

    for (size_t i = 0; i < tui->pane->length; i++) {
        size_t x = i % tui->pane->width; // X position in flattened buffer.
        size_t y = i / tui->pane->width; // Y position in flattened buffer.

        if (tui->pane->update[i] == tui->pane->cache[i]) {
            // No change, skip.
            continue;
        }

        // Check if we're next to the last written character.
        if (tui->cursor.y != y || tui->cursor.x != x - 1) {
            // Write the move cursor sequence.
            int err_code = _TUI_write_move(tui, x, y);
            if (err_code != 0) {
                return err_code;
            }
        } else if (tui->write_length + 1 >= tui->max_length) {
            // Exceeded capacity for the write buffer.
            return TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Update buffer being written, propagate the change to the back buffer.
        tui->write_buffer[tui->write_length++] = tui->pane->update[i];
        tui->pane->cache[i] = tui->pane->update[i];
        tui->cursor.x = x;
        tui->cursor.y = y;
    }

    // Null-terminate for printf.
    int index = tui->write_length < tui->max_length ? tui->write_length
                                                    : tui->max_length - 1;
    tui->write_buffer[index] = TUI_NULL_CHAR;

    // Render the differences and move the cursor to the bottom of the pane.
    tui->cursor.x = tui->pane->width;
    tui->cursor.y = tui->pane->height;
    TUI_PRINTF(TUI_FMT_MOVE_ANSI, tui->write_buffer, (tuipos_t)tui->cursor.y,
               (tuipos_t)tui->cursor.x);

    fflush(stdout);
    return 0;
}

#ifdef __cplusplus
}
#endif
