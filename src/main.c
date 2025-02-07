// NOLINTBEGIN
// IMPLEMENTATION DEFINITIONS
#define RYCE_IMPL

// OVERRIDES / ENABLED FEATURES
#define RYCE_MOUSE_MODE_ALL
#define RYCE_WIDE_CHAR_SUPPORT

#include "input.h"
#include "loop.h"
#include "map.h"
#include "simplex.h"
#include "tui.h"
#include "vec.h"
#include <float.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// --- Constants ----------------------------------------------------------
const size_t ALPHABET_SIZE = 26;
const size_t MAP_LENGTH = 100; // Map’s X dimension (will be bumped to odd)
const size_t MAP_WIDTH = 50;   // Map’s Y dimension (will be bumped to odd)
const size_t MAP_HEIGHT = 5;   // Single-layer (2D map in a 3D container)
const double SCREEN_CHANGES = 0.0005;
const int TICKS_PER_SECOND = 144;
const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;
const float64_t SCALE = 0.025;

// --- Global state for exiting etc. ------------------------------------- //
bool draw = false;
bool player_moved = true;
bool randomness = false;
volatile sig_atomic_t lock = 0;
void handle_sigint(int sig) {
    (void)sig;
    lock = 1;
}

// --- Entity ------------------------------------------------------------ //
enum EntityAttributes {
    ATTR_NONE = 0,
    ATTR_SOLID = 1 << 0,
    ATTR_WALKABLE = 1 << 1,
};

typedef struct Entity {
    RYCE_EntityID id;
    RYCE_Glyph *glyph;
    uint8_t attr;
} Entity;

// --- Application state ------------------------------------------------- //
typedef struct AppState {
    RYCE_InputContext input;
    RYCE_TuiContext *tui;
    RYCE_LoopContext loop;
    RYCE_3dTextMap map;
    Entity *entities;
    size_t entity_count;
    RYCE_Vec3 player; // Player’s current position on the map
} AppState;

// --- Glyphs ------------------------------------------------------------- //
RYCE_Glyph GLYPHS[] = {
    {.ch = RYCE_LITERAL(' '), .style = RYCE_DEFAULT_STYLE},
    {.ch = RYCE_LITERAL('~'),
     .style = {.part = {.fg_color = RYCE_STYLE_COLOR_BLUE, .bg_color = RYCE_STYLE_COLOR_DEFAULT}}},
    {.ch = RYCE_LITERAL('.'),
     .style = {.part = {.fg_color = RYCE_STYLE_COLOR_YELLOW, .bg_color = RYCE_STYLE_COLOR_DEFAULT}}},
    {.ch = RYCE_LITERAL(','),
     .style = {.part = {.fg_color = RYCE_STYLE_COLOR_GREEN, .bg_color = RYCE_STYLE_COLOR_DEFAULT}}},
    {.ch = RYCE_LITERAL('T'),
     .style = {.part = {.fg_color = RYCE_STYLE_COLOR_GREEN, .bg_color = RYCE_STYLE_COLOR_DEFAULT}}},
    {.ch = RYCE_LITERAL('▲'),
     .style = {.part = {.fg_color = RYCE_STYLE_COLOR_WHITE, .bg_color = RYCE_STYLE_COLOR_DEFAULT}}},
    {.ch = RYCE_LITERAL('@'),
     .style = {.part = {.fg_color = RYCE_STYLE_COLOR_RED, .bg_color = RYCE_STYLE_COLOR_DEFAULT}}},

};

// --- Obtain terminal size ---------------------------------------------- //
RYCE_Vec2 get_terminal_size(void) {
    struct winsize ws;
    RYCE_Vec2 size = {0, 0};

    // Get the terminal size.
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
        size.x = ws.ws_col;
        size.y = ws.ws_row;
    }

    return size;
}

