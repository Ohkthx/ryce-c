#ifndef RYCE_TUI_H
#define RYCE_TUI_H
#define RYCE_TUI_IMPL

/*
    RyCE TUI - A single-header, STB-styled terminal UI library.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_TUI_IMPL
       #include "tui.h"

    2) In as many other files as you need, just #include "tui.h"
       WITHOUT defining RYCE_TUI_IMPL.

    3) Compile and link all files together.

    Public API:
    - Structs:
        - RYCE_Glyph
        - RYCE_SkipSequence
        - RYCE_Pane
        - RYCE_TuiContext
    - Functions:
        - ryce_init_pane
        - ryce_init_tui_ctx
        - ryce_free_pane
        - ryce_free_tui_ctx
        - ryce_render_tui
        - ryce_coords_to_idx
        - ryce_clear_screen
        - ryce_clear_pane
*/

#include <stddef.h>
#include <stdio.h>
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
#define RYCE_PRINTF(...) wprintf(__VA_ARGS__)
#define RYCE_SNPRINTF(...) swprintf(__VA_ARGS__)
#define RYCE_STRLEN(s) wcslen(s)
#define RYCE_MEMSET(...) wmemset(__VA_ARGS__)

// Wide character format strings.
#define RYCE_LITERAL(s) L##s
#define RYCE_NUM_FMT RYCE_LITERAL("%lu")
#define RYCE_STR_FMT RYCE_LITERAL("%ls")

#else

#define RYCE_CHAR char
#define RYCE_SIZE_T size_t

// Standard character functions.
#define RYCE_PRINTF(...) printf(__VA_ARGS__)
#define RYCE_SNPRINTF(...) snprintf(__VA_ARGS__)
#define RYCE_STRLEN(s) strlen(s)
#define RYCE_MEMSET(...) memset(__VA_ARGS__)

// Standard character format strings.
#define RYCE_LITERAL(s) s
#define RYCE_NUM_FMT RYCE_LITERAL("%zu")
#define RYCE_STR_FMT RYCE_LITERAL("%s")

#endif // RYCE_WIDE_CHAR_SUPPORT

// String / Character constants and formatters.
#define RYCE_EMPTY_CHAR RYCE_LITERAL(' ')

// ANSI escape sequences.
#define RYCE_CSI RYCE_LITERAL("\x1b[")
#define RYCE_COLOR_ANSI_ONE RYCE_CSI RYCE_NUM_FMT RYCE_LITERAL("m")
#define RYCE_COLOR_ANSI_TWO RYCE_CSI RYCE_NUM_FMT RYCE_LITERAL(";") RYCE_NUM_FMT RYCE_LITERAL("m")
#define RYCE_MOVE_ANSI RYCE_CSI RYCE_NUM_FMT RYCE_LITERAL(";") RYCE_NUM_FMT RYCE_LITERAL("H")
#define RYCE_HIDE_CURSOR_ANSI RYCE_CSI RYCE_LITERAL("?25l")
#define RYCE_UNHIDE_CURSOR_ANSI RYCE_CSI RYCE_LITERAL("?25h")
#define RYCE_BUFFER_FMT RYCE_STR_FMT RYCE_MOVE_ANSI

#ifndef RYCE_ZERO
#define RYCE_ZERO 0
#endif // RYCE_ZERO

// Visibility macros.
#ifndef RYCE_PUBLIC_DECL
#define RYCE_PUBLIC_DECL extern
#endif // RYCE_PUBLIC_DECL
#ifndef RYCE_PRIVATE_DECL
#define RYCE_PRIVATE_DECL static
#endif // RYCE_PRIVATE_DECL

// Numeric Contants
enum {
    RYCE_WRITE_BUFFER_MULTIPLIER = 3, //< Multiplier for the write buffer.
    RYCE_ANSI_MOVE_COST = 7,          //< Cost of moving the cursor in ANSI escape sequences.
    RYCE_ANSI_BG_OFFSET = 10,         //< Offset for background colors in ANSI escape sequences.
    RYCE_ANSI_CODE_BUFFER_SIZE = 32,  //< Size of the ansi code buffer.
    RYCE_MIN_BUFFER_SIZE = 1024,      //< Minimum size of the write buffer.
};

// Error Codes.
typedef enum RYCE_TuiError {
    RYCE_TUI_ERR_NONE,                  ///< No error.
    RYCE_TUI_ERR_INVALID_TUI,           ///< Invalid TUI pointer.
    RYCE_TUI_ERR_INVALID_PANE,          ///< Invalid pane pointer.
    RYCE_TUI_ERR_INVALID_BUFFER,        ///< Invalid buffer pointer.
    RYCE_TUI_ERR_ANSI_BUFFER_OVERFLOW,  ///< Move buffer overflow.
    RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW, ///< Write buffer overflow.
    RYCE_TUI_ERR_STDOUT_FLUSH_FAILED,   ///< Failed to flush stdout.
} RYCE_TuiError;

