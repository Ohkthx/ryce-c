#if defined(RYCE_IMPL) && !defined(RYCE_TUI_IMPL)
#define RYCE_TUI_IMPL
#endif
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

    Public API:
    - Structs:
        - RYCE_Glyph
        - RYCE_Pane
        - RYCE_TuiContext
    - Functions:
        - ryce_init_pane
        - ryce_init_tui_ctx
        - ryce_render_tui
        - ryce_pane_set
        - ryce_pane_set_str
        - ryce_move_cursor
        - ryce_clear_screen
        - ryce_clear_pane
*/
#define RYCE_TUI_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

// Standard character format strings.
#define RYCE_LITERAL(s) s
#define RYCE_NUM_FMT RYCE_LITERAL("%zu")
#define RYCE_STR_FMT RYCE_LITERAL("%s")

#endif // RYCE_WIDE_CHAR_SUPPORT

// String / Character constants and formatters.
#define RYCE_EMPTY_CHAR RYCE_LITERAL(' ')

// ANSI escape sequences.
#define RYCE_CSI RYCE_LITERAL("\x9b")
#define RYCE_MOVE_ANSI RYCE_CSI RYCE_NUM_FMT RYCE_LITERAL(";") RYCE_NUM_FMT RYCE_LITERAL("H")
#define RYCE_HIDE_CURSOR_ANSI RYCE_CSI RYCE_LITERAL("?25l")
#define RYCE_UNHIDE_CURSOR_ANSI RYCE_CSI RYCE_LITERAL("?25h")
#define RYCE_BUFFER_FMT RYCE_STR_FMT RYCE_MOVE_ANSI

#ifndef RYCE_ZERO
#define RYCE_ZERO 0
#endif // RYCE_ZERO

// ---------------------------------------------------------------------//
// BEGIN Screen Size & Buffer Definititons.
#ifndef RYCE_SCREEN_WIDTH
#define RYCE_SCREEN_WIDTH 256
#endif // RYCE_SCREEN_WIDTH

#ifndef RYCE_SCREEN_HEIGHT
#define RYCE_SCREEN_HEIGHT 256
#endif // RYCE_SCREEN_HEIGHT

#define RYCE_SCREEN_BUFFER_CAPACITY (size_t)(RYCE_SCREEN_WIDTH * RYCE_SCREEN_HEIGHT)
#define RYCE_WRITE_BUFFER_CAPACITY (RYCE_SCREEN_BUFFER_CAPACITY * 3)
// END Screen Size & Buffer Definititons.
// ---------------------------------------------------------------------//

// Numeric Contants
enum {
    RYCE_ANSI_MOVE_COST = 3,         //< Cost of moving the cursor in ANSI escape sequences.
    RYCE_ANSI_CODE_BUFFER_SIZE = 64, //< Size of the ansi code buffer.
};

// Error Codes.
typedef enum RYCE_TuiError {
    RYCE_TUI_ERR_NONE,                  //< No error.
    RYCE_TUI_ERR_INVALID_TUI,           //< Invalid TUI pointer.
    RYCE_TUI_ERR_INVALID_PANE,          //< Invalid pane pointer.
    RYCE_TUI_ERR_INVALID_BUFFER,        //< Invalid buffer pointer.
    RYCE_TUI_ERR_ANSI_BUFFER_OVERFLOW,  //< Move buffer overflow.
    RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW, //< Write buffer overflow.
    RYCE_TUI_ERR_STDOUT_FLUSH_FAILED,   //< Failed to flush stdout.
    RYCE_TUI_ERR_UNKNOWN_STYLE,         //< Unknown style flag.
    RYCE_TUI_ERR_INVALID_DIMENSIONS,    //< Invalid dimensions.
    RYCE_TUI_ERR_INVALID_COORDINATES,   //< Invalid coordinates.
} RYCE_TuiError;

// ANSI Color Codes.
typedef enum RYCE_Style_Color_Code {
    RYCE_STYLE_COLOR_DEFAULT = 0, //< Default color.
    RYCE_STYLE_COLOR_BLACK = 1,   //< Black color.
    RYCE_STYLE_COLOR_RED = 2,     //< Red color.
    RYCE_STYLE_COLOR_GREEN = 3,   //< Green color.
    RYCE_STYLE_COLOR_YELLOW = 4,  //< Yellow color.
    RYCE_STYLE_COLOR_BLUE = 5,    //< Blue color.
    RYCE_STYLE_COLOR_MAGENTA = 6, //< Magenta color.
    RYCE_STYLE_COLOR_CYAN = 7,    //< Cyan color.
    RYCE_STYLE_COLOR_WHITE = 8,   //< White color.
} RYCE_Style_Color_Code;