// --- Helper: Returns a “chunk” from a noise value. --------------------- //
int get_chunk(float noise, int slices) {
    float normalized = (noise + 1.0f) / 2.0f;
    int bin = (int)(normalized * slices);
    if (bin >= slices) {
        bin = slices - 1;
    }

    return bin - (slices / 2);
}

// --- Build the map entities -------------------------------------------- //
void init_entities(AppState *app) {
    app->entity_count = 6;
    app->entities = (Entity *)malloc(app->entity_count * sizeof(Entity));
    app->entities[0] = (Entity){.id = 0, .glyph = &GLYPHS[0], .attr = ATTR_NONE};
    app->entities[1] = (Entity){.id = 1, .glyph = &GLYPHS[1], .attr = ATTR_NONE};
    app->entities[2] = (Entity){.id = 2, .glyph = &GLYPHS[2], .attr = ATTR_WALKABLE};
    app->entities[3] = (Entity){.id = 3, .glyph = &GLYPHS[3], .attr = ATTR_WALKABLE};
    app->entities[4] = (Entity){.id = 4, .glyph = &GLYPHS[4], .attr = ATTR_SOLID};
    app->entities[5] = (Entity){.id = 5, .glyph = &GLYPHS[5], .attr = ATTR_SOLID};
}

// --- Build the 3D map -------------------------------------------------- //
// Here we fill the map’s cells with “entity IDs” (1..5) based on a noise value.
void init_map(RYCE_3dTextMap *map) {
    const int64_t SEED = rand();

    // Iterate over the map’s X and Y dimensions.
    for (int y = map->y.min; y <= map->y.max; y++) {
        for (int x = map->x.min; x <= map->x.max; x++) {
            float64_t noise = ryce_simplex_noise2(SEED, (x - map->x.min) * SCALE, (y - map->y.min) * SCALE);

            RYCE_Vec3 vec = {
                .x = x,
                .y = y,
                .z = 0 // get_chunk(noise, map->height),
            };

            RYCE_EntityID entity = RYCE_ENTITY_NONE;
            if (noise <= -0.5) {
                entity = 1; // Water
            } else if (noise <= -0.3) {
                entity = 2; // Beach
            } else if (noise <= 0.0) {
                entity = 3; // Grass
            } else if (noise <= 0.5) {
                entity = 4; // Forest
            } else {
                entity = 5; // Mountain
            }

            ryce_map_add_entity(map, &vec, entity);
        }
    }
}

// --- Spawn player ------------------------------------------------------ //
RYCE_Vec3 spawn_player(AppState *app) {
    // Iterate from the origin (0,0,0) up to the top-right corner.
    for (int y = 0; y <= app->map.y.max; y++) {
        for (int x = 0; x <= app->map.x.max; x++) {
            RYCE_Vec3 vec = {.x = x, .y = y, .z = 0};
            RYCE_EntityID entity = ryce_map_get_entity(&app->map, &vec);
            // If this cell is not solid, use it for the player spawn.
            if (!(app->entities[entity].attr & ATTR_SOLID)) {
                ryce_map_add_entity(&app->map, &vec, 0);
                return vec;
            }
        }
    }
    // Fallback: if no valid location was found, return the origin.
    return (RYCE_Vec3){0, 0, 0};
}

// --- Check movement ---------------------------------------------------- //
void move_player(AppState *app, RYCE_Vec3 *vec) {
    RYCE_EntityID entity_id = ryce_map_get_entity(&app->map, vec);
    if (entity_id == RYCE_ENTITY_NONE) {
        return;
    }

    Entity entity = app->entities[entity_id];
    if (entity.attr & ATTR_WALKABLE) {
        app->player = *vec;
        player_moved = true;
    }
}