// ANSI Color Codes.
typedef enum RYCE_Color_Code {
    RYCE_COLOR_BLACK = 30,
    RYCE_COLOR_RED = 31,
    RYCE_COLOR_GREEN = 32,
    RYCE_COLOR_YELLOW = 33,
    RYCE_COLOR_BLUE = 34,
    RYCE_COLOR_MAGENTA = 35,
    RYCE_COLOR_CYAN = 36,
    RYCE_COLOR_WHITE = 37,
    RYCE_COLOR_DEFAULT = 39,
} RYCE_Color_Code;

/*
    Public API Structs
*/

typedef struct RYCE_Glyph {
    RYCE_CHAR ch;             ///< Character to render.
    RYCE_Color_Code fg_color; ///< Foreground color.
    RYCE_Color_Code bg_color; ///< Background color.
} RYCE_Glyph;

typedef struct RYCE_SkipSequence {
    int set;          ///< Flag to indicate if the buffer is set.
    size_t start_idx; ///< Start index of the buffer.
    size_t end_idx;   ///< End index of the buffer.
} RYCE_SkipSequence;

typedef struct RYCE_Pane {
    size_t width;       ///< Pane width in characters.
    size_t height;      ///< Pane height in characters.
    size_t capacity;    ///< Capacity of the update and back buffers.
    RYCE_Glyph *update; ///< Current modified buffer.
    RYCE_Glyph *cache;  ///< Last rendered buffer.
} RYCE_Pane;

