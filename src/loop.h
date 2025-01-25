#ifndef RYCE_LOOP_H
#define RYCE_LOOP_IMPL

/*
    RyCE Loop - A single-header, STB-styled core loop ctx.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_LOOP_IMPL
       #include "loop.h"

    2) In as many other files as you need, just #include "loop.h"
       WITHOUT defining RYCE_LOOP_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_LOOP_H (1)
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Visibility macros.
#ifndef RYCE_PUBLIC_DECL
#define RYCE_PUBLIC_DECL extern
#endif
#ifndef RYCE_PRIVATE_DECL
#define RYCE_PRIVATE_DECL static
#endif

#define RYCE_TIME_NSEC 1'000'000'000L

// Error Codes.
typedef enum RYCE_LoopError {
    RYCE_LOOP_ERR_NONE,                ///< No error.
    RYCE_LOOP_INVALID_SIGINT_POINTER,  ///< Invalid sigint pointer.
    RYCE_LOOP_INVALID_TIMING,          ///< Invalid timing values.
    RYCE_LOOP_ERR_STDOUT_FLUSH_FAILED, ///< Failed to flush stdout.
} RYCE_LoopError;

/*
    Public API Structs
*/

/**
 * @brief Controls the loop rate of a program.
 */
typedef struct RYCE_LoopContext {
    volatile sig_atomic_t *sigint; //< Pointer to a flag to signal when to stop.
    struct timespec last;          //< Last recorded tick time.
    struct timespec interval;      //< Desired interval between ticks.
    size_t tps;                    //< Ticks per second.
    size_t tick;                   //< Current tick count.
} RYCE_LoopContext;

/*
    Public API Functions.
*/

/**
 * @brief Initializes the core loop context.
 *
 * @param sigint Pointer to a sig_atomic_t flag to signal stopping.
 * @param tps Ticks per second. Must be greater than 0.
 * @return RYCE_LoopContext Initialized context for tracking the core loop.
 */
RYCE_PUBLIC_DECL RYCE_LoopError ryce_init_loop_ctx(volatile sig_atomic_t *sigint, size_t tps, RYCE_LoopContext *ctx);

/**
 * @brief Pauses the thread to maintain the loop tick rate.
 *
 * @param ctx Pointer to the RYCE_LoopContext.
 */
RYCE_PUBLIC_DECL RYCE_LoopError ryce_loop_tick(RYCE_LoopContext *ctx);

#endif // RYCE_LOOP_H

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_LOOP_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_LOOP_IMPL

RYCE_PUBLIC_DECL RYCE_LoopError ryce_init_loop_ctx(volatile sig_atomic_t *sigint, size_t tps, RYCE_LoopContext *ctx) {
    if (!sigint) {
        return RYCE_LOOP_INVALID_SIGINT_POINTER;
    }

    tps = (tps > 0) ? tps : 1; // Prevent DIV by 0.
    *ctx = (RYCE_LoopContext){
        .sigint = sigint,
        .interval = {.tv_sec = 0, .tv_nsec = RYCE_TIME_NSEC / tps},
        .tps = tps,
        .tick = 0,
    };

    // Get the current time.
    if (clock_gettime(CLOCK_MONOTONIC, &ctx->last) != 0) {
        return RYCE_LOOP_INVALID_TIMING;
    }

    return RYCE_LOOP_ERR_NONE;
}

RYCE_PUBLIC_DECL RYCE_LoopError ryce_loop_tick(RYCE_LoopContext *ctx) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return RYCE_LOOP_INVALID_TIMING;
    }

    // Calculate next tick time: last + interval
    struct timespec next_tick = ctx->last;
    next_tick.tv_sec += ctx->interval.tv_sec;
    next_tick.tv_nsec += ctx->interval.tv_nsec;
    if (next_tick.tv_nsec >= RYCE_TIME_NSEC) {
        next_tick.tv_sec += 1;
        next_tick.tv_nsec -= RYCE_TIME_NSEC;
    }

    // Calculate time to sleep until next_tick.
    long sleep_sec = next_tick.tv_sec - now.tv_sec;
    long sleep_nsec = next_tick.tv_nsec - now.tv_nsec;
    if (sleep_nsec < 0) {
        sleep_sec -= 1;
        sleep_nsec += RYCE_TIME_NSEC;
    }

    // If we are ahead of schedule, sleep until next_tick.
    if (sleep_sec > 0 || (sleep_sec == 0 && sleep_nsec > 0)) {
        struct timespec req = {.tv_sec = sleep_sec, .tv_nsec = sleep_nsec};
        struct timespec rem;
        while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
            req = rem; // Continue sleeping for the remaining time.
        }
    } else {
        /// TODO: Handle being behind schedule.
    }

    // Increment tick count and update last tick time.
    ctx->tick++;
    ctx->last = next_tick;

    return RYCE_LOOP_ERR_NONE;
}

#endif // RYCE_LOOP_IMPL
