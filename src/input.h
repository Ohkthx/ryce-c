#if defined(RYCE_IMPL) && !defined(RYCE_INPUT_IMPL)
#define RYCE_INPUT_IMPL
#endif
#ifndef RYCE_INPUT_H
/*
    RyCE Input - A single-header, STB-styled input context.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_INPUT_IMPL
       #include "input.h"

    2) In as many other files as you need, just #include "input.h"
       WITHOUT defining RYCE_INPUT_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_INPUT_H

#include <pthread.h> // pthread_t, pthread_mutex_t, pthread_create, pthread_join
#include <signal.h>  // sig_atomic_t
#include <stdio.h>   // printf, fflush, sscanf
#include <stdlib.h>  // malloc, free, realloc
#include <termios.h> // termios, tcgetattr, tcsetattr, TCSAFLUSH
#include <unistd.h>  // read, STDIN_FILENO

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
    Mouse Modes:
    \x1b[?1000h - Basic Mouse Mode
    \x1b[?1002h - Button-event tracking
    \x1b[?1003h - Any-event tracking
    \x1b[?1006h - SGR Mouse Mode
*/

#define RYCE_MOUSE_MODE_BASIC "\x1b[?1000h"
#define RYCE_MOUSE_MODE_BUTTON "\x1b[?1002h"
#define RYCE_MOUSE_MODE_ALL "\x1b[?1003h"
#define RYCE_MOUSE_MODE_SGR "\x1b[?1006h"

#ifndef RYCE_MOUSE_MODE
#define RYCE_MOUSE_MODE RYCE_MOUSE_MODE_BASIC
#endif // RYCE_MOUSE_MODE

#ifndef RYCE_INPUT_INITIAL_EVENTS
#define RYCE_INPUT_INITIAL_EVENTS 16
#endif // RYCE_INPUT_INITIAL_EVENTS

// Error Codes.
typedef enum RYCE_InputError {
    RYCE_INPUT_ERR_NONE,                  ///< No error.
    RYCE_INPUT_CAUGHT_SIGINT,             ///< Caught SIGINT.
    RYCE_INPUT_TCSETATTR_FAILED,          ///< Failed to set terminal attributes.
    RYCE_INPUT_TCGETATTR_FAILED,          ///< Failed to get terminal attributes.
    RYCE_INPUT_ERR_ALLOCATE_CONTEXT,      ///< Failed context allocation.
    RYCE_INPUT_ERR_ALLOCATE_EVENT_BUFFER, ///< Failed event buffer allocation.
    RYCE_INPUT_ERR_ALLOCATE_EVENTS,       ///< Failed events allocation.
    RYCE_INPUT_ERR_THREAD_LISTEN,         ///< Failed to create input listening thread.
} RYCE_InputError;

typedef enum RYCE_InputEventType {
    RYCE_EVENT_KEY,  // Represents a keyboard event.
    RYCE_EVENT_MOUSE // Represents a mouse event.
} RYCE_InputEventType;

/*
    Public API Structs
*/

/**
 * @brief Structure representing an input event.
 */
typedef struct RYCE_InputEvent {
    RYCE_InputEventType type; //< The type of event.
    union {
        char key;
        struct {
            size_t button; //< The button pressed on the mouse.
            bool released; //< Whether the button was released.
            size_t x;      //< The x-coordinate of the mouse event.
            size_t y;      //< The y-coordinate of the mouse event.
        } mouse;
    } data;
} RYCE_InputEvent;

/**
 * @brief Stores a series of events and the total amount.
 */
typedef struct RYCE_InputEventBuffer {
    RYCE_InputEvent *events; //< Events contained within the buffer.
    size_t length;           //< Total amount of events.
    size_t max_length;       //< Maximum number of events the buffer can hold.
} RYCE_InputEventBuffer;

/**
 * @brief Structure representing a collection of input events.
 */
typedef struct RYCE_InputContext {
    struct termios initial_termios;   //< Initial terminal settings.
    pthread_mutex_t events_lock;      //< Mutex to protect access to the events array.
    pthread_t thread_id;              //< ID of the input listening thread.
    volatile sig_atomic_t *sigint;    //< Lock to signal when to stop the thread.
    RYCE_InputEventBuffer *ev_buffer; //< Buffer to store events.
} RYCE_InputContext;

/*
    Public API Functions.
*/

/**
 * @brief Disables raw mode for the terminal.
 *
 * @param initial_termios Initial terminal settings.
 */
RYCE_PUBLIC_DECL RYCE_InputError ryce_disable_raw_mode(struct termios *initial_termios);

