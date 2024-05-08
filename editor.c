#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"
#include "2d-tile-editor.h"

const int FPS = 60;
const int PANEL_HEIGHT = 24;
const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;
int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

// pos of tile to add or delete
int mpx, mpy = 0;

// mouse position relative to the 2d camera object
Vector2 mp = {0,0};

void init(World *world, TileList *tile_dictionary)
{
	// SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "Editor");
	SetTargetFPS(FPS);
	
	tile_dictionary->size = 0;
	tile_dictionary->cap = TILE_CAP;
	tile_dictionary->list = malloc(TILE_CAP);
	
	world->textures.size = 0;
	
	world->spawn = (Vector2){0, 0};
	world->area = (Rectangle){288,32, 512, 512};
	
	world->walls.size = 0;
	world->doors.size = 0;
	world->floors.size = 0;
	world->health_buffs.size = 0;
	world->damage_buffs.size = 0;
	world->interactables.size = 0;

    world->walls.cap = TILE_CAP;
    world->doors.cap = TILE_CAP;
    world->floors.cap = TILE_CAP;
    world->health_buffs.cap = TILE_CAP;
    world->damage_buffs.cap = TILE_CAP;
	world->interactables.cap = TILE_CAP;

	world->walls.list = malloc(TILE_CAP);
	world->doors.list = malloc(TILE_CAP);
	world->floors.list = malloc(TILE_CAP);
	world->health_buffs.list = malloc(TILE_CAP);
	world->damage_buffs.list = malloc(TILE_CAP);
	world->interactables.list = malloc(TILE_CAP);
}

void deinit(World *world, TileList *tile_dictionary)
{
	free(tile_dictionary->list);
	free(world->walls.list);
	free(world->floors.list);
	free(world->doors.list);
	free(world->health_buffs.list);
	free(world->damage_buffs.list);
	free(world->interactables.list);

	for(int i = 0; i < world->textures.size; i++)
	{
		UnloadTexture(world->textures.better_textures[i].tx);
	}

	CloseWindow();
}

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

void resetLayer(TileList *layer)
{
	free(layer->list);
	layer->size = 0;
	layer->cap = TILE_CAP;
	layer->list = malloc(TILE_CAP);
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
	if(layer->size == 0)
	{
		return;
	}

	for(int i = pos; i < layer->size; i++)
	{
		layer->list[i] = layer->list[i + 1];
	}

	layer->size--;

	if(layer->size == 0)
	{
		resetLayer(layer);
	}
}

Texture2D addTexture(Textures* textures, const char* filePath)
{
	int ctxi = -1;
	bool unique = true;

	for(int i = 0; i < textures->size; i++)
	{
		// if we alr have that texture, load it from the ith position
		if(strcmp(textures->better_textures[i].fp, filePath) == 0)
		{
			unique = false;
			ctxi = i;
			break;
		}
	}

	if(unique)
	{
		textures->better_textures[textures->size] = (BetterTexture){LoadTexture(filePath), "NULL"};
		strcpy(textures->better_textures[textures->size].fp, filePath);
		ctxi = textures->size;
		textures->size++;
	}

	// return the texture of new or recurring element
	return textures->better_textures[ctxi].tx;
}

// breaks the user-selected png into tiles that user can choose from to append to the world
void readImage(Texture2D texture, TileList *tile_dictionary, char *fp)
{
	// tile's current position
	Vector2 cp = {0, 0};

	while (cp.x < texture.width && cp.y < texture.height)
	{	
		Tile tile = (Tile){cp, (Vector2){0, 0}, texture, -1 ,""};
        strcpy(tile.fp, fp);
		addTile(tile_dictionary, tile);

		// move to next row of png
		if(cp.x + TILE_SIZE == texture.width)
		{ 
			cp.x = 0;
			cp.y += TILE_SIZE;
		}

		else
		{
			cp.x += TILE_SIZE;
		}
	}
}

