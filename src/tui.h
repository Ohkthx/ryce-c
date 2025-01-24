#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TUI_WIDE_CHAR_SUPPORT
#include <wchar.h>

typedef wchar_t tuichar_t;
typedef unsigned long tuipos_t;

// Wide character functions.
#define TUI_PRINTF(fmt, ...) wprintf((fmt), ##__VA_ARGS__)
#define TUI_SNPRINTF(dst, n, fmt, ...) swprintf((dst), (n), (fmt), ##__VA_ARGS__)
#define TUI_STRLEN(s) wcslen(s)
#define TUI_MEMSET(dst, c, n) wmemset((dst), (wchar_t)(c), (n))
#define TUI_MEMCPY(dst, src, n) wmemcpy((dst), (src), (n))

// Wide character format strings.
#define TUI_LITERAL(s) L##s
#define TUI_NUM_FMT TUI_LITERAL("%lu")

#else

typedef char tuichar_t;
typedef size_t tuipos_t;

// Standard character functions.
#define TUI_PRINTF(fmt, ...) printf((fmt), ##__VA_ARGS__)
#define TUI_SNPRINTF(dst, n, fmt, ...) snprintf((dst), (n), (fmt), ##__VA_ARGS__)
#define TUI_STRLEN(s) strlen(s)
#define TUI_MEMSET(dst, c, n) memset((dst), (c), (n))
#define TUI_MEMCPY(dst, src, n) memcpy((dst), (src), (n))

// Standard character format strings.
#define TUI_LITERAL(s) s
#define TUI_NUM_FMT TUI_LITERAL("%zu")

#endif

// String / Character constants and formatters.
#define TUI_EMPTY_CHAR TUI_LITERAL(' ')
#define TUI_NULL_CHAR TUI_LITERAL('\0')
#define TUI_STR_FMT TUI_LITERAL("%s")

// ANSI escape sequences.
#define TUI_CSI TUI_LITERAL("\033[")
#define TUI_MOVE_ANSI TUI_CSI TUI_NUM_FMT TUI_LITERAL(";") TUI_NUM_FMT TUI_LITERAL("H")
#define TUI_HIDE_CURSOR_ANSI TUI_CSI TUI_LITERAL("?25l")
#define TUI_UNHIDE_CURSOR_ANSI TUI_CSI TUI_LITERAL("?25h")

#ifdef TUI_HIDE_CURSOR
#define TUI_BUFFER_FMT TUI_STR_FMT
#else
#define TUI_BUFFER_FMT TUI_STR_FMT TUI_MOVE_ANSI
#endif

// Error Codes.
const int TUI_ERR_INVALID_TUI = -1;           ///< Invalid TUI pointer.
const int TUI_ERR_INVALID_PANE = -2;          ///< Invalid pane pointer.
const int TUI_ERR_INVALID_BUFFER = -3;        ///< Invalid buffer pointer.
const int TUI_ERR_MOVE_BUFFER_OVERFLOW = -4;  ///< Move buffer overflow.
const int TUI_ERR_WRITE_BUFFER_OVERFLOW = -5; ///< Write buffer overflow.

/**
 * @brief Position within a 2D terminal.
 */
typedef struct Position {
    size_t x; ///< X coordinate on terminal.
    size_t y; ///< Y coordinate on terminal.
} Position;

/**
 * @brief Stores a buffer of characters.
 */
typedef struct SkipSequence {
    int set;          ///< Flag to indicate if the buffer is set.
    size_t start_idx; ///< Start index of the buffer.
    size_t end_idx;   ///< End index of the buffer.
} SkipSequence;

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
    tuichar_t *write_buffer;   ///< Buffer that stores differences to be written.
} Tui;

/**
 * @brief Counts the number of digits in a number.
 *
 * @param num Number to count digits of.
 * @return size_t Number of digits in the number.
 */
static inline size_t _TUI_digits(size_t num) { return num == 0 ? 1 : (size_t)log10(num) + 1; }

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

    size_t total_size = sizeof(Pane) + (width * height) * sizeof(tuichar_t) * 2;
    Pane *pane = (Pane *)malloc(total_size);
    if (!pane) {
        return NULL;
    }

    pane->width = width;
    pane->height = height;
    pane->length = width * height;
    pane->update = (tuichar_t *)(pane + 1);
    pane->cache = pane->update + pane->length;

    // Fills the buffers with empty spaces.
    TUI_MEMSET(pane->update, TUI_EMPTY_CHAR, pane->length);
    TUI_MEMSET(pane->cache, TUI_EMPTY_CHAR, pane->length);

    return pane;
}

/**
 * @brief Checks if a pane is initialized correctly.
 */
