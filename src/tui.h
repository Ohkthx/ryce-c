#ifndef RYCE_TUI_H

/*
    RyCE TUI - A single-header, STB-styled terminal UI library.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_TUI_IMPL
       #include "tui.h"

    2) In as many other files as you need, just #include "tui.h"
       WITHOUT defining RYCE_TUI_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_TUI_H (1)
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    Wide-char vs. standard char support.
    These macros and typedefs are "public" because they determine
    the types and functions that user code might call or reference.
*/
#ifdef RYCE_WIDE_CHAR_SUPPORT
#include <wchar.h>

#define RYCE_CHAR wchar_t
#define RYCE_SIZE_T unsigned long

// Wide character functions.
#define RYCE_PRINTF(fmt, ...) wprintf((fmt), ##__VA_ARGS__)
#define RYCE_SNPRINTF(dst, n, fmt, ...) swprintf((dst), (n), (fmt), ##__VA_ARGS__)
#define RYCE_STRLEN(s) wcslen(s)
#define RYCE_MEMSET(dst, c, n) wmemset((dst), (wchar_t)(c), (n))
#define RYCE_MEMCPY(dst, src, n) wmemcpy((dst), (src), (n))

// Wide character format strings.
#define RYCE_LITERAL(s) L##s
#define RYCE_NUM_FMT RYCE_LITERAL("%lu")

#else

#define RYCE_CHAR char
#define RYCE_SIZE_T size_t

// Standard character functions.
#define RYCE_PRINTF(fmt, ...) printf((fmt), ##__VA_ARGS__)
#define RYCE_SNPRINTF(dst, n, fmt, ...) snprintf((dst), (n), (fmt), ##__VA_ARGS__)
#define RYCE_STRLEN(s) strlen(s)
#define RYCE_MEMSET(dst, c, n) memset((dst), (c), (n))
#define RYCE_MEMCPY(dst, src, n) memcpy((dst), (src), (n))

// Standard character format strings.
#define RYCE_LITERAL(s) s
#define RYCE_NUM_FMT RYCE_LITERAL("%zu")

#endif // RYCE_WIDE_CHAR_SUPPORT

// String / Character constants and formatters.
#define RYCE_EMPTY_CHAR RYCE_LITERAL(' ')
#define RYCE_NULL_CHAR RYCE_LITERAL('\0')
#define RYCE_STR_FMT RYCE_LITERAL("%s")

// ANSI escape sequences.
#define RYCE_CSI RYCE_LITERAL("\x1b[")
#define RYCE_MOVE_ANSI RYCE_CSI RYCE_NUM_FMT RYCE_LITERAL(";") RYCE_NUM_FMT RYCE_LITERAL("H")
#define RYCE_HIDE_CURSOR_ANSI RYCE_CSI RYCE_LITERAL("?25l")
#define RYCE_UNHIDE_CURSOR_ANSI RYCE_CSI RYCE_LITERAL("?25h")
#define RYCE_BUFFER_FMT RYCE_STR_FMT RYCE_MOVE_ANSI

// Numeric Contants
#define RYCE_ANSI_MOVE_COST 7
#define RYCE_ANSI_MOVE_BUFFER_SIZE 32
#define RYCE_MIN_BUFFER_SIZE 1024

// Visibility macros.
#ifndef RYCE_PUBLIC_DECL
#define RYCE_PUBLIC_DECL extern
#endif
#ifndef RYCE_PRIVATE_DECL
#define RYCE_PRIVATE_DECL static
#endif

// Error Codes.
typedef enum RYCE_TuiError {
    RYCE_TUI_ERR_NONE,                  ///< No error.
    RYCE_TUI_ERR_INVALID_TUI,           ///< Invalid TUI pointer.
    RYCE_TUI_ERR_INVALID_PANE,          ///< Invalid pane pointer.
    RYCE_TUI_ERR_INVALID_BUFFER,        ///< Invalid buffer pointer.
    RYCE_TUI_ERR_MOVE_BUFFER_OVERFLOW,  ///< Move buffer overflow.
    RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW, ///< Write buffer overflow.
    RYCE_TUI_ERR_STDOUT_FLUSH_FAILED,   ///< Failed to flush stdout.
} RYCE_TuiError;

/*
    Public API Structs
*/

typedef struct RYCE_CursorPosition {
    size_t x; ///< X coordinate on terminal.
    size_t y; ///< Y coordinate on terminal.
} RYCE_CursorPosition;

typedef struct RYCE_SkipSequence {
    int set;          ///< Flag to indicate if the buffer is set.
    size_t start_idx; ///< Start index of the buffer.
    size_t end_idx;   ///< End index of the buffer.
} RYCE_SkipSequence;

typedef struct RYCE_Pane {
    size_t width;      ///< Pane width in characters.
    size_t height;     ///< Pane height in characters.
    size_t length;     ///< Length of the update and back buffers.
    RYCE_CHAR *update; ///< Current modified buffer.
    RYCE_CHAR *cache;  ///< Last rendered buffer.
} RYCE_Pane;

