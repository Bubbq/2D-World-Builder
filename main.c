#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#undef RAYGUI_IMPLEMENTATION
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;
const int PANEL_HEIGHT = 24;
const int TILE_SIZE = 16;
const float SCALE = 2.0f;
const int SCREEN_TILE_SIZE = TILE_SIZE * SCALE;
int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

int mpx, mpy = 0;

int to_delete = -1;

// mouse position relative to the 2d camera object
Vector2 mp = {0};

enum Element
{
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
	char fp[512];
} Tile;

typedef struct
{
	int size;
	size_t cap;
	Tile *list;
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

void resizeLayer(TileList *layer)
{
	layer->cap *= 2;
	layer->list = realloc(layer->list, layer->cap);

	if (layer->list == NULL)
	{
		printf("RESIZING FAILED \n");
		exit(1);
	}
}

void eraseLayer(TileList *layer)
{
	for (int i = 0; i < layer->size; i++) {
		UnloadTexture(layer->list[i].tx);
	}

	free(layer->list);
	layer->list = malloc(25 * sizeof(Tile));
	layer->size = 0;
	layer->cap = 25 * sizeof(Tile);
}

void addTile(TileList *layer, Tile tile)
{
	if (layer->size * sizeof(Tile) == layer->cap)
	{
		resizeLayer(layer);
	}

	layer->list[layer->size++] = tile;
}

void removeTile(TileList *layer, int pos)
{
	if (layer->size == 0)
	{
		return;
	}

	for (int i = pos; i < layer->size; i++)
	{
		layer->list[i] = layer->list[i + 1];
	}

	layer->size--;

	if(layer->size == 0)
	{
		eraseLayer(layer);
	}
}

void init(World *world, TileList *tile_dict)
{
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "Editor");
	SetTargetFPS(60);
	
	tile_dict->list = malloc(25 * sizeof(Tile));
	tile_dict->cap = 25 * sizeof(Tile);
	
	world->spawn = (Vector2){0, 0};
	world->walls.list = malloc(25 * sizeof(Tile));
	world->floors.list = malloc(25 * sizeof(Tile));
	world->doors.list = malloc(25 * sizeof(Tile));
	world->health_buffs.list = malloc(25 * sizeof(Tile));
	world->damage_buffs.list = malloc(25 * sizeof(Tile));
	world->interactables.list = malloc(25 * sizeof(Tile));
	world->walls.cap = world->doors.cap = world->floors.cap = world->damage_buffs.cap = world->health_buffs.cap = world->interactables.cap = (25 * sizeof(Tile));
}

void deinit(World *world, TileList *tile_dict)
{
	eraseLayer(tile_dict);
	eraseLayer(&world->walls);
	eraseLayer(&world->floors);
	eraseLayer(&world->doors);
	eraseLayer(&world->health_buffs);
	eraseLayer(&world->damage_buffs);
	eraseLayer(&world->interactables);
	CloseWindow();
}

// breaks the user-selected png into tiles that user can choose from to append to the world
void readPNG(Texture2D texture, TileList *tile_dict, char *fp)
{
	Vector2 cp = {0, 0};
	// surface area of the image with respect to tilesize
	while (cp.x < texture.width && cp.y < texture.height)
	{	
		Tile tile = {cp, (Vector2){0}, texture, FLOOR ,"NULL"};
        	strcpy(tile.fp, fp);
		addTile(tile_dict, tile);

		if (cp.x + TILE_SIZE != texture.width)
		{
			cp.x += TILE_SIZE;
		}
		
		// move to next row of png
		else
		{ 
			cp.x = 0;
			cp.y += TILE_SIZE;
		}
	}
}