// --- Input processing -------------------------------------------------- //
void input_action(AppState *app, size_t iter) {
    if (iter % 5 != 0)
        return;

    RYCE_InputEventBuffer buffer = ryce_input_get(&app->input);
    for (size_t i = 0; i < buffer.length; ++i) {
        RYCE_InputEvent ev = buffer.events[i];
        if (ev.type == RYCE_EVENT_MOUSE) {
            if (draw) {
                size_t idx =
                    ryce_vec2_to_idx(&(RYCE_Vec2){.x = ev.data.mouse.x, .y = ev.data.mouse.y}, app->tui->pane->width);
                app->tui->pane->update[idx].ch = RYCE_LITERAL('#');
                app->tui->pane->update[idx].style.part.fg_color = RYCE_STYLE_COLOR_WHITE;
            }
            continue;
        }

        RYCE_Vec3 next_pos = app->player;
        bool moved = false;

        switch (ev.data.key) {
        case 'x':
            lock = 1;
            pthread_cancel(app->input.thread_id);
            return;
        case 'w': // Move up (decrease y)
            next_pos.y = ryce_math_clamp(app->player.y - 1, app->map.y.min, app->map.y.max);
            moved = true;
            break;
        case 's': // Move down (increase y)
            next_pos.y = ryce_math_clamp(app->player.y + 1, app->map.y.min, app->map.y.max);
            moved = true;
            break;
        case 'a': // Move left (decrease x)
            next_pos.x = ryce_math_clamp(app->player.x - 1, app->map.x.min, app->map.x.max);
            moved = true;
            break;
        case 'd': // Move right (increase x)
            next_pos.x = ryce_math_clamp(app->player.x + 1, app->map.x.min, app->map.x.max);
            moved = true;
            break;
        case 'e': // Move up (increase z)
            next_pos.z = ryce_math_clamp(app->player.z + 1, app->map.z.min, app->map.z.max);
            moved = true;
            break;
        case 'q': // Move down (decrease z)
            next_pos.z = ryce_math_clamp(app->player.z - 1, app->map.z.min, app->map.z.max);
            moved = true;
            break;
        case 'v':
            draw = !draw;
            break;
        case 'c':
            ryce_clear_pane(app->tui->pane);
            break;
        case 't':
            randomness = !randomness;
            break;
        default:
            break;
        }

        if (moved) {
            move_player(app, &next_pos);
        }
    }

    free(buffer.events);
}

// --- Tick processing --------------------------------------------------- //
// (For example, applying some random changes to the TUI update buffer.)
void tick_action(AppState *app) {
    if (randomness) {
        int changes = (int)app->tui->pane->capacity * SCREEN_CHANGES;
        for (int i = 0; i < changes; i++) {
            wchar_t c = ((rand() % 2) == 0 ? RYCE_LITERAL('A') : RYCE_LITERAL('a')) + (rand() % ALPHABET_SIZE);
            app->tui->pane->update[rand() % app->tui->pane->capacity].ch = c;
        }
    }
}

// --- Render action ----------------------------------------------------- //
// Draws the map onto the TUI pane.
void render_map(AppState *app) {
    int pane_width = app->tui->pane->width;
    int pane_height = app->tui->pane->height;

    for (int ty = 0; ty < pane_height; ty++) {
        for (int tx = 0; tx < pane_width; tx++) {
            // Calculate the map coordinate, the offset from the TUI’s center is added
            // to the player’s map position.
            int map_x = app->player.x + (tx - pane_width / 2);
            int map_y = app->player.y + (ty - pane_height / 2);
            size_t tui_idx = ryce_vec2_to_idx(&(RYCE_Vec2){.x = tx, .y = ty}, pane_width);

            // Draw the default empty glpyh if outside the map bounds.
            if (map_x < app->map.x.min || map_x > app->map.x.max || map_y < app->map.y.min || map_y > app->map.y.max) {
                app->tui->pane->update[tui_idx] = RYCE_DEFAULT_GLYPH;
                continue;
            }

            RYCE_Glyph glyph = RYCE_DEFAULT_GLYPH;
            glyph.style.part.style_flags = RYCE_STYLE_MODIFIER_BOLD;
            RYCE_Vec3 position = {.x = map_x, .y = map_y, .z = app->player.z};
            RYCE_EntityID entity = RYCE_ENTITY_NONE;

            // Search elevations below player's Z for map entities.
            for (; position.z >= app->map.z.min; position.z--) {
                entity = ryce_map_get_entity(&app->map, &position);
                if (entity != RYCE_ENTITY_NONE) {
                    if (position.z < app->player.z) {
                        // Add styling to the lower elevations.
                        glyph.style.part.style_flags = RYCE_STYLE_MODIFIER_DIM | RYCE_STYLE_MODIFIER_ITALIC;
                    }
                    break;
                }
            }

            app->tui->pane->update[tui_idx] = *app->entities[entity].glyph;
        }
    }
}