/**
 * @brief Enables raw mode for the terminal.
 *
 * @param initial_termios Initial terminal settings.
 */
RYCE_PUBLIC_DECL RYCE_InputError ryce_enable_raw_mode(struct termios *initial_termios);

/**
 * @brief Initializes the RYCE_InputContext structure.
 *
 * @return Pointer to the initialized RYCE_InputContext structure.
 */
RYCE_PUBLIC_DECL RYCE_InputError ryce_init_input_ctx(volatile sig_atomic_t *sigint, RYCE_InputContext *ctx);

/**
 * @brief Frees the memory allocated for RYCE_InputContext.
 *
 * @param events Pointer to the RYCE_InputContext structure to be freed.
 */
RYCE_PUBLIC_DECL void ryce_input_free_ctx(RYCE_InputContext *ctx);

/**
 * @brief Starts a new thread to listen for input events. This collects events
 * in a separate thread.
 *
 * @param events Pointer to the RYCE_InputContext structure that will be used to
 * store new events.
 * @return 0 if the thread was created successfully, otherwise an error code.
 */
RYCE_PUBLIC_DECL RYCE_InputError ryce_input_listen(RYCE_InputContext *ctx);

/**
 * @brief Joins the input listening thread.
 *
 * @param thread_id The thread ID of the input listening thread.
 */
RYCE_PUBLIC_DECL RYCE_InputError ryce_input_join(RYCE_InputContext *ctx);

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_INPUT_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_INPUT_IMPL

RYCE_PRIVATE RYCE_InputError ryce_input_add_event_internal(RYCE_InputContext *ctx, RYCE_InputEvent event) {
    pthread_mutex_lock(&ctx->events_lock);

    if (ctx->ev_buffer->length >= ctx->ev_buffer->max_length) {
        RYCE_InputEventBuffer *buffer = ctx->ev_buffer;
        size_t new_max = buffer->max_length * 2;
        RYCE_InputEvent *new_events = (RYCE_InputEvent *)realloc(buffer->events, sizeof(RYCE_InputEvent) * new_max);
        if (new_events == nullptr) {
            pthread_mutex_unlock(&ctx->events_lock);
            return RYCE_INPUT_ERR_ALLOCATE_EVENTS;
        }

        ctx->ev_buffer->events = new_events;
        ctx->ev_buffer->max_length = new_max;
    }

    // Add the event.
    ctx->ev_buffer->events[ctx->ev_buffer->length++] = event;

    pthread_mutex_unlock(&ctx->events_lock);
    return RYCE_INPUT_ERR_NONE;
}

RYCE_PRIVATE RYCE_InputError ryce_input_parse_basic_mouse_internal(RYCE_InputContext *ctx) {
    const int ASCII_ENCODING_OFFSET = 32;
    unsigned char seq[] = {'\0', '\0', '\0'};
    size_t length = 0;

    // Read the three bytes for the basic mouse event.
    while (length < 3 && read(STDIN_FILENO, &seq[length], 1) == 1) {
        length++;
    }

    if (length == 3) {
        // Parse and add the basic mouse event.
        RYCE_InputEvent event = {
            .type = RYCE_EVENT_MOUSE,
            .data.mouse = {.button = seq[0] - ASCII_ENCODING_OFFSET,
                           .released = (bool)(seq[0] - ASCII_ENCODING_OFFSET == 3),
                           .x = seq[1] - ASCII_ENCODING_OFFSET,
                           .y = seq[2] - ASCII_ENCODING_OFFSET},
        };

        return ryce_input_add_event_internal(ctx, event);
    }

    // Collect the unknown sequence into events.
    for (size_t i = 0; i < length; i++) {
        RYCE_InputEvent event = (RYCE_InputEvent){.type = RYCE_EVENT_KEY, .data.key = (char)seq[i]};
        RYCE_InputError error = ryce_input_add_event_internal(ctx, event);
        if (error != RYCE_INPUT_ERR_NONE) {
            return error;
        }
    }

    return RYCE_INPUT_ERR_NONE;
}