typedef struct RYCE_TuiContext {
    RYCE_CursorPosition cursor;                        ///< Current cursor position.
    RYCE_CHAR move_buffer[RYCE_ANSI_MOVE_BUFFER_SIZE]; ///< Buffer to store move sequences.
    RYCE_Pane *pane;                                   ///< Current pane.
    size_t max_length;                                 ///< Maximum length of the write_buffer.
    size_t write_length;                               ///< Current length of the write_buffer.
    RYCE_CHAR *write_buffer;                           ///< Buffer that stores differences to be written.
} RYCE_TuiContext;

/*
    Public API Functions.
*/

/**
 * @brief Performs a `clear` or `cls` using ANSI escape sequences.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_clear_screen(void);

/**
 * @brief Initializes a pane and fills its buffers with default values.
 *
 * @param width Width of the pane.
 * @param height Height of the pane.
 * @return Pane* Pointer to the initialized pane.
 */
RYCE_PUBLIC_DECL RYCE_Pane *ryce_init_pane(size_t width, size_t height);

/**
 * @brief Initializes the Text UI Controller with a pane and a buffer to store changes.
 *
 * @param width Width of the interface to be rendered.
 * @param height Height of the interface to be rendered.
 * @return RYCE_TuiContext* Pointer to the initialized controller.
 */
RYCE_PUBLIC_DECL RYCE_TuiContext *ryce_init_tui_ctx(size_t width, size_t height);

/**
 * @brief Frees the memory allocated for a Pane.
 */
RYCE_PUBLIC_DECL void ryce_free_pane(RYCE_Pane *pane);

/**
 * @brief Frees the memory allocated for a TUI.
 */
RYCE_PUBLIC_DECL void ryce_free_ctx(RYCE_TuiContext *tui);