// ANSI Style Codes.
typedef enum RYCE_Style_Code {
    RYCE_STYLE_MODIFIER_DEFAULT = 0,            //< Default style.
    RYCE_STYLE_MODIFIER_BOLD = 1 << 0,          //< Bold style.
    RYCE_STYLE_MODIFIER_DIM = 1 << 1,           //< Dim style.
    RYCE_STYLE_MODIFIER_ITALIC = 1 << 3,        //< Italic style.
    RYCE_STYLE_MODIFIER_UNDERLINE = 1 << 4,     //< Underline style.
    RYCE_STYLE_MODIFIER_BLINK = 1 << 5,         //< Blink style.
    RYCE_STYLE_MODIFIER_REVERSE = 1 << 7,       //< Reverse style.
    RYCE_STYLE_MODIFIER_HIDDEN = 1 << 8,        //< Hidden style.
    RYCE_STYLE_MODIFIER_STRIKETHROUGH = 1 << 9, //< Strikethrough style.
} RYCE_Style_Code;

/*
    Public API Structs
*/

typedef union RYCE_Style {
    struct {
        uint8_t fg_color;     //< Foreground color. [n & 0xFF]
        uint8_t bg_color;     //< Background color. [(n >> 8) & 0xFF]
        uint16_t style_flags; //< Style flags. [(n >> 16) & 0xFFFF]
    } part;
    struct {
        uint16_t color; //< Color flags.
        uint16_t style; //< Style flags.
    } flag;
    uint32_t value;
} RYCE_Style;

typedef struct RYCE_Glyph {
    RYCE_CHAR ch;     ///< Character to render.
    RYCE_Style style; ///< Style of the character.
} RYCE_Glyph;

typedef struct RYCE_TuiContext {
    struct {
        int64_t x;       ///< X coordinate.
        int64_t y;       ///< Y coordinate.
        uint32_t width;  ///< Width.
        uint32_t height; ///< Height.
    } view;              ///< View rectangle.
    struct {
        int64_t x; ///< X position.
        int64_t y; ///< Y position.
    } cursor;
    RYCE_Style style;                                  ///< Current style.
    size_t pane_count;                                 ///< Number of panes.
    RYCE_CHAR ansi_buffer[RYCE_ANSI_CODE_BUFFER_SIZE]; ///< Buffer to store move sequences.
    struct {
        uint64_t length;                              ///< Current length of the write_buffer.
        RYCE_CHAR buffer[RYCE_WRITE_BUFFER_CAPACITY]; ///< Buffer that stores differences to be written.
    } write;
    RYCE_Glyph update[RYCE_SCREEN_BUFFER_CAPACITY];  ///< Current modified buffer.
    RYCE_Glyph cache[RYCE_SCREEN_BUFFER_CAPACITY];   ///< Last rendered buffer.
    size_t render_mask[RYCE_SCREEN_BUFFER_CAPACITY]; ///< Mask to track rendered cells.
} RYCE_TuiContext;

typedef struct RYCE_Pane {
    size_t id; //< Pane ID.
    struct {
        int64_t x;        ///< X coordinate.
        int64_t y;        ///< Y coordinate.
        uint32_t width;   ///< Width.
        uint32_t height;  ///< Height.
    } view;               ///< View rectangle.
    RYCE_TuiContext *ctx; ///< Pointer to the TUI context.
} RYCE_Pane;

/*
    Public API Structs Constants
*/

/**
 * @brief Default style with default colors and no styling.
 */
const RYCE_Style RYCE_DEFAULT_STYLE = {
    .part =
        {
            .fg_color = RYCE_STYLE_COLOR_DEFAULT,
            .bg_color = RYCE_STYLE_COLOR_DEFAULT,
            .style_flags = RYCE_STYLE_MODIFIER_DEFAULT,
        },
};

/**
 * @brief Default glyph with empty value and styling.
 */