static inline int _TUI_is_init_pane(Pane *pane) { return pane && pane->update && pane->cache; }

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
        TUI_PRINTF("Failed to init pane.\n");
        free(tui);
        return NULL;
    }

    // Create the buffer to store differences to be written.
    int default_length = (width * height) * 3;
    tui->max_length = default_length < 1024 ? 1024 : default_length;
    tui->write_buffer = (tuichar_t *)malloc(tui->max_length * sizeof(tuichar_t));
    if (!tui->write_buffer) {
        free(tui->pane);
        free(tui);
        return NULL;
    }

    tui->write_length = 0;
    TUI_MEMSET(tui->write_buffer, TUI_NULL_CHAR, tui->max_length);
    TUI_MEMSET(tui->move_buffer, TUI_NULL_CHAR, sizeof(tui->move_buffer) / sizeof(tuichar_t));

#ifdef TUI_HIDE_CURSOR
    TUI_PRINTF(TUI_HIDE_CURSOR_ANSI); // Hide the cursor.
#endif

    return tui;
}

/**
 * @brief Frees the memory allocated for a TUI.
 */
static inline void TUI_free(Tui *tui) {
    if (!tui) {
        return;
    }

    free(tui->pane);
    free(tui->write_buffer);
    free(tui);

#ifdef TUI_HIDE_CURSOR
    TUI_PRINTF(TUI_UNHIDE_CURSOR_ANSI); // Show the cursor.
#endif
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
    TUI_SNPRINTF(tui->move_buffer, sizeof(tui->move_buffer) / sizeof(tuichar_t), TUI_MOVE_ANSI,
                 (tuipos_t)y + 1, (tuipos_t)x + 1);

    size_t seq_len = TUI_STRLEN(tui->move_buffer);
    if (tui->write_length + seq_len >= tui->max_length) {
        return TUI_ERR_MOVE_BUFFER_OVERFLOW;
    }

    // Append the move sequence to the write buffer.
    TUI_MEMCPY(tui->write_buffer + tui->write_length, tui->move_buffer, seq_len);
    tui->write_length += seq_len;
    return 0;
}

/**
 * @brief Injects a move sequence or reprinted characters into the write buffer.
 *
 * @param tui TUI to write to.
 * @param x X position to write to.
 * @param y Y position to write to.
 * @param skip Skip sequence to check.
 * @return int 0 if successful, otherwise an error code.
 */
static inline int inject_sequence(Tui *tui, size_t x, size_t y, SkipSequence *skip) {
    if (tui->cursor.y != y) {
        // Move the cursor to the correct row.
        int err_code = _TUI_write_move(tui, x, y);
        if (err_code != 0) {
            return err_code;
        }

        skip->set = 0; // Reset the skip sequence.
        return 0;
    } else if (!skip->set || tui->cursor.x == x) {
        return 0;
    }

    size_t skipped_length = skip->end_idx - skip->start_idx;
    size_t expected = _TUI_digits(x) + _TUI_digits(y) + 7;

    if (skipped_length && skipped_length < expected) {
        if (tui->write_length + skipped_length >= tui->max_length) {
            // Not enough space in the write buffer.
            return TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Reprint skipped due to being cheaper than moving cursor.
        TUI_MEMCPY(tui->write_buffer + tui->write_length, tui->pane->update + skip->start_idx,
                   skipped_length);
        tui->write_length += skipped_length;
    } else {
        // Move the cursor to the correct position.
        int err_code = _TUI_write_move(tui, x, y);
        if (err_code != 0) {
            return err_code;
        }
    }

    skip->set = 0; // Reset the skip sequence.
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
    SkipSequence skip = {0, 0, 0};
    size_t x = 1, y = 1;

    for (size_t i = 0; i < tui->pane->length; i++) {
        if (tui->pane->update[i] == tui->pane->cache[i]) {
            // No change, the character is skipable.
            if (!skip.set) {
                skip.start_idx = i;
                skip.end_idx = i + 1;
                skip.set = 1;
            } else {
                skip.end_idx++;
            }

            continue;
        }

        x = i % tui->pane->width; // X position in flattened buffer.
        y = i / tui->pane->width; // Y position in flattened buffer.

        // Inject move sequences or reprinted characters.
        int seq_error = inject_sequence(tui, x, y, &skip);
        if (seq_error != 0) {
            return seq_error;
        }

        // Check if we're next to the last written character.
        if (tui->write_length + 1 >= tui->max_length) {
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
    int index = tui->write_length < tui->max_length ? tui->write_length : tui->max_length - 1;
    tui->write_buffer[index] = TUI_NULL_CHAR;

#ifdef TUI_HIDE_CURSOR
    TUI_PRINTF(TUI_BUFFER_FMT, tui->write_buffer);
#else
    // Render the differences and move the cursor to the bottom of the pane.
    tui->cursor.x = tui->pane->width;
    tui->cursor.y = tui->pane->height;
    TUI_PRINTF(TUI_BUFFER_FMT, tui->write_buffer, (tuipos_t)tui->cursor.y,
               (tuipos_t)tui->cursor.x);
#endif

    fflush(stdout);
    return 0;
}

#ifdef __cplusplus
}
#endif
