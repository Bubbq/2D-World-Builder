#include <limits.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/animation.h"
#include "headers/raylib.h"
#include "headers/tile.h"
#include "headers/worldbuilder.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "headers/gui_window_file_dialog.h"

const int TILE_SIZE = 16;
const int SCREEN_TILE_SIZE = 32;

typedef struct
{
    Tile curr_tile;
    TileList avail_tiles;
    TileList* curr_layer;
    Texture2D curr_texture;
    TileType curr_type;
    Vector2 mouse_pos;
    char path[CHAR_LIMIT];
    float width;
    float height;
    float animation_speed;
    float nframes;
} Editor;

const int FPS = 60;

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;

const char* SPAWN_PATH = "spawn.txt";

const Tile NULL_TILE = {(Vector2){}, (Vector2){}, (Texture2D){}, UNDF, "", false, (Animation){}};

void drawTile(Tile tile, int size) { DrawTexturePro(tile.tx, get_object_area(tile.src, TILE_SIZE), get_object_area(tile.sp, size), (Vector2){0,0}, 0, WHITE);}

Vector2 get_gridded_mouse_position(Camera2D camera)
{
    Vector2 pos = GetScreenToWorld2D(GetMousePosition(), camera);

    pos.x = (((int)pos.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
    pos.y = (((int)pos.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
    
    return pos;
}

Editor create_editor()
{
    Editor editor;

    editor.curr_tile = NULL_TILE;
    editor.avail_tiles = create_tilelist();
    editor.curr_layer = NULL;
    editor.animation_speed = 0;
    editor.nframes = 0;

    return editor;
}

void read_image(TileList* avail_tiles, Texture2D texture, char* path)
{
    Vector2 cp = {};

    while(CheckCollisionPointRec(cp, (Rectangle) {0,0,texture.width, texture.height }))
    {
        Tile tile;

        tile.src = cp;
        tile.sp = (Vector2){};
        tile.tx = texture;
        strcpy(tile.fp, path);
        
        add_tile(avail_tiles, tile);

        if((cp.x += TILE_SIZE) == texture.width) 
            cp = (Vector2){ 0, (cp.y + TILE_SIZE) };
    }
}

void load_button(GuiWindowFileDialogState* directory, World* world, TileList* avail_tiles, Tile* curr_tile, Texture2D* curr_texture, char* path)
{
    if (GuiButton((Rectangle){ 70, 3, 65, 20 }, GuiIconText(ICON_FILE_OPEN, "LOAD"))) 
        directory->windowActive = true;

    if(directory->SelectFilePressed)
    {
        *curr_tile = NULL_TILE;

		strcpy(path,TextFormat("%s" PATH_SEPERATOR "%s", directory->dirPathText,directory->fileNameText));
        
        // loading sprite
        if (IsFileExtension(directory->fileNameText, ".png"))
		{
			*curr_texture = add_texture(&world->textures, path);
            read_image(avail_tiles, *curr_texture, path);
		}

        // loading world
        if (IsFileExtension(directory->fileNameText, ".txt"))
        {
            clear_world(world);
            world->spawn = get_spawn_point(SPAWN_PATH);
            load_world(world, path);
        }

        strcpy(path, "");
        directory->SelectFilePressed = false;
    }
}

void delete_world_button(World* world)
{ 
    if(GuiButton((Rectangle){ 137, 3, 65, 20 }, "DELETE")) 
        clear_world(world); 
}

void focus_button(Camera2D* camera)
{ 
    if(GuiButton((Rectangle){ 204, 3, 50, 20 }, "FOCUS")) 
        camera->target = (Vector2){}; 
}

void save_spawn_point(Vector2 spawn)
{
    FILE* file = fopen(SPAWN_PATH, "w");
    if(file == NULL)
    {
        printf("failed to save spawn point\n");
        return;
    }

    fprintf(file, "%.2f,%.2f\n", spawn.x, spawn.y);
    fclose(file);
}

void save_button(World* world, Tile* curr_tile, bool* showInputTextBox, char* path)
{
    if (GuiButton((Rectangle){ 3, 3, 65, 20 },GuiIconText(ICON_FILE_SAVE, "SAVE"))) 
        *showInputTextBox = true;

    if(*showInputTextBox)
    {
        *curr_tile = NULL_TILE; 
        int result = GuiTextInputBox((Rectangle){ 376, 436, 240, 140 },GuiIconText(ICON_FILE_SAVE, "Save file as..."),"file name:", "Ok;Cancel", path, 255, NULL);

        // saving world 
        if((result == 1) && (strlen(path) > 0))
        {
            save_spawn_point(world->spawn);
            save_tilelist(&world->walls, path);
            save_tilelist(&world->doors, path);
            save_tilelist(&world->floors, path);
            save_tilelist(&world->health_buffs, path);
            save_tilelist(&world->damage_buffs, path);
            save_tilelist(&world->interactables, path);
        }

        // reset flag upon saving and premature exit
		if ((result == 0) || (result == 1) || (result == 2))
		{
			*showInputTextBox = false;
			strcpy(path, "");
		}
    }
}

void clear_workspace_button(TileList* avail_tiles, Tile* curr_tile)
{
    if (GuiButton((Rectangle){ 235, 32, 24, 24 } ,GuiIconText(ICON_BIN, "")))
	{
		*curr_tile = NULL_TILE;
        clear_tilelist(avail_tiles);
	}
}

void tile_workspace(TileList* avail_tiles, Tile* curr_tile)
{
    const int TILES_PER_ROW = 8;
    const int MAX_ROWS = 15;

    const int NTILES = TILES_PER_ROW * MAX_ROWS;
    int itr = avail_tiles->size > NTILES ? NTILES : avail_tiles->size;

    GuiPanel((Rectangle){ 3, 32, 256, 504 }, "TILES");

    clear_workspace_button(avail_tiles, curr_tile);
    
    Vector2 cp = { 3, 56 };

    for(int i = 0; i < itr; i++, (cp.x += SCREEN_TILE_SIZE))
    {
        // next row transition
        if(((i % TILES_PER_ROW) == 0) && (i != 0)) 
            cp = (Vector2){ 3, (cp.y + SCREEN_TILE_SIZE) };

        Tile tile = avail_tiles->list[i];
        tile.sp = cp; 

        drawTile(tile, SCREEN_TILE_SIZE);

        // choosing a tile 
        if (CheckCollisionPointRec(GetMousePosition(),get_object_area(tile.sp, SCREEN_TILE_SIZE)))
        {
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
                *curr_tile = tile;

            DrawRectangleLinesEx(get_object_area(tile.sp, SCREEN_TILE_SIZE), 2, BLUE);
        }
    }
    
    // highlight current tile
    if(curr_tile->tx.id > 0) 
        DrawRectangleLinesEx(get_object_area(curr_tile->sp, SCREEN_TILE_SIZE), 2, GREEN);
}

void select_layer_panel(World* world, TileType* curr_type, TileList** curr_layer)
{
    GuiPanel((Rectangle){ 829, 32, 160, 210 }, "LAYER TO EDIT");
    GuiToggleGroup((Rectangle){ 845, 72, 128, 24 },"WALLS \n FLOORS \n DOORS \n BUFFS (HEALTH) \n BUFFS (DAMAGE) \n INTERACTABLES", curr_type);

    // updating layer
    switch (*curr_type)
    {
        case WALL: *curr_layer = &world->walls; break;
        case DOOR: *curr_layer = &world->doors; break;
        case FLOOR: *curr_layer = &world->floors; break;
        case HEALTH_BUFF: *curr_layer = &world->health_buffs; break;
        case DAMAGE_BUFF: *curr_layer = &world->damage_buffs; break;
        case INTERACTABLE: *curr_layer = &world->interactables; break;
        default:
            break;
    }
}

void select_spawn_point(Rectangle world_area, Vector2* spawn, Camera2D camera)
{
    Vector2 spawn_point = GetScreenToWorld2D(GetMousePosition(), camera); 
    
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(spawn_point, world_area)) 
        *spawn = GetScreenToWorld2D(GetMousePosition(), camera); 
}

void draw_world(World* world)
{
    draw_tilelist(&world->walls,world->area, RED, SCREEN_TILE_SIZE, "W");
    draw_tilelist(&world->floors,world->area, ORANGE,SCREEN_TILE_SIZE, "F");
    draw_tilelist(&world->doors,world->area, YELLOW, SCREEN_TILE_SIZE, "D");
    draw_tilelist(&world->health_buffs, world->area, GREEN, SCREEN_TILE_SIZE, "HB");
    draw_tilelist(&world->damage_buffs, world->area, BLUE, SCREEN_TILE_SIZE, "DB");
    draw_tilelist(&world->interactables, world->area,PURPLE, SCREEN_TILE_SIZE, "I");
    DrawText("S", world->spawn.x, world->spawn.y, 5, PINK);
    DrawRectangleLinesEx(world->area, 3, GRAY);
}

void edit_layer(Editor* editor, Rectangle world_area)
{
    // deletion
    if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) 
    {
        for(int i = 0; i < editor->curr_layer->size; i++)
            if(Vector2Equals(editor->mouse_pos, editor->curr_layer->list[i].sp)) 
                delete_tile(editor->curr_layer, i);
    }

    // creation
    if((editor->curr_tile.tt != UNDF) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        bool add = true;
        
        // prevent adding tile on same space
        for(int i = 0; (i < editor->curr_layer->size) && (add); i++)
        {
            if(Vector2Equals(editor->mouse_pos, editor->curr_layer->list[i].sp)) 
                add = false;
        }

        if(add)
        {
            bool animate = ((editor->animation_speed > 0) && (editor->nframes > 0));
            Animation tile_animation;
            Tile tile;

            tile_animation.nframes = editor->nframes;
            tile_animation.fcount = 0;
            tile_animation.fspeed = animate ? (FPS / editor->animation_speed) : 0;
            tile_animation.xfposition = tile_animation.yfposition = 0;

            tile.src = editor->curr_tile.src;
            tile.sp = editor->mouse_pos;    
            tile.tx = editor->curr_tile.tx;    
            tile.tt = editor->curr_type;
            strcpy(tile.fp, editor->curr_tile.fp);
            tile.animated = animate;
            tile.animtaion = tile_animation;
            
            add_tile(editor->curr_layer, tile);
        }
    }
}

void edit_world_size_panel(float* world_width, float* world_height)
{
    GuiPanel((Rectangle){ 829 , 280, 160, 90 }, "WORLD SIZE");
    
    DrawText("WIDTH", 860, 315, 10, GRAY);
    DrawText("HEIGHT", 860, 345, 10, GRAY);

    // adj width
    if(GuiButton((Rectangle){ 910, 310, 20, 20 }, "+")) 
        *world_width += SCREEN_TILE_SIZE; 
    if(GuiButton((Rectangle){ 933, 310, 20, 20 }, "-") && (*world_width > SCREEN_TILE_SIZE)) 
        *world_width -= SCREEN_TILE_SIZE;

    // adj height
    if(GuiButton((Rectangle){ 910, 340, 20, 20 }, "+")) 
        *world_height += SCREEN_TILE_SIZE; 
    if(GuiButton((Rectangle){ 933, 340, 20, 20 }, "-") && (*world_height > SCREEN_TILE_SIZE))
         *world_height -= SCREEN_TILE_SIZE;
}

void edit_tile_animation_panel(float* fps, float* nframes)
{
    char text[CHAR_LIMIT];

    GuiPanel((Rectangle){3, 545, 256, 80}, "ANIMATION");

    sprintf(text, "%.0f FPS", *fps);
    GuiSliderBar((Rectangle){ 53, 580, 85, 10 }, "SPEED", text, fps, 0, 10);

    sprintf(text, "%.0f", *nframes);
    GuiSliderBar((Rectangle){ 90, 605, 85, 10 }, "# OF FRAMES", text, nframes, 0, 10);
}

void move_world(Camera2D* camera)
{
    if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera->zoom);
        camera->target = Vector2Add(camera->target, delta);
    }
}

