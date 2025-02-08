#if defined(RYCE_IMPL) && !defined(RYCE_LOOP_IMPL)
#define RYCE_LOOP_IMPL
#endif
#ifndef RYCE_LOOP_H
/*
    RyCE Loop - A single-header, STB-styled core loop context.

    USAGE:

    1) In exactly ONE of your .c or .cpp files, do:

       #define RYCE_LOOP_IMPL
       #include "loop.h"

    2) In as many other files as you need, just #include "loop.h"
       WITHOUT defining RYCE_LOOP_IMPL.

    3) Compile and link all files together.
*/
#define RYCE_LOOP_H

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

#define RYCE_TIME_NSEC 1'000'000'000L

// Error Codes.
typedef enum RYCE_LoopError {
    RYCE_LOOP_ERR_NONE,                ///< No error.
    RYCE_LOOP_CAUGHT_SIGINT,           ///< Caught SIGINT.
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
    size_t target_tps;             //< Ticks per second.
    size_t tick;                   //< Current tick count.

    // Fields for measuring actual TPS.
    double tps;               //< Last computed ticks per second.
    struct timespec last_tps; //< Time at which the current measurement window started.
    size_t tick_count;        //< Ticks counted in the current measurement window.
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

/*===========================================================================
   ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▖   ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
     █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌   ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌  █  ▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
     █  ▐▌  ▐▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌  █  ▐▛▀▜▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
   ▗▄█▄▖▐▌  ▐▌▐▌   ▐▙▄▄▖▐▙▄▄▖▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌  █  ▐▌ ▐▌  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌
   IMPLEMENTATION
   Provide function definitions only if RYCE_LOOP_IMPL is defined.
  ===========================================================================*/
#ifdef RYCE_LOOP_IMPL

RYCE_PUBLIC RYCE_LoopError ryce_init_loop_ctx(volatile sig_atomic_t *sigint, size_t tps, RYCE_LoopContext *ctx) {
    tps = tps > 0 ? tps : 1; // Prevent DIV by 0.
    *ctx = (RYCE_LoopContext){
        .sigint = sigint,
        .interval = {.tv_sec = 0, .tv_nsec = RYCE_TIME_NSEC / tps},
        .target_tps = tps,
        .tick = 0,
        .tps = 0.0,
        .tick_count = 0,
    };

    // Get the current time.
    if (clock_gettime(CLOCK_MONOTONIC, &ctx->last) != 0) {
        return RYCE_LOOP_INVALID_TIMING;
    }

    ctx->last_tps = ctx->last;

    return RYCE_LOOP_ERR_NONE;
}

RYCE_PUBLIC RYCE_LoopError ryce_loop_tick(RYCE_LoopContext *ctx) {
    if (ctx->sigint != nullptr && *(ctx->sigint) == 1) {
        return RYCE_LOOP_CAUGHT_SIGINT;
    }

    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return RYCE_LOOP_INVALID_TIMING;
    }

    // Calculate next tick time: last + interval.
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
        // TODO: handle running behind schedule.
    }

    // Get the actual time after sleeping.
    struct timespec actual;
    if (clock_gettime(CLOCK_MONOTONIC, &actual) != 0) {
        return RYCE_LOOP_INVALID_TIMING;
    }

    // Increment overall tick count.
    ctx->tick++;
    ctx->tick_count++;

    // Compute the elapsed time (in seconds) since the last TPS measurement window.
    {
        double elapsed = (actual.tv_sec - ctx->last_tps.tv_sec) + (actual.tv_nsec - ctx->last_tps.tv_nsec) / 1e9;
        if (elapsed >= 1.0) {
            // Calculate measured TPS as ticks divided by the actual elapsed time.
            ctx->tps = (double)ctx->tick_count / elapsed;
            ctx->tick_count = 0;
            ctx->last_tps = actual;
        }
    }

    // Update the last tick time.
    ctx->last = actual;

    return RYCE_LOOP_ERR_NONE;
}

#endif // RYCE_LOOP_IMPL
#endif // RYCE_LOOP_H
