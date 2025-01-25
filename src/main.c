// #define TUI_WIDE_CHAR_SUPPORT (1)
#define RYCE_LOOP_IMPL
#define RYCE_TUI_IMPL

#include "loop.h"
#include "tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

const size_t ALPHABET_SIZE = 26;
const size_t SCREEN_WIDTH = 170; // 170
const size_t SCREEN_HEIGHT = 20; // 54
const double SCREEN_CHANGES = 0.0001;
const int TICKS_PER_SECOND = 1;
const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;

volatile sig_atomic_t lock = 1;
void handle_sigint(int sig) {
    (void)sig;
    lock = 0;
    fflush(stdout);
}

/**
 * @brief Performs the actions for the tick.
 *
 * @param tui TUI to render.
 * @param changes Amount of changes to make per tick.
 */
void tick_action(RYCE_TuiContext *tui) {
    int changes = (int)tui->max_length * SCREEN_CHANGES;
    for (int i = 0; i < changes; i++) {
        // Randomly change a character in the update buffer.
        wchar_t c = (rand() % 2 == 0 ? L'A' : L'a') + (rand() % ALPHABET_SIZE);
        tui->pane->update[rand() % tui->pane->length] = c;
    }

    // Render changes.
    RYCE_TuiError err_code = ryce_render_tui(tui);
    if (err_code != RYCE_TUI_ERR_NONE) {
        ryce_clear_screen();
        fprintf(stderr, "Failed to render TUI: %d\n", err_code);
    }
}

int main(void) {
    srand((unsigned)time(nullptr));

    // Initialize a TUI to demonstrate rendering.
    RYCE_TuiContext *tui = ryce_init_tui_ctx(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!tui) {
        fprintf(stderr, "Failed to init TUI.\n");
        return EXIT_FAILURE;
    }

    // Initialize the loop context.
    RYCE_LoopContext loop = {};
    if (ryce_init_loop_ctx(&lock, TICKS_PER_SECOND, &loop) != RYCE_LOOP_ERR_NONE) {
        fprintf(stderr, "Failed to init loop context.\n");
        return EXIT_FAILURE;
    }

    ryce_clear_screen();
    while (ryce_loop_tick(&loop) == RYCE_LOOP_ERR_NONE) {
        tick_action(tui);
    }

    ryce_free_ctx(tui);
    return 0;
}