void update_edit_state(EditState* state, bool is_window_active)
{
    if(IsKeyPressed(KEY_E)) 
        *state = EDIT;
    else if(IsKeyPressed(KEY_S)) 
        *state = SPAWN;
    else if(IsKeyPressed(KEY_F) || is_window_active) 
        *state = FREE;
}

void display_current_edit_state(EditState state)
{
    char text[2];

    switch (state)
    {
        case EDIT: sprintf(text, "E"); break;
        case SPAWN: sprintf(text, "S"); break;
        case FREE: sprintf(text, "F"); break;
    }

    DrawText(text, (SCREEN_WIDTH - 20), (SCREEN_HEIGHT - 20), 20, DARKGRAY);
}

int main()
{
    World world = create_world();
    world.area = (Rectangle){ 288, 32, 512, 512 };

    EditState state = FREE;
    Editor editor = create_editor();

    Camera2D camera;
    camera.zoom = 1;
    
    SetTargetFPS(FPS);
    SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");

    bool showInputTextBox = false;
    GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
    
    // preventing adding tiles when on other panels
    const Rectangle DEAD_ZONE_1 = {3, 32, 256, 584};
    const Rectangle DEAD_ZONE_2 = {829, 32, 160, 362};

    while(!WindowShouldClose())
    {
        editor.mouse_pos = get_gridded_mouse_position(camera);
        update_edit_state(&state, (showInputTextBox || fileDialogState.windowActive));
        
        BeginDrawing();
            ClearBackground(RAYWHITE);  
            
            switch (state)
            {
                case FREE: move_world(&camera); break;
                case SPAWN: select_spawn_point(world.area, &world.spawn, camera); break;
                case EDIT: 
                    if((editor.curr_layer != NULL) && CheckCollisionPointRec(editor.mouse_pos, world.area) && (!CheckCollisionPointRec(GetMousePosition(), DEAD_ZONE_1) && !CheckCollisionPointRec(GetMousePosition(), DEAD_ZONE_2)))
                    {
                        BeginMode2D(camera);
                            edit_layer(&editor, world.area);
                            DrawRectangleLinesEx(get_object_area(editor.mouse_pos, SCREEN_TILE_SIZE), 2, RED);
                        EndMode2D();
                    }
                    break;
                default:
                    break;
            }
            
            BeginMode2D(camera);
                draw_world(&world);
            EndMode2D();

            display_current_edit_state(state);
            tile_workspace(&editor.avail_tiles, &editor.curr_tile);
            select_layer_panel(&world, &editor.curr_type, &editor.curr_layer);
            edit_world_size_panel(&world.area.width, &world.area.height);
            edit_tile_animation_panel(&editor.animation_speed, &editor.nframes);
            delete_world_button(&world);
            focus_button(&camera);
            save_button(&world, &editor.curr_tile, &showInputTextBox, editor.path);
            load_button(&fileDialogState, &world, &editor.avail_tiles, &editor.curr_tile, &editor.curr_texture, editor.path);
            GuiWindowFileDialog(&fileDialogState);
        EndDrawing();
    }
    
    dealloc_world(&world);
    unload_textures(&world.textures);
    CloseWindow();    
    return 0;
}