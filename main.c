#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#undef RAYGUI_IMPLEMENTATION            
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "headers/gui_window_file_dialog.h"

const int SCREEN_WIDTH = 1120;
const int SCREEN_HEIGHT = 992;
const int PANEL_HEIGHT = 24;
const int TILE_SIZE = 16;
const float SCALE = 2.0f;
const int SCREEN_TILE_SIZE = TILE_SIZE * SCALE;
static int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

int mpx, mpy = 0;

int to_delete = -1;

// mouse position relative to the 2d camera object 
Vector2 mp = { 0 };

enum Element{
	WALL = 0,
	FLOOR = 1,
	DOOR = 2,
	HEALTH_BUFF = 3,
	DAMAGE_BUFF = 4,
	INTERACTABLE = 5,
	SPAWN = 6,
};

typedef struct
{
    Vector2 src;
    Vector2 sp;
    Texture2D tx;
    enum Element tt;
    char fp [512];
} Tile;

typedef struct
{
    int size;
    size_t cap;
    Tile* list;
} TileList;

typedef struct 
{
    TileList walls;
    TileList floors;
    TileList doors;
    TileList health_buffs;
    TileList damage_buffs;
    TileList interactables;
    Vector2 spawn;
} World;

void initWorld(World * world)
{
    world->walls.list = (Tile*)malloc(25 * sizeof( Tile));
    world->floors.list = (Tile*)malloc(25 * sizeof( Tile));
    world->doors.list = (Tile*)malloc(25 * sizeof( Tile));
    world->health_buffs.list = (Tile*)malloc(25 * sizeof( Tile));
    world->damage_buffs.list = (Tile*)malloc(25 * sizeof( Tile));
    world->interactables.list = (Tile*)malloc(25 * sizeof( Tile));
    world->spawn= (Vector2){0, 0};
    world->walls.cap = world->doors.cap = world->floors.cap = world->damage_buffs.cap = world->health_buffs.cap = world->interactables.cap = 25 * sizeof(Tile);
}

void resize_tile_list(TileList * layer)
{
    layer->cap *= 2;
    layer->list = (Tile*)realloc(layer->list, layer->cap);

    if(layer->list == NULL)
    {
        printf("RESIZING FAILED \n");
        exit(1);
    }
}

void add_tile(TileList * layer, Tile tile)
{
    if(layer->size * sizeof(Tile) == layer->cap)
    {
        resize_tile_list(layer);
    }

    layer->list[layer->size] = tile;
    layer->size++;

	printf("add old size %d new size %d, %zu\n", layer->size - 1, layer->size, layer->cap);
}

void remove_tile(TileList * layer, int pos)
{
	if (layer->size == 0) {
		printf("out of bounds\n");
		return;
	}

    for(int i = pos; i < layer->size; i++)
    {
        layer->list[i] = layer->list[i + 1];
    }

	layer->size--;
	if (layer->size == 0) {
		printf("clearing tile \n");
		free(layer->list);
		layer->list = malloc(sizeof(Tile));
		layer->size = 0;
		layer->cap = sizeof(Tile);
	} 
	
	else 
	{
		layer->list = (Tile*)realloc(layer->list, sizeof(Tile) * layer->size + 1);
		printf("remove old size %d new size %d, %zu\n", layer->size + 1, layer->size, layer->cap);
	}
}

void clear_layer(TileList * layer)
{
	if(layer->size == 1)

	for(int i = 0; i < layer->size; i++)
	{
		UnloadTexture(layer->list[i].tx);
	}
	
	free(layer->list);
	layer->list = (Tile*)malloc(25 * sizeof(Tile));
	layer->size = 0;
	layer->cap = 25 * sizeof(Tile);
}

void init(World * world, TileList * tile_dict)
{
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(992, 992, "Editor");
    initWorld(world);
    SetTargetFPS(60);
    tile_dict->list = (Tile*)malloc(25 * sizeof(Tile));
    tile_dict->cap = 25 * sizeof(Tile);
}

void deinit(World * world, TileList * tile_dict)
{
	clear_layer(tile_dict);
	clear_layer(&world->walls);
	clear_layer(&world->floors);
	clear_layer(&world->doors);
	clear_layer(&world->health_buffs);
	clear_layer(&world->damage_buffs);
	clear_layer(&world->interactables);
    CloseWindow();
}

// breaks the user-selected png into tiles that user can choose from to append to the world
void readPNG(Texture2D texture, TileList * tile_dict, char fp [512])
{
	Vector2 cp = {0, 0};
	// surface area of the image with respect to tilesize
	while(cp.x < texture.width && cp.y < texture.height)
	{
        add_tile(tile_dict, (Tile){cp, (Vector2){ 0 }, texture, FLOOR, *fp});
        
        if(cp.x + TILE_SIZE != texture.width)
		{
			cp.x += TILE_SIZE;
		}

		// move to next row	
		else
		{
			cp.x = 0;
			cp.y += TILE_SIZE;
		}
	}
}