void selectTile(Textures* textures, TileList *tile_dictionary, Tile *current_tile)
{
	// border of display panel
	Rectangle display_panel = {16, 32, 256, 896};
	GuiPanel(display_panel, "TILES");

	// clears tiles to choose from
	if (GuiButton((Rectangle){248, 32, 24, 24},GuiIconText(ICON_BIN, "")))
	{
		resetLayer(tile_dictionary);
		*current_tile = (Tile){ 0 };
		DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;
	}

	// last tile's position, once we reach here, need to resize tiles to fit display panel
	Vector2 lt = {display_panel.x + display_panel.width - DISPLAY_TILE_SIZE, 888};
	Vector2 cp = {display_panel.x, display_panel.y + PANEL_HEIGHT};

	for (int i = 0; i < tile_dictionary->size; i++)
	{
		Tile* tile = &tile_dictionary->list[i];
		tile->sp = cp;

		// shrinking to fit
		if (Vector2Equals(tile->sp, lt))
		{
			DISPLAY_TILE_SIZE /= 1.5;
			*current_tile = (Tile){0};
		}

		// drawing tile
		DrawTexturePro(tile->tx,(Rectangle){tile->src.x, tile->src.y, TILE_SIZE, TILE_SIZE},
										(Rectangle){cp.x, cp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE},(Vector2){0, 0}, 0, WHITE);
		
		// moving along display panel
		cp.x += DISPLAY_TILE_SIZE;

		// moving to new row
		if(cp.x == display_panel.x + display_panel.width)
		{
			cp.x = display_panel.x;
			cp.y += DISPLAY_TILE_SIZE;
		}

		// choosing tile from left clicking
		if (CheckCollisionPointRec(GetMousePosition(),(Rectangle){tile->sp.x, tile->sp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE}))
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				*current_tile = (Tile){tile->src, tile->sp, addTexture(textures, tile->fp), tile->tt, ""};
				strcpy(current_tile->fp, tile->fp);
			}

			DrawRectangleLines(tile->sp.x, tile->sp.y,DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, BLUE);
		}
	}

	// highlighting current tile
	if(current_tile->tx.id != 0)
	{
		DrawRectangleLines(current_tile->sp.x, current_tile->sp.y,DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, GREEN);
	}
}

// to edit a specific layer by tile creation and/or deletion
void editLayer(TileList *layer, Tile *current_tile, Rectangle *world_area, enum Element current_tile_type, int frames, int anim_speed)
{
	// creation
	if (CheckCollisionPointRec(mp, *world_area))
	{
		// where the tile will be
		DrawRectangleLines(mpx, mpy, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, BLUE);

		if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
		{
			bool to_add = true;

			// cant place tiles of the same type in the same location
			for (int i = 0; i < layer->size; i++)
			{
				if (Vector2Equals(layer->list[i].sp, (Vector2){mpx, mpy}))
				{
					to_add = false;
				}
			}

			// adding tile
			if (to_add)
			{
				Tile tile = (Tile){current_tile->src, (Vector2){mpx, mpy}, current_tile->tx, current_tile_type, "", true};
				strcpy(tile.fp, current_tile->fp);
				addTile(layer, tile);
			}
		}
	}

	int to_delete = -1;

	for (int i = 0; i < layer->size; i++)
	{
		if (layer->list[i].tx.id != 0)
		{
			Tile* tile = &layer->list[i];
			bool mouse_col = CheckCollisionPointRec(mp, (Rectangle){tile->sp.x, tile->sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE});
			
			// deletion
			if (mouse_col && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				to_delete = i;
			}

			// toggling animation
			if(mouse_col && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_A))
			{
				tile->fc = 0; 
				tile->frames = frames;
				tile->anim_speed = anim_speed;
				tile->anim = !tile->anim; 
			}
		}
	}

	if (to_delete != -1)
	{
		removeTile(layer, to_delete);
		to_delete = -1;
	}
}

