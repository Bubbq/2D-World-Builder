#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"

#include "list.h"

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
    int index;
} AssetData;

typedef struct
{
    Vector2 world_position; 
    AssetData asset_data;
} Tile; 

typedef struct
{
    Camera2D camera;
    int tile_size;
} WorldSettings;

#define INITIAL_TILE_SIZE 32

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

#define FPS 60

#define ASSET_TILE_SIZE 16

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

void draw_tile_scroll_panel(ScrollPanel* scroll_panel)
{
    if (!scroll_panel)
        return;

    const char* panel_text = "Tiles";
    const bool is_scrollbar_visible = true;
    GuiScrollPanel(scroll_panel->bounds, panel_text, scroll_panel->content, &scroll_panel->scrollbar, NULL, is_scrollbar_visible);
}

void draw_side_bar(const Rectangle container, ScrollPanel* tile_scroll_panel)
{
    draw_tile_scroll_panel(tile_scroll_panel);
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

#define VALID_TEXTURE_EXT ".png"

bool configure_selected_filepath(GuiWindowFileDialogState* file_dialog_state, char* dest, const size_t dest_size)
{
    if (!file_dialog_state)
        return false;

    const int written = snprintf(dest, dest_size, "%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText);
    
    return (0 < written) && (written < dest_size);
}

void handle_file_select(GuiWindowFileDialogState* file_dialog_state)
{
    if (!file_dialog_state) 
        return;

    file_dialog_state->SelectFilePressed = false;
    
    if (!IsFileExtension(file_dialog_state->fileNameText, VALID_TEXTURE_EXT)) {
        fprintf(stderr, "handle_file_select: \"%s\" is not a %s\n", file_dialog_state->fileNameText, VALID_TEXTURE_EXT);
        return;
    }

    char asset_path[1024] = {0};
    if (!configure_selected_filepath(file_dialog_state, asset_path, sizeof(asset_path))) {
        fprintf(stderr, "handle_file_select: failed to configure asset path\n");
        return;
    }

    // TODO: add asset to asset_cache
    
    // TODO: create tiles and add to tile palette
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

void update_scroll_panel(const Rectangle container, const float padding, ScrollPanel* scroll_panel)
{
    if (scroll_panel) {
        scroll_panel->bounds = get_padded_rectangle(padding, container);
        
        // TODO: pass nelements scroll_panel contains to update the content rect
        scroll_panel->content = scroll_panel->bounds; 
    }
}

// bounds update

int main()
{
    const int padding = 5;

    editor_init();

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
        .width = 200,
        .height = GetScreenHeight() - top_bar.height,
    };

    ScrollPanel tile_scroll_panel = {
        .bounds = get_padded_rectangle(padding, side_bar),
        .content = tile_scroll_panel.bounds, // TODO: adjust the height based on the number of tiles that is loaded
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
        update_scroll_panel(side_bar, padding, &tile_scroll_panel);

        if (CheckCollisionPointRec(GetMousePosition(), world_border) && !file_dialog_state.windowActive) 
            handle_world_input(&world_settings);

        if (file_dialog_state.SelectFilePressed) 
            handle_file_select(&file_dialog_state);

        BeginDrawing();

            ClearBackground(WHITE);

            if (file_dialog_state.windowActive)
                GuiLock();
            
            draw_top_bar(top_bar, padding, &file_dialog_state.windowActive);
            draw_world(world_border, padding,&world, &world_settings);
            draw_side_bar(side_bar, &tile_scroll_panel);
            
            GuiUnlock();

            GuiWindowFileDialog(&file_dialog_state);

            DrawFPS(0, 0);
            
        EndDrawing();
    }

    world_free(&world);

    editor_free();

    return 0;
}