const RYCE_Glyph RYCE_DEFAULT_GLYPH = {
    .ch = RYCE_EMPTY_CHAR,
    .style = RYCE_DEFAULT_STYLE,
};

/*
    Public API Functions.
*/

/**
 * @brief Initializes a pane and fills its buffers with default values.
 *
 * @param x X coordinate of the pane.
 * @param y Y coordinate of the pane.
 * @param width Width of the pane.
 * @param height Height of the pane.
 * @param ctx Pointer to the TUI context.
 * @param out Pointer to the pane to be initialized.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_init_pane(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                              RYCE_TuiContext *ctx, RYCE_Pane *out);

/**
 * @brief Initializes the Text UI Controller with a pane and a buffer to store changes.
 *
 * @param width Width of the interface to be rendered.
 * @param height Height of the interface to be rendered.
 * @param out Pointer to the TUI context to be initialized.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_init_tui_ctx(uint32_t width, uint32_t height, RYCE_TuiContext *out);

/**
 * @brief Renders the TUI and all panes to the terminal.

 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_render_tui(RYCE_TuiContext *tui);

/**
 * @brief Sets a character in the pane at the specified coordinates.
 *
 * @param pane Pointer to the pane.
 * @param x X coordinate of the character.
 * @param y Y coordinate of the character.
 * @param glyph Pointer to the glyph to be set.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_pane_set(RYCE_Pane *pane, uint32_t x, uint32_t y, const RYCE_Glyph *glyph);

/** * @brief Sets a character in the pane at the specified coordinates using a style.
 *
 * @param pane Pointer to the pane.
 * @param x X coordinate of the character.
 * @param y Y coordinate of the character.
 * @param style Pointer to the style to be applied.
 * @param ch Pointer to the character to be set.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_pane_set_str(RYCE_Pane *pane, uint32_t x, uint32_t y, const RYCE_Style *style,
                                                 RYCE_CHAR *ch);

/** * @brief Moves the cursor to a specific position using ANSI escape sequences.
 *
 * @param tui Pointer to the TUI context.
 * @param x X coordinate of the cursor.
 * @param y Y coordinate of the cursor.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_move_cursor(RYCE_TuiContext *tui, int64_t x, int64_t y);

/**
 * @brief Performs a `clear` or `cls` using ANSI escape sequences.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_clear_screen(void);

/**
 * @brief Clears the update and cache buffers of a pane.
 *
 * @param pane Pane to clear.
 * @return RYCE_TuiError RYCE_TUI_ERR_NONE if successful, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_TuiError ryce_clear_pane(RYCE_Pane *pane);

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

#ifndef RYCE_ARRAY_LEN
#define RYCE_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#endif // RYCE_ARRAY_LEN

#ifdef RYCE_WIDE_CHAR_SUPPORT
#include <locale.h> // setlocale, LC_ALL
#endif

typedef struct RYCE_SkipSequence {
    bool set;           ///< Flag to indicate if the buffer is set.
    uint64_t start_idx; ///< Start index of the buffer.
    uint64_t end_idx;   ///< End index of the buffer.
} RYCE_SkipSequence;

/**
 * @brief ANSI escape sequences for colors.
 */
static const struct {
    uint8_t bit; // Bitmask for the color flag.
    int fg_code; // ANSI code for the foreground color.
    int bg_code; // ANSI code for the background color.
} COLOR_MAP[] = {
    {RYCE_STYLE_COLOR_DEFAULT, 39, 49}, {RYCE_STYLE_COLOR_BLACK, 30, 40},  {RYCE_STYLE_COLOR_RED, 31, 41},
    {RYCE_STYLE_COLOR_GREEN, 32, 42},   {RYCE_STYLE_COLOR_YELLOW, 33, 43}, {RYCE_STYLE_COLOR_BLUE, 34, 44},
    {RYCE_STYLE_COLOR_MAGENTA, 35, 45}, {RYCE_STYLE_COLOR_CYAN, 36, 46},   {RYCE_STYLE_COLOR_WHITE, 37, 47},
};

/**
 * @brief ANSI escape sequences for styling.
 */