void selectTile(Rectangle side_panel, TileList *tile_dict, Tile *current_tile)
{
	GuiPanel(side_panel, "TILES");

	// button to remove tiles to choose from
	if (GuiButton((Rectangle){side_panel.x + side_panel.width - PANEL_HEIGHT,side_panel.y, PANEL_HEIGHT, PANEL_HEIGHT},GuiIconText(ICON_BIN, "")))
	{
		eraseLayer(tile_dict);
		*current_tile = (Tile){ 0 };
		DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;
	}

	// position of the last tile before overflowing, resize display tile size if any tile reaches here to prevent overfilling in left side panel
	Vector2 lt = {side_panel.x + side_panel.width - DISPLAY_TILE_SIZE, 888};
	Vector2 cp = {side_panel.x, side_panel.y + PANEL_HEIGHT};

	for (int i = 0; i < tile_dict->size; i++)
	{
		tile_dict->list[i].sp = cp;

		// shrink to fit
		if (Vector2Equals(tile_dict->list[i].sp, lt))
		{
			DISPLAY_TILE_SIZE /= 2;
			*current_tile = (Tile){0};
		}

		DrawTexturePro(tile_dict->list[i].tx,(Rectangle){tile_dict->list[i].src.x, tile_dict->list[i].src.y,TILE_SIZE, TILE_SIZE},(Rectangle){cp.x, cp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE},(Vector2){0, 0}, 0, WHITE);
		
		cp.x += DISPLAY_TILE_SIZE;

		// moving to new row
		if ((i + 1) % (int)(side_panel.width / DISPLAY_TILE_SIZE) == 0 && i != 0)
		{
			cp.x = side_panel.x;
			cp.y += DISPLAY_TILE_SIZE;
		}

		if (CheckCollisionPointRec(GetMousePosition(),(Rectangle){tile_dict->list[i].sp.x, tile_dict->list[i].sp.y,DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE}))
		{
			// higlight tile that user is hovering over
			DrawRectangleLines(tile_dict->list[i].sp.x, tile_dict->list[i].sp.y,DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, BLUE);
			// update current tile
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				Texture2D tx = LoadTexture(tile_dict->list[i].fp);
				*current_tile = (Tile){tile_dict->list[i].src, tile_dict->list[i].sp, tx, tile_dict->list[i].tt, "NULL"};
				strcpy(current_tile->fp, tile_dict->list[i].fp);
			}
		}
	}

	// highlight the current tile a user has selected
	if (current_tile->tx.id != 0)
	{
		DrawRectangleLines(current_tile->sp.x, current_tile->sp.y,DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, GREEN);
	}
}