/**
 * @brief Renders the TUI and all panes to the terminal.

 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_render_tui(RYCE_TuiContext *tui);

#endif // RYCE_TUI_H

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_TUI_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_TUI_IMPL

#include <math.h> // log10

RYCE_PRIVATE_DECL size_t ryce_count_digits_internal(const size_t num) {
    return num == 0 ? 1 : (size_t)log10((double)num) + 1;
}

RYCE_PRIVATE_DECL bool ryce_is_pane_init_internal(RYCE_Pane *pane) {
    return (bool)(pane && pane->update && pane->cache);
}

RYCE_PRIVATE_DECL void ryce_softreset_controller_internal(RYCE_TuiContext *tui) {
    if (!tui) {
        return;
    }

    tui->write_length = 0;

    // Null-terminate the buffers.
    tui->write_buffer[0] = RYCE_NULL_CHAR;
    tui->move_buffer[0] = RYCE_NULL_CHAR;
}

RYCE_PRIVATE_DECL RYCE_TuiError ryce_write_move_internal(RYCE_TuiContext *tui, const size_t x, const size_t y) {
    if (!tui) {
        return RYCE_TUI_ERR_INVALID_TUI;
    }

    // Write the move sequence to the buffer.
    RYCE_SNPRINTF(tui->move_buffer, sizeof(tui->move_buffer) / sizeof(RYCE_CHAR), RYCE_MOVE_ANSI, (RYCE_SIZE_T)y + 1,
                  (RYCE_SIZE_T)x + 1);

    size_t seq_len = RYCE_STRLEN(tui->move_buffer);
    if (tui->write_length + seq_len >= tui->max_length) {
        return RYCE_TUI_ERR_MOVE_BUFFER_OVERFLOW;
    }

    // Append the move sequence to the write buffer.
    RYCE_MEMCPY(tui->write_buffer + tui->write_length, tui->move_buffer, seq_len);
    tui->write_length += seq_len;
    return RYCE_TUI_ERR_NONE;
}

RYCE_PRIVATE_DECL RYCE_TuiError ryce_inject_sequence_internal(RYCE_TuiContext *tui, const size_t x, const size_t y,
                                                              RYCE_SkipSequence *skip) {
    if (tui->cursor.y != y) {
        // Move the cursor to the correct roki.
        RYCE_TuiError err_code = ryce_write_move_internal(tui, x, y);
        if (err_code != RYCE_TUI_ERR_NONE) {
            return err_code;
        }

        skip->set = 0; // Reset the skip sequence.
        return RYCE_TUI_ERR_NONE;
    } else if (!skip->set || tui->cursor.x == x) {
        return RYCE_TUI_ERR_NONE;
    }

    size_t skipped_length = skip->end_idx - skip->start_idx;
    size_t expected = ryce_count_digits_internal(x) + ryce_count_digits_internal(y) + RYCE_ANSI_MOVE_COST;

    if (skipped_length && skipped_length < expected) {
        if (tui->write_length + skipped_length >= tui->max_length) {
            // Not enough space in the write buffer.
            return RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Reprint skipped due to being cheaper than moving cursor.
        RYCE_MEMCPY(tui->write_buffer + tui->write_length, tui->pane->update + skip->start_idx, skipped_length);
        tui->write_length += skipped_length;
    } else {
        // Move the cursor to the correct position.
        RYCE_TuiError err_code = ryce_write_move_internal(tui, x, y);
        if (err_code != RYCE_TUI_ERR_NONE) {
            return err_code;
        }
    }

    skip->set = 0; // Reset the skip sequence.
    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC_DECL RYCE_TuiError ryce_clear_screen(void) {
    RYCE_PRINTF(RYCE_CSI "2J" RYCE_CSI "0;0H");
    if (fflush(stdout) != 0) {
        return RYCE_TUI_ERR_STDOUT_FLUSH_FAILED;
    }

    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC_DECL RYCE_Pane *ryce_init_pane(const size_t width, const size_t height) {
    if (width == 0 || height == 0) {
        return nullptr;
    }

    size_t total_size = sizeof(RYCE_Pane) + ((width * height) * sizeof(RYCE_CHAR) * 2);
    RYCE_Pane *pane = (RYCE_Pane *)malloc(total_size);
    if (!pane) {
        return nullptr;
    }

    pane->width = width;
    pane->height = height;
    pane->length = width * height;
    pane->update = (RYCE_CHAR *)(pane + 1);
    pane->cache = pane->update + pane->length;

    // Fills the buffers with empty spaces.
    RYCE_MEMSET(pane->update, RYCE_EMPTY_CHAR, pane->length);
    RYCE_MEMSET(pane->cache, RYCE_EMPTY_CHAR, pane->length);

    return pane;
}

RYCE_PUBLIC_DECL RYCE_TuiContext *ryce_init_tui_ctx(const size_t width, const size_t height) {
    if (width == 0 || height == 0) {
        return nullptr;
    }

    RYCE_TuiContext *tui = (RYCE_TuiContext *)malloc(sizeof(RYCE_TuiContext));
    if (!tui) {
        return nullptr;
    }

    tui->cursor = (RYCE_CursorPosition){.x = width, .y = height};

    // Create the default pane.
    tui->pane = ryce_init_pane(width, height);
    if (!tui->pane) {
        free(tui);
        return nullptr;
    }

    // Create the buffer to store differences to be written.
    size_t default_length = (width * height) * 3;
    tui->max_length = default_length < RYCE_MIN_BUFFER_SIZE ? RYCE_MIN_BUFFER_SIZE : default_length;
    tui->write_buffer = (RYCE_CHAR *)malloc(tui->max_length * sizeof(RYCE_CHAR));
    if (!tui->write_buffer) {
        ryce_free_pane(tui->pane);
        free(tui);
        return nullptr;
    }

    tui->write_length = 0;
    RYCE_MEMSET(tui->write_buffer, RYCE_NULL_CHAR, tui->max_length);
    RYCE_MEMSET(tui->move_buffer, RYCE_NULL_CHAR, sizeof(tui->move_buffer) / sizeof(RYCE_CHAR));

    return tui;
}

RYCE_PUBLIC_DECL void ryce_free_pane(RYCE_Pane *pane) {
    if (!pane) {
        return;
    }

    free(pane);
}

RYCE_PUBLIC_DECL void ryce_free_ctx(RYCE_TuiContext *tui) {
    if (!tui) {
        return;
    }

    ryce_free_pane(tui->pane);
    free(tui->write_buffer);
    free(tui);
}

RYCE_PUBLIC_DECL RYCE_TuiError ryce_render_tui(RYCE_TuiContext *tui) {
    if (!tui) {
        return RYCE_TUI_ERR_INVALID_TUI;
    } else if (!ryce_is_pane_init_internal(tui->pane)) {
        return RYCE_TUI_ERR_INVALID_PANE;
    }

    // Reset the write and move sequence buffers.
    ryce_softreset_controller_internal(tui);
    RYCE_SkipSequence skip = {0, 0, 0};
    size_t x = 1;
    size_t y = 1;

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
        RYCE_TuiError seq_error = ryce_inject_sequence_internal(tui, x, y, &skip);
        if (seq_error != RYCE_TUI_ERR_NONE) {
            return seq_error;
        }

        // Check if we're next to the last written character.
        if (tui->write_length + 1 >= tui->max_length) {
            // Exceeded capacity for the write buffer.
            return RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Update buffer being written, propagate the change to the back buffer.
        tui->write_buffer[tui->write_length++] = tui->pane->update[i];
        tui->pane->cache[i] = tui->pane->update[i];
        tui->cursor.x = x;
        tui->cursor.y = y;
    }

    // Null-terminate for printf.
    size_t index = tui->write_length < tui->max_length ? tui->write_length : tui->max_length - 1;
    tui->write_buffer[index] = RYCE_NULL_CHAR;

    // Render the differences and move the cursor to the bottom of the pane.
    tui->cursor.x = tui->pane->width;
    tui->cursor.y = tui->pane->height;
    RYCE_PRINTF(RYCE_BUFFER_FMT, tui->write_buffer, (RYCE_SIZE_T)tui->cursor.y, (RYCE_SIZE_T)tui->cursor.x);
    if (fflush(stdout) != 0) {
        return RYCE_TUI_ERR_STDOUT_FLUSH_FAILED;
    }

    return RYCE_TUI_ERR_NONE;
}

#endif // RYCE_TUI_IMPL