void editWorld(World *world, Tile *current_tile, enum Element current_tile_type, int frames, int anim_speed)
{
	// deleting world
	if(IsKeyPressed(KEY_P))
	{
		for(int i = 0; i < world->textures.size; i++)
		{
			memset(world->textures.better_textures[i].fp, 0, sizeof(world->textures.better_textures[i].fp));
			UnloadTexture(world->textures.better_textures[i].tx);
		}

		world->textures.size = 0;

		resetLayer(&world->walls);
		resetLayer(&world->floors);
		resetLayer(&world->doors);
		resetLayer(&world->health_buffs);
		resetLayer(&world->damage_buffs);
		resetLayer(&world->interactables);

		*current_tile = (Tile){0};
	}

	// edit corresp layer based on layer selection
	switch (current_tile_type)
	{
		case WALL:
			editLayer(&world->walls, current_tile, &world->area, current_tile_type, frames, anim_speed);
			break;
		case FLOOR:
			editLayer(&world->floors, current_tile, &world->area, current_tile_type, frames, anim_speed);
			break;
		case DOOR:
			editLayer(&world->doors, current_tile, &world->area, current_tile_type, frames, anim_speed);
			break;
		case HEALTH_BUFF:
			editLayer(&world->health_buffs, current_tile, &world->area,current_tile_type, frames, anim_speed);
			break;
		case DAMAGE_BUFF:
			editLayer(&world->damage_buffs, current_tile, &world->area,current_tile_type, frames, anim_speed);
			break;
		case INTERACTABLE:
			editLayer(&world->interactables, current_tile, &world->area,current_tile_type, frames, anim_speed);
			break;
		// updating spawn point
		case SPAWN:
			if (CheckCollisionPointRec(mp, world->area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				world->spawn = (Vector2){mpx, mpy};
			}
			break;
		default:
			break;
	}
}

void drawLayer(TileList *layer, Color color, Rectangle world_area, const char *dsc)
{
	for (int i = 0; i < layer->size; i++)
	{
		Tile* tile = &layer->list[i];

		// only draw tile if in bounds
		if ((tile->sp.x < world_area.x + world_area.width) && (tile->sp.y < world_area.y + world_area.height))
		{
			DrawTexturePro(tile->tx,(Rectangle){tile->src.x, tile->src.y,TILE_SIZE,TILE_SIZE},
										(Rectangle){tile->sp.x, tile->sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},(Vector2){0, 0}, 0, WHITE);
			
			// character describing what type of tile it is
			DrawText(dsc, tile->sp.x + SCREEN_TILE_SIZE - MeasureText(dsc, 5), tile->sp.y + TILE_SIZE, 5, color);

			if(tile->anim)
			{
				DrawText("A", tile->sp.x + (TILE_SIZE / 2.0f), tile->sp.y + TILE_SIZE, 5, BLACK);
			}
		}
	}
}

void saveLayer(TileList *layer, char* filePath)
{
	FILE* outFile;
	outFile = fopen(filePath, "a"); 
	if (outFile == NULL)
	{
		printf("ERROR SAVING LAYER\n");
		return;
	}

	for (int i = 0; i < layer->size; i++)
	{
		Tile* tile = &layer->list[i];
		if (tile->tx.id != 0)
		{
			fprintf(outFile, "%d,%d,%d,%d,%d,%s,%d,%d,%d,%d\n",
				(int)tile->src.x, (int)tile->src.y, (int)tile->sp.x,(int)tile->sp.y, 
				tile->tt, tile->fp, tile->anim, tile->fc, tile->frames, tile->anim_speed);
		}
	}

	fclose(outFile);
}

void saveWorldWindow(World* world, bool* showInputTextBox, Tile* current_tile, char* saved_file_path)
{
	Rectangle input_window = (Rectangle){(GetScreenWidth() / 2.0f) - 120, (GetScreenHeight() / 2.0f) - 60, 240, 140};

	// toggles showing save world window
	if (GuiButton((Rectangle){16, 8, 96, 16},GuiIconText(ICON_FILE_SAVE, "SAVE WORLD"))) *showInputTextBox = true;

	if(*showInputTextBox)
	{
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),Fade(RAYWHITE, 0.8f));

		int result = GuiTextInputBox(input_window,GuiIconText(ICON_FILE_SAVE, "Save file as..."),
						"Introduce output file name:", "Ok;Cancel", saved_file_path, 255, NULL);

		// save world to chosen filename
		if (result == 1)
		{
			if (saved_file_path[0] != '\0')
			{
				strcat(saved_file_path, ".txt");

				saveLayer(&world->walls, saved_file_path);
				saveLayer(&world->floors, saved_file_path);
				saveLayer(&world->doors, saved_file_path);
				saveLayer(&world->health_buffs, saved_file_path);
				saveLayer(&world->damage_buffs, saved_file_path);
				saveLayer(&world->interactables, saved_file_path);

				// saving spawn point
				FILE * outFile = fopen("spawn.txt", "w");
				if(outFile == NULL)
				{
					printf("ERROR SAVING SPAWN \n");
				}

				fprintf(outFile, "%d %d\n", (int)world->spawn.x, (int)world->spawn.y);
				fclose(outFile);
			}
		}

		// reset flag upon saving and premature exit
		if ((result == 0) || (result == 1) || (result == 2))
		{
			*showInputTextBox = false;
			strcpy(saved_file_path, "");
		}
	}
}

