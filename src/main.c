// NOLINTBEGIN
// IMPLEMENTATION DEFINITIONS
#define RYCE_IMPL

// OVERRIDES / ENABLED FEATURES
#define RYCE_MOUSE_MODE_ALL
#define RYCE_WIDE_CHAR_SUPPORT
#define RYCE_HIDE_CURSOR

#include "bla.h"
#include "camera.h"
#include "input.h"
#include "loop.h"
#include "map.h"
#include "simplex.h"
#include "tui.h"
#include "vec.h"
#include <float.h>
#include <inttypes.h> // NOTE: For PRId64
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// --- Constants --------------------------------------------------------- //
const size_t ALPHABET_SIZE = 26;
const size_t MAP_LENGTH = 500; // Map’s X dimension (will be bumped to odd)
const size_t MAP_WIDTH = 500;  // Map’s Y dimension (will be bumped to odd)
const size_t MAP_HEIGHT = 5;   // Single-layer (2D map in a 3D container)
const double SCREEN_CHANGES = 0.0005;
const int TICKS_PER_SECOND = 512;
const int DIST_PER_SECOND = 20; // Amount of blocks that can be traveled per second.
const int TICK_INTERVAL = 1000000 / TICKS_PER_SECOND;
const float64_t SCALE = 0.025;

// --- Interrupt Handler ------------------------------------------------- //
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
    RYCE_CameraContext camera;
    RYCE_InputContext input;
    RYCE_TuiContext tui;
    struct {
        RYCE_Pane map;
        RYCE_Pane debug;
    } panes;
    RYCE_LoopContext loop;
    RYCE_3dTextMap map;
    Entity *entities;
    size_t entity_count;
    RYCE_Vec3 player;      // Player’s current position on the map
    RYCE_Vec2 player_dest; // Player’s destination (for movement)
    RYCE_BLA_Error move_error;
} AppState;

// --- Glyphs ------------------------------------------------------------ //
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
    {
        .ch = RYCE_LITERAL('@'), // Player character.
        .style =
            {
                .part =
                    {
                        .fg_color = RYCE_STYLE_COLOR_RED,
                        .bg_color = RYCE_STYLE_COLOR_DEFAULT,
                        .style_flags = RYCE_STYLE_MODIFIER_BOLD,

                    },
            },
    },
    {
        .ch = RYCE_LITERAL(' '), // Player character.
        .style =
            {
                .part =
                    {
                        .fg_color = RYCE_STYLE_COLOR_DEFAULT,
                        .bg_color = RYCE_STYLE_COLOR_RED,
                        .style_flags = RYCE_STYLE_MODIFIER_BOLD,

                    },
            },
    },

};

// --- Initializers ------------------------------------------------------ //
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

