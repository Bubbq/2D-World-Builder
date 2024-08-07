#include <limits.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "worldbuilder.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"

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

void resizeTileList(TileList* tl) 
{
    tl->cap *= 2;
    tl->list = realloc(tl->list, tl->cap);
}

void addTile(TileList* tl, Tile t) 
{
    if(tl->size * sizeof(Tile) == tl->cap) resizeTileList(tl);
    tl->list[tl->size++] = t;
}

void deleteTile(TileList* tl, int pos)
{
    for(int i = pos; i < tl->size; i++) tl->list[i] = tl->list[i + 1];
    tl->size--;
}

void deleteWorld(World* world)
{
    world->spawn = (Vector2){INT_MIN, INT_MIN};
    while(world->walls.size != 0) deleteTile(&world->walls, 0);
    while(world->doors.size != 0) deleteTile(&world->doors, 0);
    while(world->floors.size != 0) deleteTile(&world->floors, 0);
    while(world->health_buffs.size != 0) deleteTile(&world->health_buffs, 0);
    while(world->damage_buffs.size != 0) deleteTile(&world->damage_buffs, 0);
    while(world->interactables.size != 0) deleteTile(&world->interactables, 0);
}

bool isnull(Tile tile) { return (tile.tt == UNDF); }

Rectangle getArea(Vector2 pos, int size) { return (Rectangle){pos.x, pos.y, size, size}; }

Vector2 getSpawnPoint() 
{
	char info[CHAR_LIMIT];
	FILE* file = fopen(SPAWN_PATH, "r");
	
	if(file == NULL) return (Vector2){INT_MIN, INT_MIN};
	else
	{
		fgets(info, sizeof(info), file);
		return (Vector2){atof(strtok(info, ",")), atof(strtok(NULL, "\n"))};
	}
}	

bool inbounds(Vector2 pos, Rectangle area) { return ((pos.x + SCREEN_TILE_SIZE) <= (area.x + area.width)) && (((pos.y + SCREEN_TILE_SIZE) <= (area.y + area.height))); }

void drawTile(Tile tile, int size) { DrawTexturePro(tile.tx, getArea(tile.src, TILE_SIZE), getArea(tile.sp, size), (Vector2){0,0}, 0, WHITE);}

// for debugging
void printTile(Tile t) { printf("%.2f,%.2f,%.2f,%.2f,%d,%s,%d,%d,%d\n", t.src.x, t.src.y, t.sp.x, t.sp.y, t.tt, t.fp, t.animated, t.animtaion.frames, t.animtaion.speed);  }