RYCE_PRIVATE RYCE_InputError ryce_input_parse_sgr_mouse_internal(RYCE_InputContext *ctx) {
    const size_t MAX_SGR_SEQ = 32;
    char seq[MAX_SGR_SEQ];
    size_t length = 0;
    char button_state = 'M';

    // Read the three bytes for the basic mouse event.
    while (read(STDIN_FILENO, &seq[length], 1) == 1 && length < MAX_SGR_SEQ) {
        if (seq[length] == 'M' || seq[length] == 'm') {
            // Parse the SGR sequence: <Cb;Cx;CyM or <Cb;Cx;Cym
            button_state = seq[length];
            seq[length] = '\0';
            break;
        }

        length++;
    }

    // Ensure the sequence is null-terminated.
    length = length >= MAX_SGR_SEQ ? MAX_SGR_SEQ - 1 : length;
    seq[length] = '\0';

    RYCE_InputEvent event = {
        .type = RYCE_EVENT_MOUSE,
        .data.mouse = {.button = 0, .x = 0, .y = 0},
    };

    // Parse the SGR sequence: Cb;Cx;CyM or Cb;Cx;Cym.
    if (sscanf(seq, "%zu;%zu;%zu", &event.data.mouse.button, &event.data.mouse.x, &event.data.mouse.y) == 3) {
        event.data.mouse.released = (bool)(button_state == 'm');
        return ryce_input_add_event_internal(ctx, event);
    }

    // Collect the unknown sequence into events.
    for (size_t i = 0; i < length; i++) {
        RYCE_InputEvent ev = (RYCE_InputEvent){.type = RYCE_EVENT_KEY, .data.key = seq[i]};
        RYCE_InputError error = ryce_input_add_event_internal(ctx, ev);
        if (error != RYCE_INPUT_ERR_NONE) {
            return error;
        }
    }

    return RYCE_INPUT_ERR_NONE;
}

RYCE_PRIVATE RYCE_InputError ryce_input_parse_ansi_sequence_internal(RYCE_InputContext *ctx) {
    const size_t BRACKET_POS = 1;
    const size_t MODE_TYPE_POS = 2;
    unsigned char seq[] = {'\x1b', '\0', '\0'};
    size_t length = 1;

    // Peek 2 bytes to determine the sequence type.
    if (read(STDIN_FILENO, &seq[BRACKET_POS], 1) != 1 || read(STDIN_FILENO, &seq[MODE_TYPE_POS], 1) != 1) {
        if (seq[BRACKET_POS] != '\0') {
            // Only one byte was read.
            length++;
        }
    } else {
        // Two bytes were read.
        length += 2;
    }

    // Extract mouse mode and process the sequence.
    if (length == 3 && seq[BRACKET_POS] == '[') {
        if (seq[MODE_TYPE_POS] == '<') {
            return ryce_input_parse_sgr_mouse_internal(ctx);
        } else if (seq[MODE_TYPE_POS] == 'M' || seq[MODE_TYPE_POS] == 'm') {
            return ryce_input_parse_basic_mouse_internal(ctx);
        }
    }

    // Collect the unknown sequence into events.
    for (size_t i = 0; i < length; i++) {
        RYCE_InputEvent event = (RYCE_InputEvent){.type = RYCE_EVENT_KEY, .data.key = (char)seq[i]};
        RYCE_InputError error = ryce_input_add_event_internal(ctx, event);
        if (error != RYCE_INPUT_ERR_NONE) {
            return error;
        }
    }

    return RYCE_INPUT_ERR_NONE;
}

RYCE_PRIVATE void *ryce_input_read_internal(void *arg) {
    RYCE_InputContext *ctx = (RYCE_InputContext *)arg;
    RYCE_InputError error = RYCE_INPUT_ERR_NONE;
    unsigned char user_input = '\0';
    ssize_t bytes_read = 0;

    while (ctx->sigint == nullptr || *(ctx->sigint) != 1) {
        bytes_read = read(STDIN_FILENO, &user_input, 1);
        if (bytes_read == -1) {
            fprintf(stderr, "Failed to read input.");
            return nullptr;
        } else if (bytes_read == 0) {
            continue;
        }

        // Check if start of ANSI escape sequence.
        if (user_input == '\x1b') {
            error = ryce_input_parse_ansi_sequence_internal(ctx);
            if (error != RYCE_INPUT_ERR_NONE) {
                fprintf(stderr, "Failed to parse ANSI sequence: %d\n", error);
                return nullptr;
            }
        } else {
            // Process as a normal key event.
            RYCE_InputEvent event = {.type = RYCE_EVENT_KEY, .data.key = (char)user_input};
            error = ryce_input_add_event_internal(ctx, event);
            if (error != RYCE_INPUT_ERR_NONE) {
                fprintf(stderr, "Failed to add key event: %d\n", error);
                return nullptr;
            }
        }
    }

    return nullptr;
}

RYCE_PUBLIC RYCE_InputError ryce_disable_raw_mode(struct termios *initial_termios) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, initial_termios) != 0) {
        return RYCE_INPUT_TCSETATTR_FAILED;
    }

    return RYCE_INPUT_ERR_NONE;
}