void render_action(AppState *app) {
    int pane_width = app->tui->pane->width;
    int pane_height = app->tui->pane->height;

    // For each cell in the TUI, determine which map coordinate to show.
    if (player_moved) {
        render_map(app);

        // Draw the player at the center of the TUI.
        int center_x = pane_width / 2;
        int center_y = pane_height / 2;
        size_t player_idx = ryce_vec2_to_idx(&(RYCE_Vec2){.x = center_x, .y = center_y}, pane_width);
        app->tui->pane->update[player_idx].ch = '@';
        app->tui->pane->update[player_idx].style.part.fg_color = RYCE_STYLE_COLOR_RED;
        app->tui->pane->update[player_idx].style.part.style_flags = RYCE_STYLE_MODIFIER_BOLD;
        player_moved = false;
    }

    // Render the TUI.
    RYCE_TuiError err_code = ryce_render_tui(app->tui);
    if (err_code != RYCE_TUI_ERR_NONE) {
        ryce_clear_screen();
        fprintf(stderr, "Failed to render TUI: %d\n\r", err_code);
        lock = 1;
    }
}

int main(void) {
    // Register SIGINT handler.
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    srand((unsigned)time(NULL));

    AppState app = {0};

    // Initialize the TUI.
    RYCE_Vec2 term_size = get_terminal_size();
    app.tui = ryce_init_tui_ctx(term_size.x, term_size.y);
    if (!app.tui) {
        fprintf(stderr, "Failed to init TUI.\n");
        return EXIT_FAILURE;
    }

    // Initialize the loop context.
    if (ryce_init_loop_ctx(&lock, TICKS_PER_SECOND, &app.loop) != RYCE_LOOP_ERR_NONE) {
        fprintf(stderr, "Failed to init loop context.\n");
        return EXIT_FAILURE;
    }

    // Initialize input and start the input thread.
    if (ryce_init_input_ctx(&lock, &app.input) != RYCE_INPUT_ERR_NONE) {
        fprintf(stderr, "Failed to init input context.\n");
        return EXIT_FAILURE;
    } else if (ryce_input_listen(&app.input) != RYCE_INPUT_ERR_NONE) {
        fprintf(stderr, "Failed to start input listening thread.\n");
        return EXIT_FAILURE;
    }

    // Initialize the 3D map.
    if (ryce_init_3d_map(&app.map, MAP_LENGTH, MAP_WIDTH, MAP_HEIGHT) != RYCE_MAP_ERR_NONE) {
        fprintf(stderr, "Failed to init 3D map.\n");
        return EXIT_FAILURE;
    }

    init_entities(&app);
    init_map(&app.map);
    app.player = spawn_player(&app);

    ryce_clear_screen();
    size_t iter = 0;
    do {
        input_action(&app, iter);
        tick_action(&app);
        render_action(&app);
        iter++;
    } while (ryce_loop_tick(&app.loop) == RYCE_LOOP_ERR_NONE);

    ryce_free_tui_ctx(app.tui);
    ryce_input_join(&app.input);
    ryce_input_free_ctx(&app.input);
    return 0;
}
// NOLINTEND