void loadWorld(World *world, Rectangle *world_area, char* filePath)
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
	enum Element tt;
	char* fp ; 
	bool anim;
	int fc;
	int frames;
	int anim_speed;

	while (fgets(line, sizeof(line), inFile))
	{
		src.x = atoi(strtok(line, ","));
		src.y = atoi(strtok(NULL, ","));
		sp.x = atoi(strtok(NULL, ","));
		sp.y = atoi(strtok(NULL, ","));
		tt = (enum Element)atoi(strtok(NULL, ","));
		fp = strtok(NULL, ",");
		anim = atoi(strtok(NULL, ","));
		fc = atoi(strtok(NULL, ","));
		frames = atoi(strtok(NULL, ","));
		anim_speed = atoi(strtok(NULL, "\n"));
		
        Tile tile = (Tile){src, sp, addTexture(&world->textures, fp), tt, "NULL", true, anim, fc, frames, anim_speed};
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

void load(World* world, GuiWindowFileDialogState* fileDialogState, Tile* current_tile, TileList* tile_dictionary, Texture2D* current_texture)
{
	char selected_file_path[512] = "";
	Rectangle button_pos = (Rectangle){128, 8, 160, 16};

	// selecting png or text file to load textures or worlds to edit
	if (GuiButton(button_pos,GuiIconText(ICON_FILE_OPEN, "LOAD WORLD/TEXTURE"))) fileDialogState->windowActive = true;
	
	if (fileDialogState->SelectFilePressed)
	{
		*current_tile = (Tile){0};
		strcpy(selected_file_path,TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState->dirPathText,fileDialogState->fileNameText));
		
		if (IsFileExtension(fileDialogState->fileNameText, ".png"))
		{
			*current_texture = LoadTexture(selected_file_path);
			readImage(*current_texture, tile_dictionary, selected_file_path);
		}

		// loading a previously made world, clear prevoius world tiles and texture bank
		if (IsFileExtension(fileDialogState->fileNameText, ".txt"))
		{
			for(int i = 0; i < world->textures.size; i++)
			{
				memset(world->textures.better_textures[i].fp, 0, sizeof(world->textures.better_textures[i].fp));
				UnloadTexture(world->textures.better_textures[i].tx);
			}

			world->textures.size = 0;

			resetLayer(&world->walls);
			resetLayer(&world->floors);
			resetLayer(&world->doors);
			resetLayer(&world->health_buffs);
			resetLayer(&world->damage_buffs);
			resetLayer(&world->interactables);
			loadWorld(world, &world->area, selected_file_path);
		}

		fileDialogState->SelectFilePressed = false;
	}
}

void selectWorldSize(Rectangle *world_area)
{
	Rectangle edit_map_size_panel = {816,302,160,160};
	GuiPanel(edit_map_size_panel, "MAP SIZE");
	
	DrawText("Width", 877, 342, 15, GRAY);
	if (GuiButton((Rectangle){848, 358, 32, 32}, "+"))
	{
		world_area->width += SCREEN_TILE_SIZE;
	}
	
	if (GuiButton((Rectangle){912, 358, 32, 32}, "-"))
	{
		if (world_area->width > SCREEN_TILE_SIZE)
		{
			world_area->width -= SCREEN_TILE_SIZE;
		}
	}

	DrawText("Height", 874, 406, 15, GRAY);
	if (GuiButton((Rectangle){848, 422, 32, 32}, "+"))
	{
		world_area->height += SCREEN_TILE_SIZE;
	}

	if (GuiButton((Rectangle){912, 422, 32, 32}, "-"))
	{
		if (world_area->height > SCREEN_TILE_SIZE)
		{
			world_area->height -= SCREEN_TILE_SIZE;
		}
	}
}

void toggleEditing(bool* edit)
{
	if(IsKeyPressed(KEY_E)) *edit = (*edit == true) ? false : true;

	char* status = "";
	if(*edit) status = "EDTITING";
	else status = "FREE-VIEW";

	DrawText(status, 900 - (MeasureText(status, 30) / 2), 950, 30, GREEN);
}