static const struct {
    uint16_t bit; // Bitmask for the style flag.
    int on_code;  // ANSI code to turn on the style.
    int off_code; // ANSI code to turn off the style.
} STYLE_MAP[] = {
    {RYCE_STYLE_MODIFIER_BOLD, 1, 22},   {RYCE_STYLE_MODIFIER_DIM, 2, 22},
    {RYCE_STYLE_MODIFIER_ITALIC, 3, 23}, {RYCE_STYLE_MODIFIER_UNDERLINE, 4, 24},
    {RYCE_STYLE_MODIFIER_BLINK, 5, 25},  {RYCE_STYLE_MODIFIER_REVERSE, 7, 27},
    {RYCE_STYLE_MODIFIER_HIDDEN, 8, 28}, {RYCE_STYLE_MODIFIER_STRIKETHROUGH, 9, 29},
};

RYCE_PRIVATE inline size_t ryce_count_digits_internal(const size_t num) {
    return num == 0 ? 1 : (size_t)log10((double)num) + 1;
}

RYCE_PRIVATE inline void ryce_softreset_controller_internal(RYCE_TuiContext *tui) {
    tui->write.length = 0;

    // Null-terminate the buffers.
    tui->write.buffer[0] = RYCE_ZERO;
    tui->ansi_buffer[0] = RYCE_ZERO;
}

RYCE_PRIVATE inline RYCE_TuiError ryce_print_write_buffer_internal(RYCE_TuiContext *tui) {
    // Null-terminate the write buffer.
    size_t index = tui->write.length < RYCE_WRITE_BUFFER_CAPACITY ? tui->write.length : RYCE_WRITE_BUFFER_CAPACITY - 1;
    tui->write.buffer[index] = RYCE_ZERO;

    if (tui->cursor.x == tui->view.width || tui->cursor.y == tui->view.height) {
        RYCE_PRINTF(RYCE_BUFFER_FMT, tui->write.buffer, (RYCE_SIZE_T)tui->cursor.y, (RYCE_SIZE_T)tui->cursor.x);
    } else {
        RYCE_PRINTF(RYCE_STR_FMT, tui->write.buffer);
    }

    if (fflush(stdout) != 0) {
        return RYCE_TUI_ERR_STDOUT_FLUSH_FAILED;
    }

    return RYCE_TUI_ERR_NONE;
}

RYCE_PRIVATE inline RYCE_TuiError ryce_write_style_internal(RYCE_TuiContext *tui, RYCE_Style *new_style) {
    RYCE_Style old_style = tui->style;
    bool changed = false;

    if (new_style->value == old_style.value) {
        return RYCE_TUI_ERR_NONE;
    }

    // Prefix the ANSI buffer with the CSI sequence.
    RYCE_SNPRINTF(tui->ansi_buffer, RYCE_ANSI_CODE_BUFFER_SIZE, RYCE_STR_FMT, RYCE_CSI);

    // Update the foreground color.
    if (new_style->part.fg_color != old_style.part.fg_color) {
        changed = true;
        RYCE_SNPRINTF(tui->ansi_buffer + RYCE_STRLEN(tui->ansi_buffer),
                      RYCE_ANSI_CODE_BUFFER_SIZE - RYCE_STRLEN(tui->ansi_buffer), RYCE_LITERAL("%d;"),
                      COLOR_MAP[new_style->part.fg_color].fg_code);
    }

    // Update the background color.
    if (new_style->part.bg_color != old_style.part.bg_color) {
        changed = true;
        RYCE_SNPRINTF(tui->ansi_buffer + RYCE_STRLEN(tui->ansi_buffer),
                      RYCE_ANSI_CODE_BUFFER_SIZE - RYCE_STRLEN(tui->ansi_buffer), RYCE_LITERAL("%d;"),
                      COLOR_MAP[new_style->part.bg_color].bg_code);
    }

    // Update the style flags.
    uint16_t diff = new_style->part.style_flags ^ old_style.part.style_flags;
    for (size_t i = 0; i < RYCE_ARRAY_LEN(STYLE_MAP); i++) {
        if ((diff & STYLE_MAP[i].bit) != 0) {
            // Determine if the style is on or off.
            changed = true;
            bool is_on = (bool)((new_style->part.style_flags & STYLE_MAP[i].bit) != 0);
            int code = (int)is_on ? STYLE_MAP[i].on_code : STYLE_MAP[i].off_code;

            // Append the ANSI code to the buffer.
            RYCE_SNPRINTF(tui->ansi_buffer + RYCE_STRLEN(tui->ansi_buffer),
                          sizeof(tui->ansi_buffer) - RYCE_STRLEN(tui->ansi_buffer), RYCE_LITERAL("%d;"), code);
        }
    }

    if (!changed) {
        // If nothing changed, just return.
        return RYCE_TUI_ERR_NONE;
    }

    // Replace trailing ';' with 'm'
    size_t len = RYCE_STRLEN(tui->ansi_buffer);
    if (len > 0 && tui->ansi_buffer[len - 1] == RYCE_LITERAL(';')) {
        tui->ansi_buffer[len - 1] = RYCE_LITERAL('m');
    } else {
        // In a weird corner case, if the buffer ended exactly, just append 'm'
        RYCE_SNPRINTF(tui->ansi_buffer + len, RYCE_ANSI_CODE_BUFFER_SIZE - len, RYCE_LITERAL("m"));
    }

    size_t seq_len = RYCE_STRLEN(tui->ansi_buffer);
    if (tui->write.length + seq_len >= RYCE_WRITE_BUFFER_CAPACITY) {
        return RYCE_TUI_ERR_ANSI_BUFFER_OVERFLOW;
    }

    // Add the ANSI sequence to the write buffer and update the styling.
    memcpy(tui->write.buffer + tui->write.length, tui->ansi_buffer, seq_len * sizeof(RYCE_CHAR));
    tui->write.length += seq_len;
    return RYCE_TUI_ERR_NONE;
}

