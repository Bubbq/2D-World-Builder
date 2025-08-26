#include "raylib.h"
#include "headers/raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "headers/gui_window_file_dialog.h"

#include "list.h"

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

void asset_free(void* to_free)
{
    if (!to_free)
        return;

    Asset* asset = (Asset*) to_free;

    if (asset->path) {
        free(asset->path); asset->path = NULL;
    }

    UnloadTexture(asset->texture);
    
    free(asset); asset = NULL;
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

// genaric ui utils

typedef struct
{
    Rectangle bounds;
    Rectangle content;
    Vector2 scrollbar;
} ScrollPanel;

void draw_infinite_grid(const Camera2D* camera, const float v_dist, const float h_dist)
{
    if (!camera) 
        return;

    Vector2 top_left = GetScreenToWorld2D((Vector2){0, 0}, (*camera));
    Vector2 bottom_right = GetScreenToWorld2D((Vector2){GetScreenWidth(), GetScreenHeight()}, (*camera));

    float start_x = floorf(top_left.x / v_dist) * v_dist;
    float start_y = floorf(top_left.y / h_dist) * h_dist;

    for (float x = start_x; x <= bottom_right.x; x += v_dist)
        DrawLine(x, top_left.y, x, bottom_right.y, GRAY);

    for (float y = start_y; y <= bottom_right.y; y += h_dist)
        DrawLine(top_left.x, y, bottom_right.x, y, GRAY);
}

void move_camera(Camera2D* camera)
{
    Vector2 delta = GetMouseDelta();
    delta = Vector2Scale(delta, -1.0f / camera->zoom);
    camera->target = Vector2Add(camera->target, delta);
}

// genaric ui utils


// application ui

void draw_top_bar(const Rectangle container)
{
    DrawRectangleRec(container, RED);
}

void draw_tile_scroll_panel(ScrollPanel* scroll_panel)
{
    if (!scroll_panel)
        return;

    const char* panel_text = "Tiles";
    const bool is_scrollbar_visible = false;
    GuiScrollPanel(scroll_panel->bounds, panel_text, scroll_panel->content, &scroll_panel->scrollbar, NULL, is_scrollbar_visible);
}

void draw_side_bar(const Rectangle container, ScrollPanel* tile_scroll_panel)
{
    draw_tile_scroll_panel(tile_scroll_panel);
}

void draw_world(World* world, Camera2D* camera, const Rectangle world_border)
{
    if (!world || !camera)
        return;

    DrawRectangleLinesEx(world_border, 1, GRAY);
    
    BeginScissorMode(world_border.x, world_border.y, world_border.width, world_border.height);
        BeginMode2D((*camera));
            draw_infinite_grid(camera, ON_SCREEN_TILE_SIZE, ON_SCREEN_TILE_SIZE);
        EndMode2D();
    EndScissorMode();
}

// application ui

// parse texture passed by the user
// need guiwindow file dialouge to queue the user
    // when you press the button, should show a window of all the files you have
    // once a PNG is pressed
        // texture should be parsed into tiles (from user specified asset sprite size, in the future suppourt specified width and height) and stored in some display list
        // path should be stored in the asset object

int main()
{
    editor_init();

    // GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());

    World world = world_init();

    const int padding = 5;

    Rectangle TOP_BAR = {
        .x = 0,
        .y = 0,
        .width = GetScreenWidth(),
        .height = 150,
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

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            move_camera(&world_camera);

        BeginDrawing();
            ClearBackground(WHITE);
            draw_top_bar(TOP_BAR);
            draw_side_bar(SIDE_BAR, &tile_scroll_panel);
            draw_world(&world, &world_camera, get_padded_rectangle(padding, WORLD_BORDER));
        EndDrawing();
    }

    world_free(&world);

    editor_free();

    return 0;
}