void moveWorld(Camera2D* camera, bool edit)
{
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !edit)
	{
		Vector2 delta = GetMouseDelta();
		delta = Vector2Scale(delta, -1.0f / camera->zoom);

		camera->target = Vector2Add(camera->target, delta);
	}

	// reset world to original position
	if (GuiButton((Rectangle){304, 8, 64, 16},
			GuiIconText(ICON_FILE_SAVE, "FOCUS")))
	{
		camera->offset = mp;
	}
}

void selectLayer(int* current_layer)
{
	Rectangle layer_panel = {816, 32,160, 240};

	// choosing which layer to  add to
	GuiPanel(layer_panel, "LAYER TO EDIT");
	
	GuiToggleGroup((Rectangle){832,72,128, 24},
		"WALLS \n FLOORS \n DOORS \n BUFFS (HEALTH) \n BUFFS (DAMAGE) \n INTERACTABLES \n SPAWN POINT",current_layer);
}

void selectAnimationSettings(Rectangle* anim_speed, int* frames)
{
	Rectangle frame_count_panel = (Rectangle){816, 494, 160, 80};

	GuiPanel(frame_count_panel, "Number of Frames");

	if(GuiButton((Rectangle){826, 526, 30, 30}, "+"))
	{
		(*frames)++;
	}

	if(GuiButton((Rectangle){876, 526, 30, 30}, "-") && *frames > 0)
	{
		(*frames)--;
	}

	char frameDsc[512] = "";
	sprintf(frameDsc, "%d", *frames);
	DrawText(frameDsc, 936, 534, 15, BLACK);
	
	// slider for changing animation speed
	GuiSliderBar((Rectangle){ frame_count_panel.x + (MeasureText("SPEED", 10) / 2.0f), 600, 145, 15}, 
				"SPEED", TextFormat("%i", (int)anim_speed->width), &anim_speed->width, 1, 60);
}

int main()
{
	World world;
	Camera2D camera;

	bool edit = false;
	int current_layer;
	Tile current_tile;
	Texture current_texture;
	TileList tile_dictionary;
	
	init(&world, &tile_dictionary);

	// obj for file dialog window
	GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
	
	// for slider representing animation speed
	Rectangle anim_speed = (Rectangle){0};

	int frames = 0;
	camera.zoom = 1.0f;
	// flag for save world window
	bool showInputTextBox = false;
	char saved_file_path[512] = "";

	while (!WindowShouldClose())
	{
		mp = GetScreenToWorld2D(GetMousePosition(), camera);
		camera.offset = GetMousePosition();
		camera.target = mp;
		
		// snap mouse position to nearest tile
		mpx = (((int)mp.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
		mpy = (((int)mp.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
		
		BeginDrawing();

			ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

			BeginMode2D(camera);

				if(edit) editWorld(&world, &current_tile, (enum Element)current_layer, frames, anim_speed.width);

				drawLayer(&world.floors, RED, world.area, "F");
				drawLayer(&world.health_buffs, ORANGE, world.area, "HB");
				drawLayer(&world.damage_buffs, YELLOW, world.area, "DB");
				drawLayer(&world.doors, GREEN, world.area, "D");
				drawLayer(&world.interactables, BLUE, world.area, "I");
				drawLayer(&world.walls, PURPLE, world.area, "W");
				DrawText("S", world.spawn.x + SCREEN_TILE_SIZE, world.spawn.y + TILE_SIZE,5, PINK);
				DrawRectangleLines(world.area.x, world.area.y, world.area.width, world.area.height, GRAY);

			EndMode2D();

			// not able to interact with other buttons or edit tiles while save or load window is open
			if (showInputTextBox || fileDialogState.windowActive)
			{
				GuiLock();
				edit = false;
			}
			
			toggleEditing(&edit);
			moveWorld(&camera, edit);
			selectWorldSize(&world.area);
			selectLayer(&current_layer);
			selectAnimationSettings(&anim_speed, &frames);
			selectTile(&world.textures, &tile_dictionary, &current_tile);
			load(&world, &fileDialogState, &current_tile, &tile_dictionary, &current_texture);
			
			// able to interact with raygui elements again
			GuiUnlock();

			saveWorldWindow(&world, &showInputTextBox, &current_tile, saved_file_path);

			GuiWindowFileDialog(&fileDialogState);

		EndDrawing();
	}

	deinit(&world, &tile_dictionary);
	return 0;
}