Vector2 mousePosition(Camera2D camera)
{
    Vector2 pos = GetScreenToWorld2D(GetMousePosition(), camera);
    pos.x = (((int)pos.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
    pos.y = (((int)pos.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
    return pos;
}

Texture2D addTexture(Textures* textures, const char* FP)
{
	// returning non unique texture
	for(int i = 0; i < textures->size; i++) { if(strcmp(textures->better_textures[i].fp, FP) == 0) return textures->better_textures[i].tx; }

	// adding new texture
	textures->better_textures[textures->size] = (BetterTexture){LoadTexture(FP), ""};
	strcpy(textures->better_textures[textures->size].fp, FP);
	textures->size++;

	return textures->better_textures[textures->size - 1].tx;
}

TileList initTileList() { return (TileList){0, TILE_CAP, malloc(TILE_CAP)}; }

Editor initEditor() { return (Editor){NULL_TILE, initTileList(), NULL, (Texture2D){}, UNDF, GetMousePosition(), "", 16.0f, 16.0f, 0.0f, 0.0f}; }

World initWorld() { return (World){initTileList(), initTileList(), initTileList(), initTileList(), initTileList(), initTileList(), (EntityList){}, (Textures){}, (Rectangle){288, 32, 512, 512}, (Vector2){INT_MIN, INT_MIN}}; }


void loadWorld(World* world, char* path)
{
    world->spawn = getSpawnPoint();

    char line[CHAR_LIMIT];
    FILE* inFile = fopen(path, "r");
    
	if(inFile == NULL) return;

    Vector2 src;
    Vector2 sp;

    TileType tt;
    char fp[CHAR_LIMIT];

	bool animated;
	int frames;
	int animation_speed;	

    while(fgets(line, sizeof(line), inFile))
    {
        src.x = atoi(strtok(line, ","));
        src.y = atoi(strtok(NULL, ","));
        sp.x = atoi(strtok(NULL, ","));
        sp.y = atoi(strtok(NULL, ","));

        tt = (TileType)atoi(strtok(NULL, ","));
		
        strcpy(fp, strtok(NULL, ","));

		animated = atoi(strtok(NULL, ","));
		frames = atoi(strtok(NULL, ","));
		animation_speed = atoi(strtok(NULL, "\n"));
        
        Tile tile = (Tile){src, sp, addTexture(&world->textures, fp), tt, "", animated, (Animation){frames, 0, animation_speed, 0, 0}};
        strcpy(tile.fp, fp);

        // adj world size
        if((tile.sp.x + SCREEN_TILE_SIZE) >= (world->area.x + world->area.width)) world->area.width += SCREEN_TILE_SIZE;
        if((tile.sp.y + SCREEN_TILE_SIZE) >= (world->area.y + world->area.height)) world->area.height += SCREEN_TILE_SIZE;

        switch (tt)
		{
			case WALL: addTile(&world->walls, tile); break;
			case FLOOR: addTile(&world->floors, tile); break;
			case DOOR: addTile(&world->doors, tile); break;
			case HEALTH_BUFF: addTile(&world->health_buffs, tile); break;
			case DAMAGE_BUFF: addTile(&world->damage_buffs, tile); break;
			case INTERACTABLE: addTile(&world->interactables, tile); break;
			default:
				break;
		}
    }

    fclose(inFile);
}

void readImage(TileList* avail_tiles, Texture2D texture, char* path)
{
    Vector2 cp = {0,0};

    while((cp.x < texture.width) && (cp.y < texture.height))
    {
        Tile tile = (Tile){cp, (Vector2){0, 0}, texture};
        strcpy(tile.fp, path);
        addTile(avail_tiles, tile);

        // move along image, next row if at the end
        if((cp.x += TILE_SIZE) == texture.width) cp = (Vector2){0, (cp.y + TILE_SIZE)};
    }
}

void loadBtn(GuiWindowFileDialogState* directory, World* world, TileList* avail_tiles, Tile* curr_tile, Texture2D* curr_texture, char* path)
{
    const Rectangle BTN_AREA = {70, 3, 65, 20};
	
    if (GuiButton(BTN_AREA, GuiIconText(ICON_FILE_OPEN, "LOAD"))) directory->windowActive = true;

    if(directory->SelectFilePressed)
    {
        *curr_tile = NULL_TILE;
		strcpy(path,TextFormat("%s" PATH_SEPERATOR "%s", directory->dirPathText,directory->fileNameText));
        
        // loading sprite
        if (IsFileExtension(directory->fileNameText, ".png"))
		{
			*curr_texture = addTexture(&world->textures, path);
            readImage(avail_tiles, *curr_texture, path);
		}

        // loading world
        if (IsFileExtension(directory->fileNameText, ".txt"))
        {
            deleteWorld(world);
            loadWorld(world, path);
        }

        strcpy(path, "");
        directory->SelectFilePressed = false;
    }
}

void deleteBtn(World* world) { if(GuiButton((Rectangle){137, 3, 65, 20}, "DELETE")) deleteWorld(world); }

void focusBtn(Camera2D* camera) { if(GuiButton((Rectangle){ 204, 3, 50, 20 }, "FOCUS")) camera->target = (Vector2){ 0, 0 }; }


void saveLayer(TileList* layer, char* path)
{
    FILE* file = fopen(path, "a");
    if(file == NULL) { printf("error saving layer\n"); return; }

    for(int i = 0; i < layer->size; i++)
    {
        Tile t = layer->list[i];
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,%d,%s,%d,%d,%d\n", t.src.x, t.src.y, t.sp.x, t.sp.y, t.tt, t.fp, t.animated, t.animtaion.frames, t.animtaion.speed); 
    }

    fclose(file);
}

void saveSpawn(Vector2 spawn)
{
    FILE* file = fopen(SPAWN_PATH, "w");
    if(file == NULL) return;
    fprintf(file, "%.2f,%.2f\n", spawn.x, spawn.y);
    fclose(file);
}

void saveBtn(World* world, Tile* curr_tile, bool* showInputTextBox, char* path)
{
    const Rectangle BTN_AREA = {3, 3, 65, 20}; 
    const Rectangle INPUT_WINDOW_AREA = {376, 436, 240, 140};
	
    if (GuiButton(BTN_AREA,GuiIconText(ICON_FILE_SAVE, "SAVE"))) *showInputTextBox = true;

    if(*showInputTextBox)
    {
        *curr_tile = NULL_TILE; 
        int result = GuiTextInputBox(INPUT_WINDOW_AREA,GuiIconText(ICON_FILE_SAVE, "Save file as..."),
						"file name:", "Ok;Cancel", path, 255, NULL);

        // save world to 'x', where 'x' is the inputted filename
        if((result == 1) && (strlen(path) > 0))
        {
            saveSpawn(world->spawn);
            saveLayer(&world->walls, path);
            saveLayer(&world->doors, path);
            saveLayer(&world->floors, path);
            saveLayer(&world->health_buffs, path);
            saveLayer(&world->damage_buffs, path);
            saveLayer(&world->interactables, path);
        }

        // reset flag upon saving and premature exit
		if ((result == 0) || (result == 1) || (result == 2))
		{
			*showInputTextBox = false;
			strcpy(path, "");
		}
    }
}

int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

void trashBtn(TileList* avail_tiles, Tile* curr_tile)
{
    const Rectangle BTN_AREA = {235, 32, 24, 24};

    if (GuiButton(BTN_AREA ,GuiIconText(ICON_BIN, "")))
	{
		*curr_tile = NULL_TILE;
        DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;
        while(avail_tiles->size != 0) deleteTile(avail_tiles, 0);
	}
}

void editBoard(TileList* avail_tiles, Tile* curr_tile)
{
    const char* PANEL_TXT = "TILES";
    
    const int TILES_PER_ROW = 8;
    const int MAX_ROWS = 15;

    const Rectangle PANEL_BORDER = { 3, 32, (TILES_PER_ROW * SCREEN_TILE_SIZE), (MAX_ROWS * SCREEN_TILE_SIZE) };
    const Vector2 LAST_TILE_POS = { (((TILES_PER_ROW - 1) * SCREEN_TILE_SIZE) + PANEL_BORDER.x), (MAX_ROWS * SCREEN_TILE_SIZE) - 8 };

    GuiPanel(PANEL_BORDER, PANEL_TXT);
    trashBtn(avail_tiles, curr_tile); 
    
    Vector2 cp = { PANEL_BORDER.x, (PANEL_BORDER.y + 24) }; // pos of ith tile to be drawn
    
    for(int i = 0; i < avail_tiles->size; i++, (cp.x += DISPLAY_TILE_SIZE))
    {
        // next row transition
        if(cp.x == (PANEL_BORDER.x + PANEL_BORDER.width)) cp = (Vector2){ PANEL_BORDER.x, (cp.y + DISPLAY_TILE_SIZE) };
        // tile overflow
        if(Vector2Equals(cp, LAST_TILE_POS)) DISPLAY_TILE_SIZE /= 2.0f;

        Tile tile = avail_tiles->list[i];
        tile.sp = cp; 

        drawTile(tile, DISPLAY_TILE_SIZE);

        // choosing a tile 
        if (CheckCollisionPointRec(GetMousePosition(),getArea(tile.sp, DISPLAY_TILE_SIZE)))
        {
            DrawRectangleLinesEx(getArea(tile.sp, DISPLAY_TILE_SIZE), 2, BLUE);
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) *curr_tile = tile;
        }
    }
    
    // highlighting current tile
    if(curr_tile->tx.id > 0) DrawRectangleLinesEx(getArea(curr_tile->sp, DISPLAY_TILE_SIZE), 2, GREEN);
}

void chooseLayer(World* world, TileType* curr_type, TileList** curr_layer)
{
	const Rectangle PANEL_BORDER = { 829, 32, 160, 210 };
    const Rectangle TOGGLE_BORDER = { 845, 72, 128, 24 };
    const char* PANEL_TXT = "LAYER TO EDIT";

    GuiPanel(PANEL_BORDER, PANEL_TXT);
    GuiToggleGroup(TOGGLE_BORDER,"WALLS \n FLOORS \n DOORS \n BUFFS (HEALTH) \n BUFFS (DAMAGE) \n INTERACTABLES", curr_type);

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

void chooseSpawnPos(Rectangle world_area, Vector2* spawn, Camera2D camera) { if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetScreenToWorld2D(GetMousePosition(), camera), world_area)) *spawn = GetScreenToWorld2D(GetMousePosition(), camera); }

void drawLayer(TileList* layer, Rectangle world_area, Color color, char* dsc)
{
    for(int i = 0; i < layer->size; i++) 
    {
        if(!isnull(layer->list[i]) && inbounds(layer->list[i].sp, world_area)) 
        {
            drawTile(layer->list[i], SCREEN_TILE_SIZE); 
            DrawText(dsc, layer->list[i].sp.x, layer->list[i].sp.y, 3, color);
        }
    }
}

void drawWorld(World* world, Camera2D camera)
{
    BeginMode2D(camera);
        drawLayer(&world->walls,world->area, RED, "W");
        drawLayer(&world->floors,world->area, ORANGE, "F");
        drawLayer(&world->doors,world->area, YELLOW, "D");
        drawLayer(&world->health_buffs, world->area, GREEN, "HB");
        drawLayer(&world->damage_buffs, world->area, BLUE, "DB");
        drawLayer(&world->interactables, world->area,PURPLE, "I");
        DrawText("S", world->spawn.x, world->spawn.y, 5, PINK);
        DrawRectangleLinesEx(world->area, 3, GRAY);
    EndMode2D();
}

void editLayer(Editor* editor, Rectangle world_area)
{
    // tile deletion
    int delete = -1;
    if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) for(int i = 0; i < editor->curr_layer->size; i++)  if(Vector2Equals(editor->mouse_pos, editor->curr_layer->list[i].sp)) delete = i;
    if(delete >= 0) deleteTile(editor->curr_layer, delete);

    // tile creation
    if(!isnull(editor->curr_tile))
    {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            bool add = true;
            for(int i = 0; (i < editor->curr_layer->size) && (add); i++) if(Vector2Equals(editor->mouse_pos, editor->curr_layer->list[i].sp)) add = false;
            if(add)
            {
                bool animate = ((editor->animation_speed > 0) && (editor->nframes > 0));
                Tile tile = {editor->curr_tile.src, editor->mouse_pos, editor->curr_tile.tx, editor->curr_type, "", 
                        animate, (Animation){editor->nframes, 0, animate ? (FPS / (int)editor->animation_speed) : 0}};
                strcpy(tile.fp, editor->curr_tile.fp);
                addTile(editor->curr_layer, tile);
            }
        }
    }
}

