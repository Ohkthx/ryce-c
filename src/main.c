// NOLINTBEGIN
// #define TUI_WIDE_CHAR_SUPPORT (1)
#define RYCE_LOOP_IMPL
#define RYCE_TUI_IMPL
#define RYCE_INPUT_IMPL
#define RYCE_MOUSE_MODE "\x1b[?1003h"

#include "input.h"
#include "loop.h"
#include "tui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

const size_t ALPHABET_SIZE = 26;
const size_t SCREEN_WIDTH = 170; // 170
const size_t SCREEN_HEIGHT = 20; // 54
const double SCREEN_CHANGES = 0.0001;
const int TICKS_PER_SECOND = 60;
const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;

volatile sig_atomic_t lock = 0;
void handle_sigint(int sig) {
    (void)sig;
    lock = 1;
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

void input_action(RYCE_InputContext *input, size_t iter) {
    if (iter % 5 != 0) {
        return;
    }

    RYCE_InputEventBuffer buffer = ryce_input_get(input);
    for (size_t i = 0; i < buffer.length; ++i) {
        RYCE_InputEvent ev = buffer.events[i];
        if (ev.type == RYCE_EVENT_MOUSE) {
            printf("Mouse event: %d %d %d\n\r", ev.data.mouse.button, ev.data.mouse.x, ev.data.mouse.y);
            continue;
        }

        char key = ev.data.key;
        printf("Key event: %c\n\r", key);
        if (key == 'q') {
            lock = 1;
            pthread_cancel(input->thread_id);
            break;
        }
    }

    free(buffer.events);
}

int main(void) {
    // Register the SIGINT handler.
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

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

    RYCE_InputContext input = {};
    if (ryce_init_input_ctx(&lock, &input) != RYCE_INPUT_ERR_NONE) {
        fprintf(stderr, "Failed to init input context.\n");
        return EXIT_FAILURE;
    } else if (ryce_input_listen(&input) != RYCE_INPUT_ERR_NONE) {
        fprintf(stderr, "Failed to start input listening thread.\n");
        return EXIT_FAILURE;
    }

    ryce_clear_screen();
    size_t iter = 0;
    do {
        input_action(&input, iter);
        /* tick_action(tui); */
    } while (ryce_loop_tick(&loop) == RYCE_LOOP_ERR_NONE);

    ryce_tui_free_ctx(tui);
    ryce_input_join(&input);
    ryce_input_free_ctx(&input);
    return 0;
}
// NOLINTEND
