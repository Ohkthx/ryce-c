// #define TUI_WIDE_CHAR_SUPPORT 1

#include "tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * @brief Clears the terminal screen.
 */
void clear_screen() {
    printf("\033[2J");
    printf("\033[0;0H");
    fflush(stdout);
}

/**
 * @brief Performs the actions for the tick.
 *
 * @param tui TUI to render.
 * @param changes Amount of changes to make per tick.
 */
void tick_action(Tui *tui, int changes) {
    for (int i = 0; i < changes; i++) {
        // Randomly change a character in the update buffer.
        wchar_t c = (rand() % 2 == 0 ? L'A' : L'a') + (rand() % 26);
        tui->pane->update[rand() % tui->pane->length] = c;
    }

    // Render changes.
    int err_code = TUI_render(tui);
    if (err_code != 0) {
        clear_screen();
        fprintf(stderr, "Failed to render TUI: %d\n", err_code);
    }
}

int main(void) {
    const int TICKS_PER_SECOND = 64;
    const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;

    // Initialize a TUI to demonstrate rendering.
    Tui *tui = TUI_init(120, 40); // 170, 54
    if (!tui) {
        fprintf(stderr, "Failed to init TUI.\n");
        return EXIT_FAILURE;
    }

    clear_screen();
    srand((unsigned)time(NULL));

    // Timing variables.
    struct timeval start_time, end_time;
    // int changes = (int)tui->max_length * 0.001;
    int changes = (int)tui->max_length * 0.1;

    while (1) {
        // Get the start time.
        gettimeofday(&start_time, NULL);

        // Execute a TUI rendering iteration.
        tick_action(tui, changes);

        // Get elapsed time.
        gettimeofday(&end_time, NULL);
        long elapsed = (end_time.tv_sec - start_time.tv_sec) * 1000000L +
                       (end_time.tv_usec - start_time.tv_usec);

        // If we finished our tick early, sleep for the difference.
        if (elapsed < TICK_INTERVAL) {
            usleep(TICK_INTERVAL - elapsed);
        } else {
            fprintf(stderr, "Tick took too long: %ld\n", elapsed);
            break;
        }
    }

    TUI_free(tui);
    return 0;
}