void editWorldSize(float* adjw, float* adjh, float* wwidth, float* wheight)
{
	const Rectangle PANEL_BORDER = { 829 , 280, 160, 90 };
    const char* PANEL_TXT = "WORLD SIZE";

    GuiPanel(PANEL_BORDER, PANEL_TXT);
    
    const Rectangle INC_WIDTH_BORDER = { 910, 310, 20, 20 };
    const Rectangle DEC_WIDTH_BORDER = { 933, 310, 20, 20 };
    const char* WIDTH = "WIDTH";

    const Rectangle INC_HEIGHT_BORDER = { 910, 340, 20, 20 };
    const Rectangle DEC_HEIGHT_BORDER = { 933, 340, 20, 20 };
    const char* HEIGHT = "HEIGHT";
    
    const char* INC_TXT = "+";
    const char* DEC_TXT = "-";

    // adj width and height of world manually
    DrawText(WIDTH, 860, 315, 10, GRAY);
    if(GuiButton(INC_WIDTH_BORDER, INC_TXT)) *wwidth += SCREEN_TILE_SIZE; 
    if(GuiButton(DEC_WIDTH_BORDER, DEC_TXT) && (*wwidth > SCREEN_TILE_SIZE)) *wwidth -= SCREEN_TILE_SIZE;

    DrawText(HEIGHT, 860, 345, 10, GRAY);
    if(GuiButton(INC_HEIGHT_BORDER, INC_TXT)) *wheight += SCREEN_TILE_SIZE; 
    if(GuiButton(DEC_HEIGHT_BORDER, DEC_TXT) && (*wheight > SCREEN_TILE_SIZE)) *wheight -= SCREEN_TILE_SIZE;
}