typedef struct RYCE_TuiContext {
    size_t cursor_x;                                   ///< Current cursor x position.
    size_t cursor_y;                                   ///< Current cursor y position.
    RYCE_CHAR ansi_buffer[RYCE_ANSI_CODE_BUFFER_SIZE]; ///< Buffer to store move sequences.
    RYCE_Pane *pane;                                   ///< Current pane.
    size_t capacity;                                   ///< Maximum capacity of the write_buffer.
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
RYCE_PUBLIC_DECL void ryce_free_tui_ctx(RYCE_TuiContext *tui);

/**
 * @brief Renders the TUI and all panes to the terminal.

 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_render_tui(RYCE_TuiContext *tui);

/**
 * @brief Converts 2D coordinates to a 1D index in a flattened buffer.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param width Width of the buffer.
 * @return size_t Index in the buffer.
 */
RYCE_PUBLIC_DECL inline size_t ryce_ciords_to_idx(size_t x, size_t y, size_t width);

/**
 * @brief Clears the update and cache buffers of a pane.
 *
 * @param pane Pane to clear.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_clear_pane(RYCE_Pane *pane);

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

#include <math.h>   // log10
#include <stdlib.h> // calloc, free

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
    tui->write_buffer[0] = RYCE_ZERO;
    tui->ansi_buffer[0] = RYCE_ZERO;
}

RYCE_PRIVATE_DECL RYCE_TuiError ryce_write_color_internal(RYCE_TuiContext *tui, const size_t idx,
                                                          RYCE_Color_Code fg_color, RYCE_Color_Code bg_color) {
    if (!tui) {
        return RYCE_TUI_ERR_INVALID_TUI;
    }

    // Decide what changes need to be made.
    int change_fg = tui->pane->update[idx].fg_color != fg_color;
    int change_bg = tui->pane->update[idx].bg_color != bg_color;
    if (!change_fg && !change_bg) {
        return RYCE_TUI_ERR_NONE;
    }

    // Write the color sequence to the buffer for both foreground and background.
    if (change_fg && change_bg) {
        RYCE_SNPRINTF(tui->ansi_buffer, sizeof(tui->ansi_buffer) / sizeof(RYCE_CHAR), RYCE_COLOR_ANSI_TWO,
                      (RYCE_SIZE_T)tui->pane->update[idx].fg_color, (RYCE_SIZE_T)tui->pane->update[idx].bg_color);
    } else {
        RYCE_Color_Code color_change = tui->pane->update[idx].fg_color;
        if (change_bg) {
            color_change = tui->pane->update[idx].bg_color + RYCE_ANSI_BG_OFFSET;
        }

        // Write the color sequence to the buffer for either foreground or background.
        RYCE_SNPRINTF(tui->ansi_buffer, sizeof(tui->ansi_buffer) / sizeof(RYCE_CHAR), RYCE_COLOR_ANSI_ONE,
                      (RYCE_SIZE_T)color_change);
    }

    size_t seq_len = RYCE_STRLEN(tui->ansi_buffer);
    if (tui->write_length + seq_len >= tui->capacity) {
        return RYCE_TUI_ERR_ANSI_BUFFER_OVERFLOW;
    }

    // Append the color sequence to the write buffer.
    memcpy(tui->write_buffer + tui->write_length, tui->ansi_buffer, seq_len * sizeof(RYCE_CHAR));
    tui->write_length += seq_len;
    return RYCE_TUI_ERR_NONE;
}

RYCE_PRIVATE_DECL RYCE_TuiError ryce_write_move_internal(RYCE_TuiContext *tui, const size_t x, const size_t y) {
    if (!tui) {
        return RYCE_TUI_ERR_INVALID_TUI;
    }

    // Write the move sequence to the buffer.
    RYCE_SNPRINTF(tui->ansi_buffer, sizeof(tui->ansi_buffer) / sizeof(RYCE_CHAR), RYCE_MOVE_ANSI, (RYCE_SIZE_T)y + 1,
                  (RYCE_SIZE_T)x + 1);

    size_t seq_len = RYCE_STRLEN(tui->ansi_buffer);
    if (tui->write_length + seq_len >= tui->capacity) {
        return RYCE_TUI_ERR_ANSI_BUFFER_OVERFLOW;
    }

    // Append the move sequence to the write buffer.
    memcpy(tui->write_buffer + tui->write_length, tui->ansi_buffer, seq_len * sizeof(RYCE_CHAR));
    tui->write_length += seq_len;
    return RYCE_TUI_ERR_NONE;
}

RYCE_PRIVATE_DECL RYCE_TuiError ryce_inject_sequence_internal(RYCE_TuiContext *tui, const size_t x, const size_t y,
                                                              RYCE_SkipSequence *skip) {
    if (tui->cursor_y != y) {
        // Move the cursor to the correct roki.
        RYCE_TuiError err_code = ryce_write_move_internal(tui, x, y);
        if (err_code != RYCE_TUI_ERR_NONE) {
            return err_code;
        }

        skip->set = 0; // Reset the skip sequence.
        return RYCE_TUI_ERR_NONE;
    } else if (!skip->set || tui->cursor_x == x) {
        return RYCE_TUI_ERR_NONE;
    }

    size_t skipped_length = skip->end_idx - skip->start_idx;
    size_t expected = ryce_count_digits_internal(x) + ryce_count_digits_internal(y) + RYCE_ANSI_MOVE_COST;

    if (skipped_length && skipped_length < expected) {
        if (tui->write_length + skipped_length >= tui->capacity) {
            // Not enough space in the write buffer.
            return RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Reprint skipped due to being cheaper than moving cursor.
        for (size_t i = 0; i < skipped_length; i++) {
            tui->write_buffer[tui->write_length + i] = tui->pane->update[skip->start_idx + i].ch;
        }

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

    size_t total_size = sizeof(RYCE_Pane) + ((width * height) * sizeof(RYCE_Glyph) * 2);
    RYCE_Pane *pane = (RYCE_Pane *)calloc(1, total_size);
    if (!pane) {
        return nullptr;
    }

    pane->width = width;
    pane->height = height;
    pane->capacity = width * height;

    pane->update = (RYCE_Glyph *)(pane + 1);
    pane->cache = pane->update + pane->capacity;

    const RYCE_Glyph default_glyph = {
        .ch = RYCE_EMPTY_CHAR, .fg_color = RYCE_COLOR_DEFAULT, .bg_color = RYCE_COLOR_DEFAULT + RYCE_ANSI_BG_OFFSET};

    for (size_t i = 0; i < pane->capacity; i++) {
        pane->update[i] = default_glyph;
        pane->cache[i] = default_glyph;
    }

    return pane;
}

RYCE_PUBLIC_DECL RYCE_TuiContext *ryce_init_tui_ctx(const size_t width, const size_t height) {
    if (width == 0 || height == 0) {
        return nullptr;
    }

    RYCE_TuiContext *tui = (RYCE_TuiContext *)calloc(1, sizeof(RYCE_TuiContext));
    if (!tui) {
        return nullptr;
    }

    tui->cursor_x = width;
    tui->cursor_y = height;
    tui->write_length = 0;

    // Create the default pane.
    tui->pane = ryce_init_pane(width, height);
    if (!tui->pane) {
        free(tui);
        return nullptr;
    }

    // Create the buffer to store differences to be written.
    size_t default_length = (width * height) * RYCE_WRITE_BUFFER_MULTIPLIER;
    tui->capacity = default_length < RYCE_MIN_BUFFER_SIZE ? RYCE_MIN_BUFFER_SIZE : default_length;
    tui->write_buffer = (RYCE_CHAR *)calloc(tui->capacity, sizeof(RYCE_CHAR));
    if (!tui->write_buffer) {
        ryce_free_pane(tui->pane);
        free(tui);
        return nullptr;
    }

    return tui;
}

RYCE_PUBLIC_DECL void ryce_free_pane(RYCE_Pane *pane) {
    if (!pane) {
        return;
    }

    free(pane);
}

RYCE_PUBLIC_DECL void ryce_free_tui_ctx(RYCE_TuiContext *tui) {
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
    RYCE_Color_Code active_fg = tui->pane[0].cache->fg_color; // Last drawn fg color stored here.
    RYCE_Color_Code active_bg = tui->pane[0].cache->bg_color; // Last drawn bg color stored here.
    RYCE_TuiError error = RYCE_TUI_ERR_NONE;

    for (size_t i = 0; i < tui->pane->capacity; i++) {
        const RYCE_Glyph *old_glyph = &tui->pane->cache[i];
        const RYCE_Glyph *new_glyph = &tui->pane->update[i];

        if (new_glyph->ch == old_glyph->ch && new_glyph->fg_color == old_glyph->fg_color &&
            new_glyph->bg_color == old_glyph->bg_color) {
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

        // Inject the color code sequence if there is a change of color.
        if (new_glyph->fg_color != active_fg || new_glyph->bg_color != active_bg) {
            error = ryce_write_color_internal(tui, i, active_fg, active_bg);
            if (error != RYCE_TUI_ERR_NONE) {
                return error;
            }

            // Update the active terminal color.
            active_fg = new_glyph->fg_color;
            active_bg = new_glyph->bg_color;
        }

        // Inject move sequences or reprinted characters.
        error = ryce_inject_sequence_internal(tui, x, y, &skip);
        if (error != RYCE_TUI_ERR_NONE) {
            return error;
        }

        // Check if we're next to the last written character.
        if (tui->write_length + 1 >= tui->capacity) {
            // Exceeded capacity for the write buffer.
            return RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Update buffer being written, propagate the change to the back buffer.
        tui->write_buffer[tui->write_length++] = tui->pane->update[i].ch;
        tui->pane->cache[i] = tui->pane->update[i];
        tui->cursor_x = x;
        tui->cursor_y = y;
    }

    // Null-terminate for printf and store the last colors into the first glyph.
    size_t index = tui->write_length < tui->capacity ? tui->write_length : tui->capacity - 1;
    tui->write_buffer[index] = RYCE_ZERO;
    tui->pane->cache[0].fg_color = active_fg;
    tui->pane->cache[0].bg_color = active_bg;

    // Render the differences and move the cursor to the bottom of the pane.
    tui->cursor_x = tui->pane->width;
    tui->cursor_y = tui->pane->height;
    RYCE_PRINTF(RYCE_BUFFER_FMT, tui->write_buffer, (RYCE_SIZE_T)tui->cursor_y, (RYCE_SIZE_T)tui->cursor_x);
    if (fflush(stdout) != 0) {
        return RYCE_TUI_ERR_STDOUT_FLUSH_FAILED;
    }

    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC_DECL inline size_t ryce_coords_to_idx(const size_t x, const size_t y, const size_t width) {
    return (y * width) + x;
}

RYCE_PUBLIC_DECL RYCE_TuiError ryce_clear_pane(RYCE_Pane *pane) {
    if (!pane) {
        return RYCE_TUI_ERR_INVALID_PANE;
    }

    for (size_t i = 0; i < pane->capacity; i++) {
        if (pane->update[i].ch == RYCE_EMPTY_CHAR && pane->update[i].fg_color == RYCE_COLOR_DEFAULT &&
            pane->update[i].bg_color == RYCE_COLOR_DEFAULT + RYCE_ANSI_BG_OFFSET) {
            continue;
        }

        // Non-default character, reset to default.
        pane->update[i] = (RYCE_Glyph){.ch = RYCE_EMPTY_CHAR,
                                       .fg_color = RYCE_COLOR_DEFAULT,
                                       .bg_color = RYCE_COLOR_DEFAULT + RYCE_ANSI_BG_OFFSET};
    }

    return RYCE_TUI_ERR_NONE;
}

#endif // RYCE_TUI_IMPL