void init_map(RYCE_3dTextMap *map) {
    const int64_t SEED = rand();

    // Iterate over the map’s X and Y dimensions.
    for (int y = map->y.min; y <= map->y.max; y++) {
        for (int x = map->x.min; x <= map->x.max; x++) {
            float64_t noise = ryce_simplex_noise2(SEED, (x - map->x.min) * SCALE, (y - map->y.min) * SCALE);

            RYCE_Vec3 vec = {
                .x = x,
                .y = y,
                .z = 0 // get_elevation(noise, map->height),
            };

            // Label the entity.
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
RYCE_Vec3 init_player(AppState *app) {
    // Iterate from the origin (0,0,0) up to the top-right corner.
    for (int y = 0; y <= app->map.y.max; y++) {
        for (int x = 0; x <= app->map.x.max; x++) {
            RYCE_Vec3 vec = {.x = x, .y = y, .z = 0};
            RYCE_EntityID entity = ryce_map_get_entity(&app->map, &vec);
            if (app->entities[entity].attr & ATTR_WALKABLE) {
                return vec;
            }
        }
    }

    // Fallback: if no valid location was found, return the origin.
    return (RYCE_Vec3){0, 0, 0};
}

// --- Miscellaneous ----------------------------------------------------- //
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

// Identifies the elevation based on the noise value.
int get_elevation(float noise, int slices) {
    float normalized = (noise + 1.0f) / 2.0f;
    int bin = (int)(normalized * slices);
    if (bin >= slices) {
        bin = slices - 1;
    }

    return bin - (slices / 2);
}

// --- Movement & Camera ------------------------------------------------- //
void reset_movement(AppState *app) {
    app->player_dest = (RYCE_Vec2){.x = app->player.x, .y = app->player.y};
    app->move_error.initialized = 0;
}

void move_player(AppState *app) {
    static double move_accumulator = 0.0;
    move_accumulator += (double)DIST_PER_SECOND / TICKS_PER_SECOND;

    if (app->player.x == app->player_dest.x && app->player.y == app->player_dest.y) {
        // No movement required.
        move_accumulator = 0.0; // Reset the accumulator.
        reset_movement(app);
        return;
    }

    // Prevent out-of-bounds movement.
    app->player_dest.x = ryce_math_clamp(app->player_dest.x, app->map.x.min, app->map.x.max);
    app->player_dest.y = ryce_math_clamp(app->player_dest.y, app->map.y.min, app->map.y.max);

    if (move_accumulator < 1.0) {
        // Accumulator not large enough for a full step.
        return;
    }

    // Compute the next step toward the destination.
    RYCE_Vec2 player = {app->player.x, app->player.y};
    RYCE_Vec2 next = ryce_bla_2dline(&player, &app->player_dest, &app->move_error);

    // If the computed step is the same as the current position, the path is blocked.
    if (app->player.x == next.x && app->player.y == next.y) {
        move_accumulator = 0.0; // Reset the accumulator.
        reset_movement(app);
        return;
    }

    // Check if the next cell is walkable.
    RYCE_Vec3 dest = {next.x, next.y, app->player.z};
    RYCE_EntityID entity_id = ryce_map_get_entity(&app->map, &dest);
    if (entity_id != RYCE_ENTITY_NONE && (app->entities[entity_id].attr & ATTR_WALKABLE)) {
        app->player = dest;
        move_accumulator -= 1.0;
    } else {
        // If blocked, cancel movement.
        move_accumulator = 0.0; // Reset the accumulator.
        reset_movement(app);
    }
}

// --- Input Actions ----------------------------------------------------- //
// Map input events to player controls.
void input_action(AppState *app) {
    if (app->loop.tick % 5 != 0)
        return;

    RYCE_InputEventBuffer buffer = ryce_input_get(&app->input);
    for (size_t i = 0; i < buffer.length; ++i) {
        RYCE_InputEvent ev = buffer.events[i];

        if (ev.type == RYCE_EVENT_MOUSE && ev.data.mouse.button == 3) {
            RYCE_Vec2 mouse_pos = {.x = ev.data.mouse.x - 1, .y = ev.data.mouse.y - 1};
            RYCE_Vec2 map_pos = ryce_from_terminal(&app->camera, &mouse_pos);
            app->player_dest = (RYCE_Vec2){.x = map_pos.x, .y = map_pos.y};
        } else {
            switch (ev.data.key) {
            case 'x':
                lock = 1;
                pthread_cancel(app->input.thread_id);
                return;
            case 'w': // Move up (decrease y)
                app->player_dest.y = app->player_dest.y - 1;
                break;
            case 's': // Move down (increase y)
                app->player_dest.y = app->player_dest.y + 1;
                break;
            case 'a': // Move left (decrease x)
                app->player_dest.x = app->player_dest.x - 1;
                break;
            case 'd': // Move right (increase x)
                app->player_dest.x = app->player_dest.x + 1;
                break;
            case 'c':
                ryce_clear_pane(&app->panes.map);
                break;
            default:
                break;
            }
        }
    }

    free(buffer.events);
}

// --- Tick Actions ------------------------------------------------------ //
void tick_action(AppState *app) { move_player(app); }

// --- Render Actions ---------------------------------------------------- //
// Draws the map onto the TUI pane.
void render_map(AppState *app) {
    int pane_width = app->panes.map.view.width;
    int pane_height = app->panes.map.view.height;

    for (int ty = 0; ty < pane_height; ty++) {
        for (int tx = 0; tx < pane_width; tx++) {
            // Calculate the map coordinate, the offset from the TUI’s center is added
            // to the player’s map position.
            RYCE_Vec2 term_pos = {tx, ty};
            RYCE_Vec2 map_pos = ryce_from_terminal(&app->camera, &term_pos);
            int map_x = map_pos.x;
            int map_y = map_pos.y;

            // Draw the default empty glpyh if outside the map bounds.
            if (map_x < app->map.x.min || map_x > app->map.x.max || map_y < app->map.y.min || map_y > app->map.y.max) {
                ryce_pane_set(&app->panes.map, tx, ty, &RYCE_DEFAULT_GLYPH);
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
                    glyph.ch = app->entities[entity].glyph->ch;
                    glyph.style.part.fg_color = app->entities[entity].glyph->style.part.fg_color;
                    glyph.style.part.bg_color = app->entities[entity].glyph->style.part.bg_color;
                    if (position.z < app->player.z) {
                        // Add styling to the lower elevations.
                        glyph.style.part.style_flags = RYCE_STYLE_MODIFIER_DIM | RYCE_STYLE_MODIFIER_ITALIC;
                    }
                    break;
                }
            }

            ryce_pane_set(&app->panes.map, tx, ty, &glyph);
        }
    }
}

void render_debug(AppState *app) {
    RYCE_CHAR buffer[128]; // Used to store the debug information.

    // Ticks per Second.
    RYCE_SNPRINTF(buffer, sizeof(buffer), RYCE_LITERAL("TPS: %.2f  "), app->loop.tps);
    ryce_pane_set_str(&app->panes.debug, 0, 0, &RYCE_DEFAULT_STYLE, buffer);

    // Player Moving.
    bool is_moving = (app->player_dest.x != app->player.x || app->player_dest.y != app->player.y);
    RYCE_SNPRINTF(buffer, sizeof(buffer), RYCE_LITERAL("Player moving: %s "), is_moving ? "yes" : "no");
    ryce_pane_set_str(&app->panes.debug, 0, 1, &RYCE_DEFAULT_STYLE, buffer);

    // Player position.
    RYCE_SNPRINTF(buffer, sizeof(buffer), RYCE_LITERAL("Position: %lld, %lld, %lld"), app->player.x, -app->player.y,
                  app->player.z);
    ryce_pane_set_str(&app->panes.debug, 0, 2, &RYCE_DEFAULT_STYLE, buffer);
}

void render_action(AppState *app) {
    // Set the camera to be centered on the player in the event the player moved.
    app->camera.center = (RYCE_Vec2){.x = app->player.x, .y = app->player.y};

    // For each cell in the TUI, determine which map coordinate to show.
    render_map(app);

    // Draw the player at the center of the TUI.
    RYCE_Vec2 player = ryce_to_terminal(&app->camera, &(RYCE_Vec2){app->player.x, app->player.y});
    ryce_pane_set(&app->panes.map, player.x, player.y, &GLYPHS[6]);

    const bool is_moving = app->player_dest.x != app->player.x || app->player_dest.y != app->player.y;
    if (is_moving) {
        // Draw the player’s destination.
        RYCE_Vec2 dest_term = ryce_to_terminal(&app->camera, &(RYCE_Vec2){app->player_dest.x, app->player_dest.y});
        ryce_pane_set(&app->panes.map, dest_term.x, dest_term.y, &GLYPHS[7]);
    }

    // Render the debug pane.
    render_debug(app);

    // Render the TUI.
    RYCE_TuiError err_code = ryce_render_tui(&app->tui);
    if (err_code != RYCE_TUI_ERR_NONE) {
        ryce_clear_screen();
        fprintf(stderr, "Failed to render TUI: %d\n\r", err_code);
        lock = 1;
    }
}

int main(void) {
    srand((unsigned)time(nullptr));
    // Register SIGINT handler.
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    AppState app = {0};

    // Initialize the camera.
    RYCE_Vec2 term_size = get_terminal_size();
    const RYCE_Vec2 map_center = {.x = (MAP_LENGTH / 2) + 1, .y = (MAP_WIDTH / 2) + 1};
    if (ryce_init_camera_ctx(&app.camera, term_size.x, term_size.y, map_center) != RYCE_CAMERA_ERR_NONE) {
        fprintf(stderr, "Failed to init camera.\n");
        return EXIT_FAILURE;
    }

    // Initialize the TUI.
    if (ryce_init_tui_ctx(term_size.x, term_size.y, &app.tui) != RYCE_TUI_ERR_NONE) {
        fprintf(stderr, "Failed to init TUI.\n");
        return EXIT_FAILURE;
    }

    // Initialize the TUI pane.
    if (ryce_init_pane(0, 0, term_size.x, term_size.y, &app.tui, &app.panes.map) != RYCE_TUI_ERR_NONE) {
        fprintf(stderr, "Failed to init TUI pane.\n");
        return EXIT_FAILURE;
    }

    // Initialize the debug pane.
    if (ryce_init_pane(0, term_size.y - 3, 30, 3, &app.tui, &app.panes.debug) != RYCE_TUI_ERR_NONE) {
        fprintf(stderr, "Failed to init debug pane.\n");
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

    // Initialize entities and player.
    init_entities(&app);
    init_map(&app.map);
    app.player = init_player(&app);

    // ryce_clear_screen();
    do {
        input_action(&app);
        tick_action(&app);
        render_action(&app);
    } while (ryce_loop_tick(&app.loop) == RYCE_LOOP_ERR_NONE);

    ryce_input_join(&app.input);
    ryce_input_free_ctx(&app.input);
    return 0;
}
// NOLINTEND