RYCE_PRIVATE inline RYCE_TuiError ryce_write_move_internal(RYCE_TuiContext *tui, const size_t x, const size_t y) {
    // Write the move sequence to the buffer.
    RYCE_SNPRINTF(tui->ansi_buffer, sizeof(tui->ansi_buffer) / sizeof(RYCE_CHAR), RYCE_MOVE_ANSI, (RYCE_SIZE_T)y + 1,
                  (RYCE_SIZE_T)x + 1);

    size_t seq_len = RYCE_STRLEN(tui->ansi_buffer);
    if (tui->write.length + seq_len >= RYCE_WRITE_BUFFER_CAPACITY) {
        return RYCE_TUI_ERR_ANSI_BUFFER_OVERFLOW;
    }

    // Append the move sequence to the write buffer.
    memcpy(tui->write.buffer + tui->write.length, tui->ansi_buffer, seq_len * sizeof(RYCE_CHAR));
    tui->write.length += seq_len;
    return RYCE_TUI_ERR_NONE;
}

RYCE_PRIVATE inline RYCE_TuiError ryce_inject_sequence_internal(RYCE_TuiContext *tui, const uint32_t x,
                                                                const uint32_t y, RYCE_SkipSequence *skip) {
    if (tui->cursor.y != y) {
        // Move the cursor to the correct roki.
        RYCE_TuiError err_code = ryce_write_move_internal(tui, x, y);
        if (err_code != RYCE_TUI_ERR_NONE) {
            return err_code;
        }

        skip->set = false; // Reset the skip sequence.
        return RYCE_TUI_ERR_NONE;
    } else if (!skip->set || tui->cursor.x == x) {
        return RYCE_TUI_ERR_NONE;
    }

    size_t skipped_length = skip->end_idx - skip->start_idx;
    size_t expected = ryce_count_digits_internal(x) + ryce_count_digits_internal(y) + RYCE_ANSI_MOVE_COST;

    if (skipped_length && skipped_length < expected) {
        if (tui->write.length + skipped_length >= RYCE_WRITE_BUFFER_CAPACITY) {
            // Not enough space in the write buffer.
            return RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Reprint skipped due to being cheaper than moving cursor.
        for (size_t i = 0; i < skipped_length; i++) {
            tui->write.buffer[tui->write.length + i] = tui->update[skip->start_idx + i].ch;
        }

        tui->write.length += skipped_length;
    } else {
        // Move the cursor to the correct position.
        RYCE_TuiError err_code = ryce_write_move_internal(tui, x, y);
        if (err_code != RYCE_TUI_ERR_NONE) {
            return err_code;
        }
    }

    skip->set = false; // Reset the skip sequence.
    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC RYCE_TuiError ryce_init_pane(uint32_t x, uint32_t y, const uint32_t width, const uint32_t height,
                                         RYCE_TuiContext *ctx, RYCE_Pane *out) {
    if (width == 0 || height == 0) {
        return RYCE_TUI_ERR_INVALID_DIMENSIONS;
    }

    out->id = ctx->pane_count++;
    out->view.x = x;
    out->view.y = y;
    out->view.width = width;
    out->view.height = height;
    out->ctx = ctx;

    // Initialize the mask to contain the pane's ID.
    // Initialize the data for the pane to be empty.
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            // Convert from relative coordinates to absolute coordinates.
            uint32_t gx = x + col;
            uint32_t gy = y + row;

            if (gx >= ctx->view.width || gy >= ctx->view.height) {
                // Out of bounds, skip remainder of 'x' glyphs.
                continue;
            }

            // Flattened index in the TUI's row-major buffer.
            size_t idx = (gy * ctx->view.width) + gx;

            // Assign to the pane’s ID.
            ctx->render_mask[idx] = out->id;
            ctx->update[idx] = RYCE_DEFAULT_GLYPH;
        }
    }

    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC RYCE_TuiError ryce_init_tui_ctx(const uint32_t width, const uint32_t height, RYCE_TuiContext *out) {
    if (width == 0 || height == 0) {
        return RYCE_TUI_ERR_INVALID_DIMENSIONS;
    }

    out->cursor.x = width;
    out->cursor.y = height;
    out->view.x = 0;
    out->view.y = 0;
    out->view.width = width;
    out->view.height = height;
    out->pane_count = 0;
    out->style = RYCE_DEFAULT_STYLE;
    out->write.length = 0;

    // Initialize the mask and update buffers to contain the default values.
    for (size_t i = 0; i < RYCE_SCREEN_BUFFER_CAPACITY; i++) {
        out->render_mask[i] = 0;
        out->update[i] = RYCE_DEFAULT_GLYPH;
    }

#ifdef RYCE_WIDE_CHAR_SUPPORT
    setlocale(LC_ALL, "");
#endif
#ifdef RYCE_HIDE_CURSOR
    RYCE_PRINTF(RYCE_HIDE_CURSOR_ANSI);
#endif
    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC RYCE_TuiError ryce_render_tui(RYCE_TuiContext *tui) {
    // Reset the write and move sequence buffers.
    ryce_softreset_controller_internal(tui);
    RYCE_SkipSequence skip = {.set = false, .start_idx = 0, .end_idx = 0};
    int64_t x = 1;
    int64_t y = 1;
    RYCE_TuiError error = RYCE_TUI_ERR_NONE;

    for (uint32_t i = 0; i < RYCE_SCREEN_BUFFER_CAPACITY; i++) {
        x = i % tui->view.width; // X position in flattened buffer.
        y = i / tui->view.width; // Y position in flattened buffer.

        if (x >= tui->view.width || y >= tui->view.height) {
            // Out of bounds, skip remainder of 'x' glyphs.
            i = ((y + 1) * tui->view.width) - 1;
            continue;
        }

        const RYCE_Glyph *old_glyph = &tui->cache[i];
        const RYCE_Glyph *new_glyph = &tui->update[i];

        if (new_glyph->ch == old_glyph->ch && new_glyph->style.value == old_glyph->style.value &&
            new_glyph->style.value == tui->style.value) {
            // No change, the character is skipable.
            if (!skip.set) {
                skip.start_idx = i;
                skip.end_idx = i + 1;
                skip.set = true;
            } else {
                skip.end_idx++;
            }

            continue;
        }

        // Inject move sequences or reprinted characters.
        error = ryce_inject_sequence_internal(tui, x, y, &skip);
        if (error != RYCE_TUI_ERR_NONE) {
            return error;
        }

        // Inject the color code and style sequence if there is a change.
        error = ryce_write_style_internal(tui, &tui->update[i].style);
        if (error != RYCE_TUI_ERR_NONE) {
            return error;
        }

        // Check if we're next to the last written character.
        if (tui->write.length + 1 >= RYCE_WRITE_BUFFER_CAPACITY) {
            // Exceeded capacity for the write buffer.
            return RYCE_TUI_ERR_WRITE_BUFFER_OVERFLOW;
        }

        // Update buffer being written, propagate the change to the back buffer.
        tui->write.buffer[tui->write.length++] = tui->update[i].ch;
        tui->cache[i] = tui->update[i];
        tui->style = tui->update[i].style;
        tui->cursor.x = x;
        tui->cursor.y = y;
    }

    // Render the differences and move the cursor to the bottom of the pane.
    tui->cursor.x = tui->view.width;
    tui->cursor.y = tui->view.height;
    return ryce_print_write_buffer_internal(tui);
}

RYCE_PUBLIC RYCE_TuiError ryce_pane_set(RYCE_Pane *pane, uint32_t x, uint32_t y, const RYCE_Glyph *glyph) {
    if (x >= pane->view.width || y >= pane->view.height) {
        return RYCE_TUI_ERR_INVALID_COORDINATES;
    }

    // Convert from relative coordinates to absolute coordinates.
    size_t idx = ((pane->view.y + y) * pane->ctx->view.width) + (pane->view.x + x);
    if (idx >= RYCE_SCREEN_BUFFER_CAPACITY) {
        return RYCE_TUI_ERR_INVALID_COORDINATES;
    } else if (pane->ctx->render_mask[idx] != pane->id) {
        return RYCE_TUI_ERR_INVALID_PANE;
    }

    pane->ctx->update[idx] = *glyph;
    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC RYCE_TuiError ryce_pane_set_str(RYCE_Pane *pane, uint32_t x, uint32_t y, const RYCE_Style *style,
                                            RYCE_CHAR *ch) {
    if (x >= pane->view.width || y >= pane->view.height) {
        return RYCE_TUI_ERR_INVALID_COORDINATES;
    }

    // Ensure we don't exceed the pane's width.
    size_t len = RYCE_STRLEN(ch);
    len = len > pane->view.width - x ? pane->view.width - x : len;

    for (size_t i = 0; i < len; i++) {
        RYCE_Glyph glyph = {.ch = ch[i], .style = *style};
        if (ryce_pane_set(pane, x + i, y, &glyph) != RYCE_TUI_ERR_NONE) {
            return RYCE_TUI_ERR_INVALID_COORDINATES;
        }
    }

    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC_DECL RYCE_TuiError ryce_move_cursor(RYCE_TuiContext *tui, int64_t x, int64_t y) {
    if (x == tui->cursor.x && y == tui->cursor.y) {
        return RYCE_TUI_ERR_NONE;
    }

    ryce_softreset_controller_internal(tui);
    RYCE_TuiError err_code = ryce_write_move_internal(tui, x, y);
    if (err_code != RYCE_TUI_ERR_NONE) {
        return err_code;
    }

    tui->cursor.x = x;
    tui->cursor.y = y;
    return ryce_print_write_buffer_internal(tui);
}

RYCE_PUBLIC RYCE_TuiError ryce_clear_pane(RYCE_Pane *pane) {
    uint32_t x = 0; // X position in flattened buffer.
    uint32_t y = 0; // Y position in flattened buffer.

    for (size_t i = 0; i < RYCE_SCREEN_BUFFER_CAPACITY; i++) {
        if (pane->ctx->render_mask[i] != pane->id) {
            continue;
        }

        if (pane->ctx->update[i].ch == RYCE_EMPTY_CHAR &&
            pane->ctx->update[i].style.value == RYCE_DEFAULT_STYLE.value) {
            continue;
        }

        x = i % pane->ctx->view.width; // X position in flattened buffer.
        y = i / pane->ctx->view.width; // Y position in flattened buffer.
        if (x >= pane->view.x + pane->view.width || y >= pane->view.y + pane->view.height) {
            // Out of bounds, skip.
            i = ((y + 1) * pane->view.width) - 1;
            continue;
        }

        // Non-default character, reset to default.
        pane->ctx->update[i] = RYCE_DEFAULT_GLYPH;
    }

    return RYCE_TUI_ERR_NONE;
}

RYCE_PUBLIC RYCE_TuiError ryce_clear_screen(void) {
    RYCE_PRINTF(RYCE_CSI "2J" RYCE_CSI "0;0H");
    if (fflush(stdout) != 0) {
        return RYCE_TUI_ERR_STDOUT_FLUSH_FAILED;
    }

    return RYCE_TUI_ERR_NONE;
}

#endif // RYCE_TUI_IMPL
#endif // RYCE_TUI_H