RYCE_PUBLIC RYCE_InputError ryce_enable_raw_mode(struct termios *initial_termios) {
    if (tcgetattr(STDIN_FILENO, initial_termios) != 0) {
        return RYCE_INPUT_TCGETATTR_FAILED;
    }

    struct termios raw = *initial_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN); // Removed ISIG for Ctrl-C.
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return RYCE_INPUT_TCSETATTR_FAILED;
    }

    printf(RYCE_MOUSE_MODE);
    fflush(stdout);

    return RYCE_INPUT_ERR_NONE;
}

RYCE_PUBLIC RYCE_InputError ryce_init_input_ctx(volatile sig_atomic_t *sigint, RYCE_InputContext *ctx) {
    *ctx = (RYCE_InputContext){
        .sigint = sigint,
    };

    // Initialize the locks.
    pthread_mutex_init(&ctx->events_lock, nullptr);

    // Allocate memory for the entire event buffer.
    ctx->ev_buffer = (RYCE_InputEventBuffer *)malloc(sizeof(RYCE_InputEventBuffer));
    if (ctx->ev_buffer == nullptr) {
        return RYCE_INPUT_ERR_ALLOCATE_EVENT_BUFFER;
    }

    // Initialize the event buffer.
    ctx->ev_buffer->length = 0;
    ctx->ev_buffer->max_length = RYCE_INPUT_INITIAL_EVENTS;
    ctx->ev_buffer->events = (RYCE_InputEvent *)malloc(sizeof(RYCE_InputEvent) * RYCE_INPUT_INITIAL_EVENTS);
    if (ctx->ev_buffer->events == nullptr) {
        free(ctx->ev_buffer);
        return RYCE_INPUT_ERR_ALLOCATE_EVENTS;
    }

    return RYCE_INPUT_ERR_NONE;
}

RYCE_PUBLIC void ryce_input_free_ctx(RYCE_InputContext *ctx) {
    free(ctx->ev_buffer->events);
    free(ctx->ev_buffer);
    pthread_mutex_destroy(&ctx->events_lock);
}

RYCE_PUBLIC RYCE_InputEventBuffer ryce_input_get(RYCE_InputContext *ctx) {
    RYCE_InputEventBuffer buffer = {nullptr, 0, RYCE_INPUT_INITIAL_EVENTS};
    if (ctx == nullptr) {
        return buffer;
    }

    pthread_mutex_lock(&ctx->events_lock);
    if (ctx->ev_buffer->length == 0) {
        pthread_mutex_unlock(&ctx->events_lock);
        return buffer;
    }

    // Stores new events, will replace old events.
    RYCE_InputEvent *empty_events = (RYCE_InputEvent *)malloc(sizeof(RYCE_InputEvent) * ctx->ev_buffer->max_length);
    if (empty_events == nullptr) {
        pthread_mutex_unlock(&ctx->events_lock);
        return buffer;
    }

    // Swap the events from the ctx into the buffer.
    buffer.length = ctx->ev_buffer->length;
    buffer.max_length = ctx->ev_buffer->max_length;
    buffer.events = ctx->ev_buffer->events;

    // Reset the ctx's events to be empty.
    ctx->ev_buffer->events = empty_events;
    ctx->ev_buffer->length = 0;
    pthread_mutex_unlock(&ctx->events_lock);
    return buffer;
}

RYCE_PUBLIC RYCE_InputError ryce_input_listen(RYCE_InputContext *ctx) {
    RYCE_InputError error = ryce_enable_raw_mode(&ctx->initial_termios);
    if (error != RYCE_INPUT_ERR_NONE) {
        return error;
    }

    int err = pthread_create(&ctx->thread_id, nullptr, ryce_input_read_internal, ctx);
    if (err != 0) {
        return RYCE_INPUT_ERR_THREAD_LISTEN;
    }

    return RYCE_INPUT_ERR_NONE;
}

/**
 * @brief Joins the input listening thread.
 *
 * @param thread_id The thread ID of the input listening thread.
 */
RYCE_PUBLIC RYCE_InputError ryce_input_join(RYCE_InputContext *ctx) {
    pthread_join(ctx->thread_id, nullptr);

    RYCE_InputError error = ryce_disable_raw_mode(&ctx->initial_termios);
    if (error != RYCE_INPUT_ERR_NONE) {
        return error;
    }

    return RYCE_INPUT_ERR_NONE;
}

#endif // RYCE_INPUT_IMPL
#endif // RYCE_INPUT_H