void animateTiles(float* fps, float* nframes)
{
    const Rectangle PANEL_BORDER = {3, 525, 256, 80};
    const char* PANEL_TXT = "ANIMATION";

    GuiPanel(PANEL_BORDER, PANEL_TXT);

    const Rectangle SPEED_SLIDER_BORDER = { 53, 560, 85, 10 };
    const char* SSB_TXT = "SPEED";

    const Rectangle FRAME_NUMBER_SLIDER = { 90, 585, 85, 10 };
    const char* FNS_TXT = "# OF FRAMES";

    char text[CHAR_LIMIT];

    sprintf(text, "%d FPS", (int)(*fps));
    GuiSliderBar(SPEED_SLIDER_BORDER, SSB_TXT, text, fps, 0, 10);

    sprintf(text, "%d", (int)(*nframes));
    GuiSliderBar(FRAME_NUMBER_SLIDER, FNS_TXT, text, nframes, 0, 10);
}

void moveWorldDisplay(Camera2D* camera, Vector2* mp)
{
    Vector2 delta = GetMouseDelta();
    delta = Vector2Scale(delta, -1.0f / camera->zoom);
    camera->target = Vector2Add(camera->target, delta);
}

void toggleEditorState(EditState* state)
{
    if(IsKeyPressed(KEY_E)) *state = EDIT;
    else if(IsKeyPressed(KEY_S)) *state = SPAWN;
    else if(IsKeyPressed(KEY_F)) *state = FREE;
}

