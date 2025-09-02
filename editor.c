#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"

#include "list.h"
#include "utils.h"
#include "asset_cache.h"

#define FPS 60
#define INITIAL_TILE_SIZE 32

#define VALID_ASSET_EXTENSION ".png"

#define SCROLLBAR_WIDTH 13
#define TILE_PALETTE_TILES_PER_ROW 10

// basic utils/misc

int nearest_multiple(const float value, const int multiple)
{
    const int quotient = roundf(value / multiple);
    const int nearest = quotient * multiple;
    return nearest;
}

int bound_value_to_interval(const int min, const int max, const int value)
{
    if (value > max)
        return max;

    else if (value < min)
        return min;

    else
        return value;
}

Vector2 vector2_scale(Vector2 vec, const float scale)
{
    return (Vector2) {
        .x = vec.x * scale,
        .y = vec.y * scale,
    };
}

Vector2 vector2_add(Vector2 a, Vector2 b)
{
    return (Vector2) {
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

void move_camera(Camera2D* camera)
{
    Vector2 delta = GetMouseDelta();
    delta = vector2_scale(delta, (-1.0f / camera->zoom));
    camera->target = vector2_add(camera->target, delta);
}

// basic utils/misc

typedef enum
{
	TILE_TYPE_WALL,
	TILE_TYPE_FLOOR,
	TILE_TYPE_DOOR,
	TILE_TYPE_BUFF,
	TILE_TYPE_INTERACTABLE,
    TILE_TYPE_N_ITEMS,
} TileType;

typedef struct
{
    Vector2 position;
    Texture* texture;
    unsigned long int id;
} AssetData;

void asset_data_print(void* data)
{
    if (data) {
        AssetData* asset_data = (AssetData*) data;
        printf("POSITION: (%f,%f), ID: %ld\n", asset_data->position.x, asset_data->position.y, asset_data->id);
    } 
}

typedef struct
{
    AssetData asset_data;
    Vector2 world_position; 
} Tile;

typedef struct
{
    Camera2D camera;
    int tile_size;
} WorldSettings;

WorldSettings world_settings_init()
{
    return (WorldSettings) {
        .tile_size = INITIAL_TILE_SIZE,
        .camera = (Camera2D) {
            .offset = (Vector2){0,0},
            .target = (Vector2){0,0},
            .rotation = 0.0f,
            .zoom = 1.0f
        },
    };
}

void adjust_tile_size(const float mouse_wheel_move, int* tile_size)
{
    if (!tile_size)
        return;
    
    const int delta = 4;
    const int min_tile_size = 8;
    const int max_tile_size = 128;

    // mouse_wheel_move is either -1, 0, or 1
    (*tile_size) = bound_value_to_interval(min_tile_size, max_tile_size, (*tile_size) + (delta * mouse_wheel_move));
}

void handle_world_input(WorldSettings* settings)
{
    if (!settings)
        return;

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        move_camera(&settings->camera);
    
    const float mouse_wheel_move = GetMouseWheelMove();

    if (mouse_wheel_move) 
        adjust_tile_size(mouse_wheel_move, &settings->tile_size);    
}

typedef struct
{
    List tiles[TILE_TYPE_N_ITEMS];
    Vector2 spawn_point;
} World;

World world_init()
{
    World world;

    world.spawn_point = (Vector2){0};

    for (int i = 0; i < TILE_TYPE_N_ITEMS; i++)
        world.tiles[i] = list_init();

    return world;
}

void world_free(World* world)
{
    for (int i = 0; i < TILE_TYPE_N_ITEMS; i++)
        list_free(&world->tiles[i]);
}

void editor_init()
{
    SetTargetFPS(FPS);    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 1000, "Editor");
}

void editor_free()
{
    CloseWindow();
}

// ui 

Rectangle get_padded_rectangle(const float padding, const Rectangle rect)
{
    return (Rectangle) {
        .x = rect.x + padding,
        .y = rect.y + padding,
        .width  = rect.width  - (padding * 2),
        .height = rect.height - (padding * 2),
    };
}

Rectangle get_panel_content_bounds(const Rectangle gui_panel_bounds)
{
    const float delta = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - 1;

    return (Rectangle) {
        .x = gui_panel_bounds.x,
        .y = gui_panel_bounds.y + delta,
        .width  = gui_panel_bounds.width,
        .height = gui_panel_bounds.height - delta,
    };
}

// TODO: somehow out of sync with GuiScrollPanel's 'needsVerticalScrollbar' variable
bool is_scrollbar_visible(const float content_height, const float bounds_height)
{
    return content_height > (bounds_height - (2 * GuiGetStyle(DEFAULT, BORDER_WIDTH)));
}

typedef struct
{
    Rectangle bounds;
    Rectangle content;
    Vector2 scrollbar;
} ScrollPanel;

Rectangle get_world_bounds(const Rectangle screen_bounds, const Camera2D camera)
{
    const Vector2 tr_screen = (Vector2) {
        .x = screen_bounds.x,
        .y = screen_bounds.y,
    };

    const Vector2 tr_world = GetScreenToWorld2D(tr_screen, camera);

    return (Rectangle) {
        .x = tr_world.x,
        .y = tr_world.y,
        .width = screen_bounds.width,
        .height = screen_bounds.height,
    };
}

void draw_infinite_grid(const Rectangle bounds, const float v_dist, const float h_dist)
{
    const float x0 = nearest_multiple(bounds.x, v_dist);

    for (float x = x0; x <= bounds.x + bounds.width; x += v_dist)
        DrawLine(x, bounds.y, x, bounds.y + bounds.height, BLACK);

    const float y0 = nearest_multiple(bounds.y, h_dist);
    
    for (float y = y0; y <= bounds.y + bounds.height; y += h_dist)
        DrawLine(bounds.x, y, bounds.x + bounds.width, y, BLACK);
}

void layout_dynamic_bar(const Rectangle container, const float padding, Rectangle* recs, const size_t nrecs)
{
    const float bar_w = container.width - (padding * (nrecs + 1));

    const float widget_w = bar_w / nrecs;
    const float widget_h = container.height - (padding * 2);

    const float x0 = container.x + padding;
    const float y0 = container.y + padding;

    for (size_t i = 0; i < nrecs; i++) {
        recs[i] = (Rectangle) {
            .x = x0 + (i * widget_w) + (i > 0 ? padding : 0),
            .y = y0,
            .width = widget_w,
            .height = widget_h,
        };
    }
}

float get_tile_palette_size(const bool is_scrollbar_visible, const float container_width)
{
    const float availible_width = container_width - (is_scrollbar_visible ? SCROLLBAR_WIDTH : 0);
    return availible_width / (TILE_PALETTE_TILES_PER_ROW * 1.0f);
}

void draw_top_bar(const Rectangle container, const float padding, bool* load_window_active)
{
    const size_t widget_count = 2;
    Rectangle widget_bounds[widget_count];
    layout_dynamic_bar(container, padding, widget_bounds, widget_count);
    
    if (!load_window_active)
        GuiSetState(STATE_DISABLED);

    if (GuiButton(widget_bounds[0], GuiIconText(ICON_FILE_OPEN, "LOAD")))
        (*load_window_active) = true;

    GuiSetState(STATE_NORMAL);

    if (GuiButton(widget_bounds[1], GuiIconText(ICON_FILE_SAVE, "SAVE")))
        ;
}

void draw_tile_scroll_panel(ScrollPanel* scroll_panel, List* tile_palette, const float sprite_size)
{
    if (!scroll_panel || !tile_palette)
        return;

    const char* panel_text = "Tiles";
    const bool show_scrollbar = true;
    GuiScrollPanel(scroll_panel->bounds, panel_text, scroll_panel->content, &scroll_panel->scrollbar, NULL, show_scrollbar);

    const Rectangle tile_palette_container = get_panel_content_bounds(scroll_panel->bounds);
    const float     tile_palette_size      = get_tile_palette_size(is_scrollbar_visible(scroll_panel->content.height, scroll_panel->bounds.height), tile_palette_container.width);

    const Rectangle scissor_rect = get_padded_rectangle(1, tile_palette_container);

    BeginScissorMode(scissor_rect.x, scissor_rect.y, scissor_rect.width, scissor_rect.height);
    
        Vector2 tile_position = {tile_palette_container.x, tile_palette_container.y};

        int i = 0;
        for (Node* node = tile_palette->head; node; node = node->next, i++, tile_position.x += tile_palette_size) {
            if ((i != 0) && ((i % TILE_PALETTE_TILES_PER_ROW) == 0)) {
                tile_position = (Vector2) {
                    .x = tile_palette_container.x,
                    .y = (tile_position.y + tile_palette_size)
                };
            }  
            
            AssetData* metadata = (AssetData*) node->content;

            const Rectangle src_rect = {
                .x = metadata->position.x,
                .y = metadata->position.y,
                .width = sprite_size,
                .height = sprite_size,
            };

            const Rectangle dest_rect = {
                .x = tile_position.x,
                .y = tile_position.y + scroll_panel->scrollbar.y,
                .width = tile_palette_size,
                .height = tile_palette_size
            };

            DrawTexturePro((*metadata->texture), src_rect, dest_rect, (Vector2){0,0}, 0.0f, WHITE);
        }

    EndScissorMode();
}

void draw_side_bar(const Rectangle container, ScrollPanel* tile_scroll_panel, List* tile_palette, const float sprite_size)
{
    draw_tile_scroll_panel(tile_scroll_panel, tile_palette, sprite_size);
}

void draw_world(const Rectangle container, const float padding, World* world, WorldSettings* settings)
{
    if (!world || !settings)
        return;
    
    const Rectangle padded_container = get_padded_rectangle(padding, container);

    if ((padded_container.width > 0) && (padded_container.height > 0)) 
        DrawRectangleLinesEx(padded_container, 1, GRAY);

    BeginScissorMode(padded_container.x, padded_container.y, padded_container.width, padded_container.height);
        BeginMode2D(settings->camera);
            const Rectangle world_bounds = get_world_bounds(padded_container, settings->camera);
            draw_infinite_grid(world_bounds, settings->tile_size, settings->tile_size);
        EndMode2D();
    EndScissorMode();
}

// ui

// GuiWindowFileDialogState stuff

bool configure_selected_filepath(GuiWindowFileDialogState* file_dialog_state, char* dest, const size_t dest_size)
{
    if (!file_dialog_state || !dest)
        return false;

    const int written = snprintf(dest, dest_size, "%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText);
    
    return (0 < written) && (written < dest_size);
}

void update_tile_scroll_panel(const float ntiles, ScrollPanel* scroll_panel)
{
    if (!scroll_panel)
        return;

    const int nrows = ceil(ntiles / TILE_PALETTE_TILES_PER_ROW);
    const float tile_palette_size = get_tile_palette_size(is_scrollbar_visible(scroll_panel->content.height, scroll_panel->bounds.height), scroll_panel->bounds.width);

    scroll_panel->content.height = (nrows) * tile_palette_size;
}

void parse_asset_entry(AssetEntry* entry, List* tile_palette, const float sprite_size)
{
    if (!asset_entry_is_ready(entry) || !tile_palette)
        return;

    const unsigned long int id = entry->id;

    Texture* texture = &entry->texture;

    for (float y = 0; y < texture->height; y += sprite_size) {
        for (float x = 0; x < texture->width; x += sprite_size) {
            AssetData* asset_data = malloc(sizeof(AssetData));
            if (asset_data) {
                asset_data->id = id;
                asset_data->texture = texture;
                asset_data->position = (Vector2){x,y};
                list_append(tile_palette, node_init(asset_data, sizeof(AssetData), asset_data_print, free));
            }
        }
    }
}

void handle_file_select(ScrollPanel* tile_scroll_panel, GuiWindowFileDialogState* file_dialog_state, AssetCache* cache, List* tile_palette, const float sprite_size)
{
    if (!tile_scroll_panel || !file_dialog_state || !cache || !tile_palette) 
        return;

    file_dialog_state->SelectFilePressed = false;

    char asset_path[1024] = {0};
    if (!configure_selected_filepath(file_dialog_state, asset_path, sizeof(asset_path))) {
        fprintf(stderr, "handle_file_select: failed to configure asset path\n");
        return;
    }

    if (!is_file_extension(asset_path, VALID_ASSET_EXTENSION)) {
        fprintf(stderr, "handle_file_select: \"%s\" is not a %s\n", asset_path, VALID_ASSET_EXTENSION);
        return;
    }

    AssetEntry* existing = asset_cache_find(cache, hash_string(asset_path));
    if (existing) {
        fprintf(stderr, "handle_file_select: the asset \"%s\" is already in the cache\n", asset_path);
        return;
    }

    AssetEntry* new_entry = asset_entry_init(asset_path);
    if (!new_entry) {
        fprintf(stderr, "handle_file_select: failed to create new AssetEntry object\n");
        return;
    }

    asset_cache_add(cache, new_entry);

    parse_asset_entry(new_entry, tile_palette, sprite_size);
    
    update_tile_scroll_panel(tile_palette->count, tile_scroll_panel);
}

// GuiWindowFileDialogState stuff

// bounds update

void update_ui_zones(Rectangle* top_bar, Rectangle* side_bar, Rectangle* world_border)
{
    if (!top_bar || !side_bar || !world_border)
        return;

    top_bar->width = GetScreenWidth();
    side_bar->height = GetScreenHeight() - top_bar->height;
    world_border->width = GetScreenWidth() - side_bar->width;
    world_border->height = GetScreenHeight() - top_bar->height;
}

// bounds update

// TODO: handle highlighting hovering nodes and the selected one 
// TODO: make PaletteManager struct to handle updates and input for the tile palette system?
// TODO: make AssetManager to contain the cache and sprite size of the editing session?
// TODO: better function to handle the updates in the scroll panels content rect and bound rect (scroll_panel_update)

// BUGS
    // width adjustment when drawing tiles not in sync with GuiScrollPanel scrollbar's visibility

int main()
{
    const int padding = 5;
    
    float sprite_size = 16.0f;

    editor_init();

    List tile_palette = list_init();

    AssetCache asset_cache = NULL;

    GuiWindowFileDialogState file_dialog_state = InitGuiWindowFileDialog(GetWorkingDirectory());

    World world = world_init();
    WorldSettings world_settings = world_settings_init();

    Rectangle top_bar = {
        .x = 0,
        .y = 0,
        .width = GetScreenWidth(),
        .height = 35,
    };
    
    Rectangle side_bar = {
        .x = 0,
        .y = top_bar.y + top_bar.height,
        .width = GetScreenWidth() * 0.30f,
        .height = GetScreenHeight() - top_bar.height,
    };

    ScrollPanel tile_scroll_panel = {
        .bounds = get_padded_rectangle(padding, side_bar),
        .content = tile_scroll_panel.bounds, 
        .scrollbar = (Vector2){0,0},
    };

    Rectangle world_border = {
        .x = side_bar.x + side_bar.width,
        .y = top_bar.y + top_bar.height,
        .width = GetScreenWidth() - side_bar.width,
        .height = GetScreenHeight() - top_bar.height,
    };
    
    while (!WindowShouldClose())
    {
        update_ui_zones(&top_bar, &side_bar, &world_border);

        tile_scroll_panel.bounds = get_padded_rectangle(padding, side_bar);

        if (CheckCollisionPointRec(GetMousePosition(), world_border) && !file_dialog_state.windowActive) 
            handle_world_input(&world_settings);

        if (file_dialog_state.SelectFilePressed) 
            handle_file_select(&tile_scroll_panel, &file_dialog_state, &asset_cache, &tile_palette, sprite_size);

        BeginDrawing();

            ClearBackground(WHITE);

            if (file_dialog_state.windowActive)
                GuiLock();
            
            draw_top_bar(top_bar, padding, &file_dialog_state.windowActive);
            draw_world(world_border, padding, &world, &world_settings);
            draw_side_bar(side_bar, &tile_scroll_panel, &tile_palette, sprite_size);
            
            GuiUnlock();

            GuiWindowFileDialog(&file_dialog_state);

            DrawFPS(0, 0);
            
        EndDrawing();
    }

    list_free(&tile_palette);

    asset_cache_free(&asset_cache);

    world_free(&world);

    editor_free();

    return 0;
}