// to edit a specific layer by tile creation and/or deletion
void editLayer(TileList *layer, Tile *current_tile, Rectangle *world_area,enum Element current_tile_type)
{
	// creation
	if (CheckCollisionPointRec(mp, *world_area))
	{
		// draw where tile will be
		DrawRectangleLines(mpx, mpy, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, BLUE);

		if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && current_tile->tx.id != 0)
		{
			bool to_add = true;

			// prevent adding duplicates
			for (int i = 0; i < layer->size; i++)
			{
				if (Vector2Equals(layer->list[i].sp, (Vector2){mpx, mpy}))
				{
					to_add = false;
				}
			}

			if (to_add)
			{
				Tile tile = (Tile){current_tile->src, (Vector2){mpx, mpy}, current_tile->tx, current_tile_type, "NULL"};
				strcpy(tile.fp, current_tile->fp);
				addTile(layer, tile);
			}
		}
	}

	// deletion
	int to_delete = -1;

	for (int i = 0; i < layer->size; i++)
	{
		if (layer->list[i].tx.id != 0)
		{
			if (CheckCollisionPointRec(mp,(Rectangle){layer->list[i].sp.x, layer->list[i].sp.y,SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				to_delete = i;
			}
		}
	}

	if (to_delete != -1)
	{
		removeTile(layer, to_delete);
		to_delete = -1;
	}
}

void editWorld(World *world, Rectangle world_area, Tile *current_tile,enum Element current_tile_type)
{
	// edit corresp layer based on layer selection
	switch (current_tile_type)
	{
		case WALL:
			editLayer(&world->walls, current_tile, &world_area, current_tile_type);
			break;
		case FLOOR:
			editLayer(&world->floors, current_tile, &world_area, current_tile_type);
			break;
		case DOOR:
			editLayer(&world->doors, current_tile, &world_area, current_tile_type);
			break;
		case HEALTH_BUFF:
			editLayer(&world->health_buffs, current_tile, &world_area,current_tile_type);
			break;
		case DAMAGE_BUFF:
			editLayer(&world->damage_buffs, current_tile, &world_area,current_tile_type);
			break;
		case INTERACTABLE:
			editLayer(&world->interactables, current_tile, &world_area,current_tile_type);
			break;
		// updating spawn point
		case SPAWN:
			if (CheckCollisionPointRec(mp, world_area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				world->spawn = (Vector2){mpx, mpy};
			}
			break;
	}
}

void drawLayer(TileList *layer, Color color, Rectangle world_area,const char *dsc)
{
	for (int i = 0; i < layer->size; i++)
	{
		// only draw tile if in bounds
		if (layer->list[i].tx.id != 0 && layer->list[i].sp.x < world_area.x + world_area.width &&layer->list[i].sp.y < world_area.y + world_area.height)
		{
			DrawTexturePro(layer->list[i].tx,(Rectangle){layer->list[i].src.x, layer->list[i].src.y,TILE_SIZE, TILE_SIZE},(Rectangle){layer->list[i].sp.x, layer->list[i].sp.y,SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},(Vector2){0, 0}, 0, WHITE);
			// character describing what type of tile it is
			DrawText(dsc,layer->list[i].sp.x + SCREEN_TILE_SIZE - MeasureText(dsc, 5),layer->list[i].sp.y + TILE_SIZE, 5, color);
		}
	}
}

void saveLayer(TileList *layer, char *filePath)
{
	FILE *outFile;
	outFile = fopen(filePath, "a"); 
	if (outFile == NULL)
	{
		printf("ERROR SAVING LAYER\n");
		return;
	}

	for (int i = 0; i < layer->size; i++)
	{
		if (layer->list[i].tx.id != 0)
		{
			fprintf(outFile, "%d,%d,%d,%d,%d,%s,\n", (int)layer->list[i].src.x,(int)layer->list[i].src.y, (int)layer->list[i].sp.x,(int)layer->list[i].sp.y, layer->list[i].tt, layer->list[i].fp);
		}
	}

	fclose(outFile);
}

void loadLayers(World *world, Rectangle *world_area, char *filePath, TileList* tile_dict)
{
	FILE *inFile = fopen(filePath, "r");
	char line[512];
	if (inFile == NULL)
	{
		printf("ERROR LOADING LAYERS \n");
		return;
	}

	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	enum Element tt;
	char* fp ; 
	char lfp[512];

	while (fgets(line, sizeof(line), inFile))
	{
		src.x = atoi(strtok(line, ","));
		src.y = atoi(strtok(NULL, ","));
		sp.x = atoi(strtok(NULL, ","));
		sp.y = atoi(strtok(NULL, ","));
		tt = (enum Element)atoi(strtok(NULL, ","));
		fp = strtok(NULL, ",");
		tx = LoadTexture(fp);
		
		if(strcmp(lfp, fp) != 0)
		{
        	tx = LoadTexture(fp);
			strcpy(lfp, fp);
		}

        Tile tile = (Tile){src, sp, tx, tt, "NULL"};
        strcpy(tile.fp, fp);

		// adj world size if loading world larger than init size
		if (tile.sp.x + SCREEN_TILE_SIZE > world_area->x + world_area->width)
		{
			world_area->width += SCREEN_TILE_SIZE;
		}

		if (tile.sp.y + SCREEN_TILE_SIZE > world_area->y + world_area->height)
		{
			world_area->height += SCREEN_TILE_SIZE;
		}

		switch (tt)
		{
			case WALL:
				addTile(&world->walls, tile);
				break;
			case FLOOR:
				addTile(&world->floors, tile);
				break;
			case DOOR:
				addTile(&world->doors, tile);
				break;
			case HEALTH_BUFF:
				addTile(&world->health_buffs, tile);
				break;
			case DAMAGE_BUFF:
				addTile(&world->damage_buffs, tile);
				break;
			case INTERACTABLE:
				addTile(&world->interactables, tile);
				break;
			default:
				break;
		}
	}

	fclose(inFile);

	// loading spawn point
	FILE * file = fopen("spawn.txt", "r");
	if(file == NULL)
	{
		printf("NO SPAWN POINT SAVED \n");
		return;
	}

	char buff[512];
	fgets(buff, sizeof(buff), file);
	world->spawn.x = atoi(strtok(buff, " "));
	world->spawn.y = atoi(strtok(NULL, "\n"));
	fclose(file);
}

void changeWorldSize(Rectangle *world_area, Rectangle edit_map_size_panel)
{
	GuiPanel(edit_map_size_panel, "MAP SIZE");
	DrawText("Width", edit_map_size_panel.x + ((edit_map_size_panel.width / 2) - MeasureText("Width", 15) / 2.0), edit_map_size_panel.y + PANEL_HEIGHT + TILE_SIZE, 15, GRAY);
	if (GuiButton((Rectangle){edit_map_size_panel.x + SCREEN_TILE_SIZE, edit_map_size_panel.y + PANEL_HEIGHT + SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, "+"))
	{
		world_area->width += SCREEN_TILE_SIZE;
	}
	
	if (GuiButton((Rectangle){edit_map_size_panel.x + SCREEN_TILE_SIZE * 3, edit_map_size_panel.y + PANEL_HEIGHT + SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, "-"))
	{
		if (world_area->width > SCREEN_TILE_SIZE)
		{
			world_area->width -= SCREEN_TILE_SIZE;
		}
	}

	DrawText("Height", edit_map_size_panel.x + ((edit_map_size_panel.width / 2) - MeasureText("Height", 15) / 2.0), edit_map_size_panel.y + PANEL_HEIGHT + TILE_SIZE + SCREEN_TILE_SIZE * 2, 15, GRAY);
	if (GuiButton((Rectangle){edit_map_size_panel.x + SCREEN_TILE_SIZE, edit_map_size_panel.y + PANEL_HEIGHT + (SCREEN_TILE_SIZE * 3), SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, "+"))
	{
		world_area->height += SCREEN_TILE_SIZE;
	}

	if (GuiButton((Rectangle){edit_map_size_panel.x + SCREEN_TILE_SIZE * 3, edit_map_size_panel.y + PANEL_HEIGHT + (SCREEN_TILE_SIZE * 3), SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, "-"))
	{
		if (world_area->height > SCREEN_TILE_SIZE)
		{
			world_area->height -= SCREEN_TILE_SIZE;
		}
	}
}

int main()
{
	World world;
	TileList tile_dict;
	Tile current_tile;
	init(&world, &tile_dict);
	GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
	Rectangle side_panel = {TILE_SIZE, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE * 8.0f,SCREEN_TILE_SIZE * 28.0f};
	Rectangle layer_panel = {GetScreenWidth() - TILE_SIZE - SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE,SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE * 7.5f};
	Rectangle edit_map_size_panel = {GetScreenWidth() - TILE_SIZE - SCREEN_TILE_SIZE * 5.0f,layer_panel.y + layer_panel.height + SCREEN_TILE_SIZE,SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE * 5.0f};
	Rectangle world_area = {side_panel.x + side_panel.width + TILE_SIZE,SCREEN_TILE_SIZE, 512, 512};
	char user_file_path[512] = {0};
	char saved_file_path[512] = {0};
	bool showInputTextBox = false;
	bool edit = false;
	int current_layer;
	Texture current_texture = {0};
	Camera2D camera = {0};
	camera.zoom = 1.0f;
	// Tile currTile = {0};

	while (!WindowShouldClose())
	{
		mp = GetScreenToWorld2D(GetMousePosition(), camera);
		camera.offset = GetMousePosition();
		camera.target = mp;
		
		// snap mouse position to nearest tile by making it divisble by the screen tile size
		mpx = (((int)mp.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
		mpy = (((int)mp.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));

		if(IsKeyPressed(KEY_E))
		{
			edit = !edit;
		}
		
		// moving world around
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !edit)
        {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f/camera.zoom);

            camera.target = Vector2Add(camera.target, delta);
        }

		// file dialog window that mirrors the files explorer in windos, mac, linux,etc.
		if (fileDialogState.SelectFilePressed)
		{
			if (IsFileExtension(fileDialogState.fileNameText, ".png"))
			{
				strcpy(user_file_path,TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState.dirPathText,fileDialogState.fileNameText));
				current_texture = LoadTexture(user_file_path);
				readPNG(current_texture, &tile_dict, user_file_path);
			}

			if (IsFileExtension(fileDialogState.fileNameText, ".txt"))
			{
				eraseLayer(&world.walls);
				eraseLayer(&world.floors);
				eraseLayer(&world.doors);
				eraseLayer(&world.health_buffs);
				eraseLayer(&world.damage_buffs);
				eraseLayer(&world.interactables);

				current_tile = (Tile){0};
				loadLayers(&world, &world_area, fileDialogState.fileNameText, &tile_dict);
			}

			fileDialogState.SelectFilePressed = false;
		}

		BeginDrawing();

		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		BeginMode2D(camera);

			DrawRectangleLines(world_area.x, world_area.y, world_area.width,world_area.height, GRAY);

			if(edit)
			{
				editWorld(&world, world_area, &current_tile, (enum Element)current_layer);
			}

			// drawing each layer
			drawLayer(&world.floors, RED, world_area, "F");
			drawLayer(&world.health_buffs, ORANGE, world_area, "HB");
			drawLayer(&world.damage_buffs, YELLOW, world_area, "DB");
			drawLayer(&world.doors, GREEN, world_area, "D");
			drawLayer(&world.interactables, BLUE, world_area, "I");
			drawLayer(&world.walls, PURPLE, world_area, "W");

			DrawText("S", world.spawn.x + SCREEN_TILE_SIZE, world.spawn.y + TILE_SIZE,5, PINK);

		EndMode2D();

		// deleting world
		if(IsKeyPressed(KEY_A))
		{
			eraseLayer(&world.walls);
			eraseLayer(&world.floors);
			eraseLayer(&world.doors);
			eraseLayer(&world.health_buffs);
			eraseLayer(&world.damage_buffs);
			eraseLayer(&world.interactables);
			current_tile = (Tile){ 0 };
		}

		if(edit)
		{
			DrawText("EDITING", 900 - (MeasureText("EDITING", 30) / 2), 950, 30, RED);
		}

		else 
		{	
			DrawText("FREE VIEW", 900- (MeasureText("FREE VIEW", 30) / 2), 950, 30, BLUE);
		}

		// choosing the tile to edit the world with
		selectTile(side_panel, &tile_dict, &current_tile);
		// choosing which layer to  add to
		GuiPanel(layer_panel, "LAYER TO EDIT");
		GuiToggleGroup((Rectangle){GetScreenWidth() - SCREEN_TILE_SIZE * 5.0f,SCREEN_TILE_SIZE + PANEL_HEIGHT + TILE_SIZE,SCREEN_TILE_SIZE * 4.0f, 24},"WALLS \n FLOORS \n DOORS \n BUFFS (HEALTH) \n BUFFS (DAMAGE) ""\n INTERACTABLES \n SPAWN POINT",&current_layer);
		// changing the size of the world
		changeWorldSize(&world_area, edit_map_size_panel);

		if (fileDialogState.windowActive) GuiLock();

		// selecting png or text file to load textures or worlds to edit
		if (GuiButton((Rectangle){SCREEN_TILE_SIZE * 4, 8, SCREEN_TILE_SIZE * 5,TILE_SIZE},GuiIconText(ICON_FILE_OPEN, "LOAD WORLD/TEXTURE"))) fileDialogState.windowActive = true;
		if (GuiButton((Rectangle){TILE_SIZE, 8, SCREEN_TILE_SIZE * 3, TILE_SIZE},GuiIconText(ICON_FILE_SAVE, "SAVE WORLD"))) showInputTextBox = true;
		
		// resetting world to original position
		if (GuiButton((Rectangle){SCREEN_TILE_SIZE * 9.5, 8, SCREEN_TILE_SIZE * 2, TILE_SIZE},GuiIconText(ICON_FILE_SAVE, "FOCUS")))
		{
			camera.offset = mp;
		}

		GuiUnlock();

		GuiWindowFileDialog(&fileDialogState);

		// prompt text for user to save world
		if (showInputTextBox)
		{
			current_tile = (Tile){0};
			DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),Fade(RAYWHITE, 0.8f));
			int result = GuiTextInputBox((Rectangle){(float)GetScreenWidth() / 2 - 120,(float)GetScreenHeight() / 2 - 60, 240, 140},GuiIconText(ICON_FILE_SAVE, "Save file as..."),"Introduce output file name:", "Ok;Cancel", saved_file_path, 255, NULL);

			// save world to chosen filename
			if (result == 1)
			{
				if (saved_file_path[0] != '\0')
				{
					strcat(saved_file_path, ".txt");

					saveLayer(&world.walls, saved_file_path);
					saveLayer(&world.floors, saved_file_path);
					saveLayer(&world.doors, saved_file_path);
					saveLayer(&world.health_buffs, saved_file_path);
					saveLayer(&world.damage_buffs, saved_file_path);
					saveLayer(&world.interactables, saved_file_path);

					// saving spawnpoint
					FILE * outFile = fopen("spawn.txt", "w");
					if(outFile == NULL)
					{
						printf("ERROR SAVING SPAWN \n");
					}

					fprintf(outFile, "%d %d\n", (int)world.spawn.x, (int)world.spawn.y);
					fclose(outFile);
				}
			}

			// reset flag upon saving and premature exit
			if ((result == 0) || (result == 1) || (result == 2))
			{
				showInputTextBox = false;
				strcpy(saved_file_path, "");
			}
		}

		EndDrawing();
	}

	deinit(&world, &tile_dict);
	return 0;
}
