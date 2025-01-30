// NOLINTBEGIN
#define RYCE_LOOP_IMPL
#define RYCE_TUI_IMPL
#define RYCE_INPUT_IMPL
#define RYCE_MOUSE_MODE "\x1b[?1003h"
// #define RYCE_WIDE_CHAR_SUPPORT

#include "input.h"
#include "loop.h"
#include "tui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

const size_t ALPHABET_SIZE = 26;
const size_t SCREEN_WIDTH = 150; // 170
const size_t SCREEN_HEIGHT = 40; // 54
const double SCREEN_CHANGES = 0.01;
const int TICKS_PER_SECOND = 144;
const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;
bool draw = false;
bool randomness = false;

volatile sig_atomic_t lock = 0;
void handle_sigint(int sig) {
    (void)sig;
    lock = 1;
}

typedef struct AppState {
    RYCE_InputContext input;
    RYCE_TuiContext *tui;
    RYCE_LoopContext loop;
} AppState;

void tick_action(AppState *app) {
    if (randomness) {
        int changes = (int)app->tui->pane->length * SCREEN_CHANGES;
        for (int i = 0; i < changes; i++) {
            // Randomly change a character in the update buffer.
            wchar_t c = (rand() % 2) == 0 ? RYCE_LITERAL('A') : RYCE_LITERAL('a') + (rand() % ALPHABET_SIZE);
            app->tui->pane->update[rand() % app->tui->pane->length].ch = c;
        }
    }

    // Render changes.
    RYCE_TuiError err_code = ryce_render_tui(app->tui);
    if (err_code != RYCE_TUI_ERR_NONE) {
        ryce_clear_screen();
        fprintf(stderr, "Failed to render TUI: %d\n\r", err_code);
        lock = 1;
    }
}

void input_action(AppState *app, size_t iter) {
    if (iter % 5 != 0) {
        return;
    }

    RYCE_InputEventBuffer buffer = ryce_input_get(&app->input);
    for (size_t i = 0; i < buffer.length; ++i) {
        RYCE_InputEvent ev = buffer.events[i];
        if (ev.type == RYCE_EVENT_MOUSE) {
            // printf("Mouse event: %d %d %d\n\r", ev.data.mouse.button, ev.data.mouse.x, ev.data.mouse.y);
            size_t idx = ryce_tui_coords_to_idx(ev.data.mouse.x - 1, ev.data.mouse.y - 1, app->tui->pane->width);
            if (draw && app->tui->pane->width >= ev.data.mouse.x && app->tui->pane->height >= ev.data.mouse.y) {
                app->tui->pane->update[idx].ch = RYCE_LITERAL('#');
            }

            continue;
        }

        char key = ev.data.key;
        // printf("Key event: %c\n\r", key);
        if (key == 'q') {
            lock = 1;
            pthread_cancel(app->input.thread_id);
            break;
        } else if (key == 'a') {
            draw = !draw;
        } else if (key == 'c') {
            ryce_clear_pane(app->tui->pane);
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

    AppState app = {};

    // Initialize a TUI to demonstrate rendering.
    app.tui = ryce_init_tui_ctx(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!app.tui) {
        fprintf(stderr, "Failed to init TUI.\n");
        return EXIT_FAILURE;
    }

    // Initialize the loop context.
    if (ryce_init_loop_ctx(&lock, TICKS_PER_SECOND, &app.loop) != RYCE_LOOP_ERR_NONE) {
        fprintf(stderr, "Failed to init loop context.\n");
        return EXIT_FAILURE;
    }

    if (ryce_init_input_ctx(&lock, &app.input) != RYCE_INPUT_ERR_NONE) {
        fprintf(stderr, "Failed to init input context.\n");
        return EXIT_FAILURE;
    } else if (ryce_input_listen(&app.input) != RYCE_INPUT_ERR_NONE) {
        fprintf(stderr, "Failed to start input listening thread.\n");
        return EXIT_FAILURE;
    }

    ryce_clear_screen();
    size_t iter = 0;
    do {
        input_action(&app, iter);
        tick_action(&app);
    } while (ryce_loop_tick(&app.loop) == RYCE_LOOP_ERR_NONE);

    ryce_free_tui_ctx(app.tui);
    ryce_input_join(&app.input);
    ryce_input_free_ctx(&app.input);
    return 0;
}
// NOLINTEND