void showCurrentState(EditState state)
{
    char text[CHAR_LIMIT];
    Color color;

    switch (state)
    {
        case EDIT: sprintf(text, "EDIT"); color = GREEN; break;
        case SPAWN: sprintf(text, "SPAWN"); color = RED; break;
        case FREE: sprintf(text, "FREE-VIEW"); color = BLUE; break;
        default:
            break;
    }

    DrawText(text, (SCREEN_WIDTH - MeasureText(text, 20)) - 3, (SCREEN_HEIGHT - 20), 20, color);
}

void init(Camera2D* camera, World* world, Editor* editor, GuiWindowFileDialogState* fds)
{
    SetTargetFPS(FPS);
	// SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");

    camera->zoom = 1.0f;
    *world = initWorld();
    *editor = initEditor();
    *fds = InitGuiWindowFileDialog(GetWorkingDirectory());
}

void deinit(World* world, TileList* avail_tiles, Texture* curr_texture)
{
    // freeing alloc mem from dynamic lists 
    free(avail_tiles->list);
    free(world->walls.list);
	free(world->doors.list);
	free(world->floors.list);
	free(world->health_buffs.list);
	free(world->damage_buffs.list);
	free(world->interactables.list);
    // unloading all textures used
	for(int i = 0; i < world->textures.size; i++) UnloadTexture(world->textures.better_textures[i].tx);
    CloseWindow();
}

int main()
{
    World world;
    Editor editor;
    Camera2D camera;
    EditState state;
    bool showInputTextBox = false;
    GuiWindowFileDialogState fileDialogState;
    init(&camera, &world, &editor, &fileDialogState);
    
    // preventing adding tiles when on other panels
    const Rectangle DEAD_ZONE_1 = {3, 32, 256, 584};
    const Rectangle DEAD_ZONE_2 = {829, 32, 160, 362};

    while(!WindowShouldClose())
    {
        toggleEditorState(&state);
        editor.mouse_pos = mousePosition(camera);
        if(showInputTextBox || fileDialogState.windowActive) state = FREE;
        BeginDrawing();
            
            ClearBackground(RAYWHITE);
            switch (state)
            {
                case FREE: if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) moveWorldDisplay(&camera, &editor.mouse_pos); break;
                case SPAWN: chooseSpawnPos(world.area, &world.spawn, camera); break;
                case EDIT: 
                    if((editor.curr_layer != NULL) && CheckCollisionPointRec(editor.mouse_pos, world.area) &&
                        (!CheckCollisionPointRec(GetMousePosition(), DEAD_ZONE_1) && !CheckCollisionPointRec(GetMousePosition(), DEAD_ZONE_2)))
                    {
                        BeginMode2D(camera);
                            editLayer(&editor, world.area);
                            DrawRectangleLinesEx(getArea(editor.mouse_pos, SCREEN_TILE_SIZE), 2, RED);
                        EndMode2D();
                    }
                    break;
                default:
                    break;
            }

            drawWorld(&world, camera);
            showCurrentState(state);

            // editing mechs
            editBoard(&editor.avail_tiles, &editor.curr_tile);
            chooseLayer(&world, &editor.curr_type, &editor.curr_layer);
            editWorldSize(&editor.width, &editor.height, &world.area.width, &world.area.height);
            animateTiles(&editor.animation_speed, &editor.nframes);

            // buttons            
            deleteBtn(&world);
            focusBtn(&camera);
            saveBtn(&world, &editor.curr_tile, &showInputTextBox, editor.path);
            loadBtn(&fileDialogState, &world, &editor.avail_tiles, &editor.curr_tile, &editor.curr_texture, editor.path);

            GuiWindowFileDialog(&fileDialogState);
        EndDrawing();
    }
    deinit(&world, &editor.avail_tiles, &editor.curr_texture);
    return 0;
}