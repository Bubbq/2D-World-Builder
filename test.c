#include "raylib.h"
#include "headers/raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "headers/gui_window_file_dialog.h"

#include "list.h"
#include "utils.h"

// basic utils/misc

int nearest_multiple(const float value, const int multiple)
{
    const int quotient = roundf(value / multiple);
    const int nearest = quotient * multiple;
    return nearest;
}

bool get_selected_filepath(GuiWindowFileDialogState* file_dialog_state, char* dest, const size_t dest_size)
{
    if (!file_dialog_state)
        return false;

    const int written = snprintf(dest, dest_size, "%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText);
    
    return (0 < written) && (written < dest_size);
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
    Texture texture;
    char* path;
} Asset;

Asset* asset_init(const char* path)
{
    if (!valid_string(path))
        return NULL;

    Asset* asset = malloc(sizeof(Asset));
    if (!asset)
        return NULL;

    asset->path = strdup(path);
    asset->texture = LoadTexture(path);

    return asset;
}

void asset_free(void* to_free)
{
    if (!to_free)
        return;

    Asset* asset = (Asset*) to_free;

    if (asset->path) {
        free(asset->path); asset->path = NULL;
    }

    if (IsTextureReady(asset->texture))
        UnloadTexture(asset->texture);
    
    free(asset); asset = NULL;
}

bool asset_is_ready(Asset* asset)
{
    return (asset) && (valid_string(asset->path)) && (IsTextureReady(asset->texture));
}

void asset_append(List* asset_list, const char* asset_path)
{
    if (!asset_list || !valid_string(asset_path))
        return;

    Asset* asset = asset_init(asset_path);
    if (!asset_is_ready(asset)) {
        fprintf(stderr, "asset_append: failed to create Asset object\n");
        asset_free(asset);
        return;
    }

    // TODO: create a asset print function for debugging
    const print_content_funct print_funct = NULL;

    Node* node = node_init(asset, sizeof(Asset), print_funct, asset_free);
    if (!node) {
        fprintf(stderr, "asset_append: failed to create Node object\n");
        asset_free(asset);
        return;
    }

    list_append(asset_list, node);
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
#define ON_SCREEN_TILE_SIZE 32
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

Rectangle get_padded_rectangle(const float padding, const Rectangle rect)
{
    return (Rectangle) {
        .x = rect.x + padding,
        .y = rect.y + padding,
        .width  = rect.width  - (padding * 2),
        .height = rect.height - (padding * 2),
    };
}

Rectangle get_gui_panel_content_bounds(const Rectangle gui_panel_bounds)
{
    return (Rectangle) {
        .x = gui_panel_bounds.x,
        .y = gui_panel_bounds.y + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT,
        .width  = gui_panel_bounds.width,
        .height = gui_panel_bounds.height,
    };
}

// ui 

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

void move_camera(Camera2D* camera)
{
    Vector2 delta = GetMouseDelta();
    delta = Vector2Scale(delta, -1.0f / camera->zoom);
    camera->target = Vector2Add(camera->target, delta);
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

void draw_world(World* world, Camera2D* camera, const Rectangle container)
{
    if (!world || !camera)
        return;
    
    if ((container.width > 0) && (container.height > 0))
        DrawRectangleLinesEx(container, 1, GRAY);

    BeginScissorMode(container.x, container.y, container.width, container.height);
        BeginMode2D((*camera));
            const Rectangle world_bounds = get_world_bounds(container, (*camera));
            draw_infinite_grid(world_bounds, ON_SCREEN_TILE_SIZE, ON_SCREEN_TILE_SIZE);
        EndMode2D();
    EndScissorMode();
}

// ui

#define VALID_TEXTURE_EXT ".png"

void handle_file_select(GuiWindowFileDialogState* file_dialog_state, List* asset_list)
{
    if (!file_dialog_state || !asset_list) 
        return;

    if (!IsFileExtension(file_dialog_state->fileNameText, VALID_TEXTURE_EXT)) {
        fprintf(stderr, "handle_file_select: \"%s\" is not a %s\n", file_dialog_state->fileNameText, VALID_TEXTURE_EXT);
        return;
    }

    char asset_path[1024] = {0};
    if (!get_selected_filepath(file_dialog_state, asset_path, sizeof(asset_path))) {
        fprintf(stderr, "handle_file_select: failed to configure asset path\n");
        return;
    }

    asset_append(asset_list, asset_path);

    // TODO: parse texture into tiles based on user's asset size
}

int main()
{
    editor_init();

    GuiWindowFileDialogState file_dialog_state = InitGuiWindowFileDialog(GetWorkingDirectory());

    List assets = list_init();

    World world = world_init();

    const int padding = 5;

    Rectangle TOP_BAR = {
        .x = 0,
        .y = 0,
        .width = GetScreenWidth(),
        .height = 35,
    };
    
    Rectangle SIDE_BAR = {
        .x = 0,
        .y = TOP_BAR.y + TOP_BAR.height,
        .width = 200,
        .height = GetScreenHeight() - TOP_BAR.height,
    };

    ScrollPanel tile_scroll_panel = {
        .bounds = get_padded_rectangle(padding, SIDE_BAR),
        .content = tile_scroll_panel.bounds, // TODO: adjust the height based on the number of tiles that is loaded
        .scrollbar = (Vector2){0,0},
    };

    Rectangle WORLD_BORDER = {
        .x = SIDE_BAR.x + SIDE_BAR.width,
        .y = TOP_BAR.y + TOP_BAR.height,
        .width = GetScreenWidth() - SIDE_BAR.width,
        .height = GetScreenHeight() - TOP_BAR.height,
    };

    Camera2D world_camera = {
        .offset = (Vector2){0,0},
        .target = (Vector2){0,0},
        .rotation = 0.0f,
        .zoom = 1.0f
    };
    
    while (!WindowShouldClose())
    {
        TOP_BAR.width = GetScreenWidth();
        
        SIDE_BAR.height = GetScreenHeight() - TOP_BAR.height;
        tile_scroll_panel.bounds = get_padded_rectangle(padding, SIDE_BAR);
        tile_scroll_panel.content = tile_scroll_panel.bounds;
        
        WORLD_BORDER.width = GetScreenWidth() - SIDE_BAR.width;
        WORLD_BORDER.height = GetScreenHeight() - TOP_BAR.height;

        if (CheckCollisionPointRec(GetMousePosition(), WORLD_BORDER) && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            move_camera(&world_camera);

        if (file_dialog_state.SelectFilePressed) {
            file_dialog_state.SelectFilePressed = false;
            handle_file_select(&file_dialog_state, &assets);
        }

        BeginDrawing();

            ClearBackground(WHITE);

            if (file_dialog_state.windowActive) 
                GuiLock();
            
            draw_top_bar(TOP_BAR, padding, &file_dialog_state.windowActive);
            draw_world(&world, &world_camera, get_padded_rectangle(padding, WORLD_BORDER));
            draw_side_bar(SIDE_BAR, &tile_scroll_panel);
            
            GuiUnlock();

            GuiWindowFileDialog(&file_dialog_state);

        EndDrawing();
    }

    list_free(&assets);

    world_free(&world);

    editor_free();

    return 0;
}