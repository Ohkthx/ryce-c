// #define TUI_WIDE_CHAR_SUPPORT (1)
#define RYCE_TUI_IMPLEMENTATION (1)

#include "tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

const size_t ALPHABET_SIZE = 26;
const double SCREEN_CHANGES = 0.1;

/**
 * @brief Performs the actions for the tick.
 *
 * @param tui TUI to render.
 * @param changes Amount of changes to make per tick.
 */
void tick_action(RYCE_TuiController *tui, int changes) {
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
    const int TICKS_PER_SECOND = 64;
    const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;

    // Initialize a TUI to demonstrate rendering.
    RYCE_TuiController *tui = ryce_init_controller(120, 40); // 170, 54
    if (!tui) {
        fprintf(stderr, "Failed to init TUI.\n");
        return EXIT_FAILURE;
    }

    ryce_clear_screen();
    srand((unsigned)time(nullptr));

    // Timing variables.
    struct timeval start_time;
    struct timeval end_time;
    // int changes = (int)tui->max_length * 0.001;
    int changes = (int)tui->max_length * SCREEN_CHANGES;

    while (1) {
        // Get the start time.
        gettimeofday(&start_time, NULL);

        // Execute a TUI rendering iteration.
        tick_action(tui, changes);

        // Get elapsed time.
        gettimeofday(&end_time, NULL);
        long elapsed = ((end_time.tv_sec - start_time.tv_sec) * 1000000L) + (end_time.tv_usec - start_time.tv_usec);

        // If we finished our tick early, sleep for the difference.
        if (elapsed < TICK_INTERVAL) {
            usleep(TICK_INTERVAL - elapsed);
        } else {
            fprintf(stderr, "Tick took too long: %ld\n", elapsed);
            break;
        }
    }

    ryce_free_controller(tui);
    return 0;
}