void select_tile(Rectangle side_panel, TileList * tile_dict, Tile *current_tile)
{
	GuiPanel(side_panel, "TILES");

	// the last tile before overflowing the side panel, TODO FIX HARDCODED Y VALUE
	Vector2 lt = {side_panel.x + side_panel.width - DISPLAY_TILE_SIZE,888};
	Vector2 cp = {side_panel.x, side_panel.y + PANEL_HEIGHT};

	for(int i = 0; i < tile_dict->size; i++)
	{
		tile_dict->list[i].sp = cp;

		// shrink to fit
		if(Vector2Equals(tile_dict->list[i].sp, lt))
		{
			DISPLAY_TILE_SIZE /= 2;
			*current_tile = (Tile){ 0 };
		}
		
		DrawTexturePro(tile_dict->list[i].tx, (Rectangle){tile_dict->list[i].src.x, tile_dict->list[i].src.y, TILE_SIZE,TILE_SIZE}, (Rectangle){cp.x, cp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
		cp.x += DISPLAY_TILE_SIZE;
		
		// moving to new row
		if((i + 1) % (int)(side_panel.width / DISPLAY_TILE_SIZE) == 0 && i != 0)
		{
			cp.x = side_panel.x;
			cp.y += DISPLAY_TILE_SIZE;
		}

		if(CheckCollisionPointRec(GetMousePosition(), (Rectangle){tile_dict->list[i].sp.x, tile_dict->list[i].sp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE}))
		{
			// higlight tile that user is hovering over
			DrawRectangleLines(tile_dict->list[i].sp.x, tile_dict->list[i].sp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, BLUE);
			// update current tile
			if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				*current_tile = tile_dict->list[i];
			}
		}
	}

	// reset tiles to choose from
	if(IsKeyPressed(KEY_F))
	{
		clear_layer(tile_dict);
		*current_tile = (Tile){ 0 };
	}

	// highlight the current tile a user has selected
	if(current_tile->tx.id != 0)
	{
		DrawRectangleLines(current_tile->sp.x, current_tile->sp.y, DISPLAY_TILE_SIZE,DISPLAY_TILE_SIZE, GREEN);
	}
}

void select_layer(Rectangle layer_panel, int* current_layer)
{
	GuiPanel(layer_panel, "LAYER TO EDIT");

	// toggling between layers
	GuiToggleGroup((Rectangle){ GetScreenWidth() - SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE + PANEL_HEIGHT + TILE_SIZE, SCREEN_TILE_SIZE * 4.0f, 24}, 
					 "WALLS \n FLOORS \n DOORS \n BUFFS (HEALTH) \n BUFFS (DAMAGE) \n INTERACTABLES \n SPAWN POINT", current_layer);
}

// to edit a specific layer by tile creation and/or deletion
void editLayer(TileList * layer, Tile * current_tile, Rectangle * worldArea, enum Element current_tile_type)
{
	// creation
	if(CheckCollisionPointRec(GetMousePosition(), *worldArea))
	{
		// draw where tile will be
		DrawRectangleLines(mpx, mpy, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, BLUE);
			
		if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && current_tile->tx.id != 0)
		{
			bool to_add = true;

			// prevent adding duplicate tiles of the same position
			for(int i = 0; i < layer->size; i++)
			{
				if(Vector2Equals(layer->list[i].sp, (Vector2){mpx, mpy}))
				{
					to_add = false;
				}
			}

			if(to_add)
			{
				// create new tile
				add_tile(layer, (Tile){current_tile->src, (Vector2){mpx, mpy}, current_tile->tx, current_tile_type,*current_tile->fp});
			}
		}
	}

	// deletion
	for(int i = 0; i < layer->size; i++)
	{
		if(layer->list[i].tx.id != 0)
		{
			if(CheckCollisionPointRec(GetMousePosition(), (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
			{
				to_delete = i;
				// remove_tile(layer, i);
			}
		}
	}

	if (to_delete != -1) {

		remove_tile(layer, to_delete);
		printf("index to delete %d\n", to_delete);
		to_delete = -1;
	}
}

void editWorld(World * world, Rectangle worldArea, Tile * current_tile, enum Element current_tile_type)
{
	// edit corresp layer based on layer selection  
	switch (current_tile_type)
	{
		case WALL: 
			editLayer(&world->walls, current_tile, &worldArea, current_tile_type); break;
		case FLOOR: 
			editLayer(&world->floors, current_tile, &worldArea, current_tile_type); break;
		case DOOR:
			editLayer(&world->doors, current_tile, &worldArea,current_tile_type); break;
		case HEALTH_BUFF:
			editLayer(&world->health_buffs, current_tile, &worldArea, current_tile_type); break;
		case DAMAGE_BUFF:
			editLayer(&world->damage_buffs, current_tile, &worldArea, current_tile_type); break;
		case INTERACTABLE: 
			editLayer(&world->interactables, current_tile, &worldArea, current_tile_type); break;
		case SPAWN:
			if(CheckCollisionPointRec(GetMousePosition(), worldArea))
			{
				if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
				{
					world->spawn = (Vector2){mpx, mpy};
				}
			}
			break;
		default:
			break;
	}
}

void drawLayer(TileList * layer, Color color, Rectangle worldArea, const char * dsc, bool showDsc)
{
	for(int i = 0; i < layer->size; i++)
	{
		// only draw tile if in bounds
		if(layer->list[i].fp[0] != '\0' && layer->list[i].sp.x < worldArea.x + worldArea.width && layer->list[i].sp.y < worldArea.y + worldArea.height)
		{
			DrawTexturePro(layer->list[i].tx, (Rectangle){layer->list[i].src.x, layer->list[i].src.y, TILE_SIZE, TILE_SIZE}, (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
			
			// char desc the type of tile it is
			if(showDsc)
			{
				DrawText(dsc, layer->list[i].sp.x + SCREEN_TILE_SIZE - MeasureText(dsc, 5), layer->list[i].sp.y + TILE_SIZE, 5, color);
			}
		}
	}
}

int main()
{
    World world;
    TileList tile_dict;
	Tile currentTile;
    init(&world, &tile_dict);
    GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
	Rectangle side_panel = {TILE_SIZE, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE * 8.0f, SCREEN_TILE_SIZE * 28.0f};
	Rectangle layer_panel = {GetScreenWidth() - TILE_SIZE - SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE * 7.0f};
	Rectangle worldArea = {side_panel.x + side_panel.width + TILE_SIZE, SCREEN_TILE_SIZE, 512,512};
    char user_file_path[512] = { 0 };
	int current_layer;
    Texture current_texture = { 0 };

    while (!WindowShouldClose()) 
    {
		// snap mouse position to nearest tile by making it divisble by the screen tile size
		mpx = (((int)GetMousePosition().x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
   	 	mpy = (((int)GetMousePosition().y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));

        if (fileDialogState.SelectFilePressed)
        {
            if (IsFileExtension(fileDialogState.fileNameText, ".png"))
            {
                strcpy(user_file_path, TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
                // UnloadTexture(current_texture);
                current_texture = LoadTexture(user_file_path);
                readPNG(current_texture, &tile_dict, user_file_path);

            }

            fileDialogState.SelectFilePressed = false;
        }

        BeginDrawing();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

			DrawRectangleLines(worldArea.x, worldArea.y, worldArea.width, worldArea.height, GRAY);
			editWorld(&world, worldArea, &currentTile, (enum Element)current_layer);

			// drawing each layer
			bool showDsc = (current_layer != 6);

			drawLayer(&world.floors, RED, worldArea, "F", showDsc);
			drawLayer(&world.health_buffs, ORANGE, worldArea, "HB", showDsc);
			drawLayer(&world.damage_buffs, YELLOW, worldArea, "DB", showDsc);
			drawLayer(&world.doors, GREEN, worldArea, "D", showDsc);
			drawLayer(&world.interactables,BLUE, worldArea, "I", showDsc);
			drawLayer(&world.walls, PURPLE, worldArea, "W", showDsc);

			DrawText("S", world.spawn.x + SCREEN_TILE_SIZE, world.spawn.y + TILE_SIZE, 5, PINK);
			// choosing the tile to edit the world with
			select_tile(side_panel, &tile_dict, &currentTile);
			// choosing which layer to  add to 
			select_layer(layer_panel, &current_layer);

            if (fileDialogState.windowActive) GuiLock();

			// selecting png or text file to load textures or worlds to edit 
            if (GuiButton((Rectangle){SCREEN_TILE_SIZE  * 4, 8, SCREEN_TILE_SIZE * 5, TILE_SIZE}, GuiIconText(ICON_FILE_OPEN, "LOAD WORLD/TEXTURE"))) fileDialogState.windowActive = true;

            GuiUnlock();

            GuiWindowFileDialog(&fileDialogState);

        EndDrawing();
    }

    UnloadTexture(current_texture);     
    deinit(&world, &tile_dict);
    return 0;
}
