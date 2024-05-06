#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#undef RAYGUI_IMPLEMENTATION
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"
#include "2d-tile-editor.h"

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;
const int PANEL_HEIGHT = 24;
const int FPS = 60;
int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

int mpx, mpy = 0;

// mouse position relative to the 2d camera object
Vector2 mp = {0,0};

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
	for (int i = 0; i < layer->size; i++) {
		UnloadTexture(layer->list[i].tx);
	}

	free(layer->list);
	layer->list = malloc(TILE_CAP);
	layer->size = 0;
	layer->cap = TILE_CAP;
}

void addTile(TileList *layer, Tile tile)
{
	if (layer->size * sizeof(Tile) == layer->cap)
	{
		resizeLayer(layer);
	}

	layer->list[layer->size++] = tile;
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
		textures->size++;
	}

	// if we already have this tile, use it at that index, if not, the newest texture should hold the correct one to load
	ctxi = (ctxi == -1) ? (textures->size - 1) : ctxi;

	// return the texture of new or recurring element
	return textures->better_textures[ctxi].tx;
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
		resetLayer(layer);
	}
}

void init(World *world, TileList *tile_dict)
{
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "Editor");
	SetTargetFPS(FPS);
	
	tile_dict->list = malloc(TILE_CAP);
	tile_dict->cap = TILE_CAP;
	
	world->area = (Rectangle){288,32, 512, 512};
	world->spawn = (Vector2){0, 0};
	
	world->walls.size = 0;
	world->doors.size = 0;
	world->floors.size = 0;
	world->textures.size = 0;
	world->health_buffs.size = 0;
	world->damage_buffs.size = 0;
	world->interactables.size = 0;

	world->walls.list = malloc(TILE_CAP);
	world->doors.list = malloc(TILE_CAP);
	world->floors.list = malloc(TILE_CAP);
	world->health_buffs.list = malloc(TILE_CAP);
	world->damage_buffs.list = malloc(TILE_CAP);
	world->interactables.list = malloc(TILE_CAP);

    world->walls.cap = TILE_CAP;
    world->doors.cap = TILE_CAP;
    world->floors.cap = TILE_CAP;
    world->health_buffs.cap = TILE_CAP;
    world->damage_buffs.cap = TILE_CAP;
	world->interactables.cap = TILE_CAP;
}

void deinit(World *world, TileList *tile_dict)
{
	free(tile_dict->list);
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

void selectTile(Textures* textures, Rectangle side_panel, TileList *tile_dict, Tile *current_tile)
{
	GuiPanel(side_panel, "TILES");

	// button to remove tiles to choose from
	if (GuiButton((Rectangle){side_panel.x + side_panel.width - PANEL_HEIGHT,side_panel.y, PANEL_HEIGHT, PANEL_HEIGHT},GuiIconText(ICON_BIN, "")))
	{
		resetLayer(tile_dict);
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
				*current_tile = (Tile){tile_dict->list[i].src, tile_dict->list[i].sp, addTexture(textures, tile_dict->list[i].fp), tile_dict->list[i].tt, "NULL"};
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
void editLayer(TileList *layer, Tile *current_tile, Rectangle *world_area,enum Element current_tile_type, int frames, int anim_speed)
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

			// adding tile
			if (to_add)
			{
				Tile tile = (Tile){current_tile->src, (Vector2){mpx, mpy}, current_tile->tx, current_tile_type, "NULL", true};
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
			bool mouse_col = CheckCollisionPointRec(mp, (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE});
			
			// deletion
			if (mouse_col && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				to_delete = i;
			}

			// toggling animation
			if(mouse_col && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_A))
			{
				layer->list[i].anim = !layer->list[i].anim; layer->list[i].fc = 0; 
				layer->list[i].frames = frames;
				layer->list[i].anim_speed = anim_speed;
			}
		}
	}

	if (to_delete != -1)
	{
		removeTile(layer, to_delete);
		to_delete = -1;
	}
}

void editWorld(World *world, Rectangle world_area, Tile *current_tile, enum Element current_tile_type, int frames, int anim_speed)
{
	// deleting world
	if(IsKeyPressed(KEY_P))
	{
		resetLayer(&world->walls);
		resetLayer(&world->floors);
		resetLayer(&world->doors);
		resetLayer(&world->health_buffs);
		resetLayer(&world->damage_buffs);
		resetLayer(&world->interactables);
		*current_tile = (Tile){ 0 };
	}

	// edit corresp layer based on layer selection
	switch (current_tile_type)
	{
		case WALL:
			editLayer(&world->walls, current_tile, &world_area, current_tile_type, frames, anim_speed);
			break;
		case FLOOR:
			editLayer(&world->floors, current_tile, &world_area, current_tile_type, frames, anim_speed);
			break;
		case DOOR:
			editLayer(&world->doors, current_tile, &world_area, current_tile_type, frames, anim_speed);
			break;
		case HEALTH_BUFF:
			editLayer(&world->health_buffs, current_tile, &world_area,current_tile_type, frames, anim_speed);
			break;
		case DAMAGE_BUFF:
			editLayer(&world->damage_buffs, current_tile, &world_area,current_tile_type, frames, anim_speed);
			break;
		case INTERACTABLE:
			editLayer(&world->interactables, current_tile, &world_area,current_tile_type, frames, anim_speed);
			break;
		// updating spawn point
		case SPAWN:
			if (CheckCollisionPointRec(mp, world_area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
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
		// only draw tile if in bounds
		if (layer->list[i].tx.id != 0 && (layer->list[i].sp.x < world_area.x + world_area.width) && (layer->list[i].sp.y < world_area.y + world_area.height))
		{
			DrawTexturePro(layer->list[i].tx,(Rectangle){layer->list[i].src.x, layer->list[i].src.y,TILE_SIZE,
								 TILE_SIZE},(Rectangle){layer->list[i].sp.x, layer->list[i].sp.y,SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},(Vector2){0, 0}, 0, WHITE);
			
			// character describing what type of tile it is
			DrawText(dsc,layer->list[i].sp.x + SCREEN_TILE_SIZE - MeasureText(dsc, 5),layer->list[i].sp.y + TILE_SIZE, 5, color);

			if(layer->list[i].anim)
			{
				DrawText("A", layer->list[i].sp.x + (TILE_SIZE / 2.0f), layer->list[i].sp.y + TILE_SIZE, 5, BLACK);
			}
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
			fprintf(outFile, "%d,%d,%d,%d,%d,%s,%d,%d,%d,%d\n", (int)layer->list[i].src.x,(int)layer->list[i].src.y, 
						(int)layer->list[i].sp.x,(int)layer->list[i].sp.y, layer->list[i].tt, layer->list[i].fp, layer->list[i].anim, layer->list[i].fc, layer->list[i].frames, layer->list[i].anim_speed);
		}
	}

	fclose(outFile);
}

void saveWorldWindow(World* world, bool* showInputTextBox, Tile* current_tile, char* saved_file_path)
{
	// toggles showing save world windoe
	if (GuiButton((Rectangle){16, 8, 96, 16},GuiIconText(ICON_FILE_SAVE, "SAVE WORLD"))) *showInputTextBox = true;

	if(*showInputTextBox)
	{
		*current_tile = (Tile){0};
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),Fade(RAYWHITE, 0.8f));

		int result = GuiTextInputBox((Rectangle){(GetScreenWidth() / 2.0f) - 120, (GetScreenHeight() / 2.0f) - 60, 240, 140},
						GuiIconText(ICON_FILE_SAVE, "Save file as..."),"Introduce output file name:", "Ok;Cancel", saved_file_path, 255, NULL);

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

				// saving spawnpoint
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

void loadWorldOrTexture(World* world, GuiWindowFileDialogState* fileDialogState, Tile* current_tile, TileList* tile_dict, Texture2D* current_texture)
{
	// selecting png or text file to load textures or worlds to edit
	if (GuiButton((Rectangle){128, 8, 160, 16},GuiIconText(ICON_FILE_OPEN, "LOAD WORLD/TEXTURE"))) fileDialogState->windowActive = true;
	
	char selected_file_path[512] = "";
	
	// file dialog window that mirrors the files explorer in windos, mac, linux,etc.
	if (fileDialogState->SelectFilePressed)
	{
		*current_tile = (Tile){};
		strcpy(selected_file_path,TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState->dirPathText,fileDialogState->fileNameText));
		
		if (IsFileExtension(fileDialogState->fileNameText, ".png"))
		{
			*current_texture = addTexture(&world->textures, selected_file_path);
			readPNG(*current_texture, tile_dict, selected_file_path);
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

void toggleEditing(bool* edit)
{
	if(IsKeyPressed(KEY_E))
	{
		*edit = (*edit == true) ? false : true;
	}

	char* status = "";

	if(*edit)
	{
		status = "EDTITING";
	}

	else
	{
		status = "FREE-VIEW";
	}

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

void selectLayer(Rectangle layer_panel, int* current_layer)
{
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
	Tile current_tile;
	int current_layer;
	bool edit = false;
	TileList tile_dict;
	Texture current_texture;
	init(&world, &tile_dict);

	// obj for file dialog window
	GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());

	// bounds for panels of the editor
	Rectangle side_panel = {16, 32, 256, 896};
	Rectangle layer_panel = {816, 32,160, 240};
	Rectangle edit_map_size_panel = {816,302,160, 160};
	
	// for slider representing animation speed
	Rectangle anim_speed = (Rectangle){0};

	char saved_file_path[512] = "";

	// flag for displaying window when saving world 
	bool showInputTextBox = false;
	int frames = 0;
	camera.zoom = 1.0f;

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

				if(edit) editWorld(&world, world.area, &current_tile, (enum Element)current_layer, frames, anim_speed.width);

				drawLayer(&world.floors, RED, world.area, "F");
				drawLayer(&world.health_buffs, ORANGE, world.area, "HB");
				drawLayer(&world.damage_buffs, YELLOW, world.area, "DB");
				drawLayer(&world.doors, GREEN, world.area, "D");
				drawLayer(&world.interactables, BLUE, world.area, "I");
				drawLayer(&world.walls, PURPLE, world.area, "W");
				DrawText("S", world.spawn.x + SCREEN_TILE_SIZE, world.spawn.y + TILE_SIZE,5, PINK);
				DrawRectangleLines(world.area.x, world.area.y, world.area.width, world.area.height, GRAY);

			EndMode2D();

			// not able to interact with other buttons while save or load window is open
			if (showInputTextBox || fileDialogState.windowActive) GuiLock();
			
			toggleEditing(&edit);
			moveWorld(&camera, edit);
			loadWorldOrTexture(&world, &fileDialogState, &current_tile, &tile_dict, &current_texture);
			changeWorldSize(&world.area, edit_map_size_panel);
			selectTile(&world.textures, side_panel, &tile_dict, &current_tile);
			selectLayer(layer_panel, &current_layer);
			selectAnimationSettings(&anim_speed, &frames);
			
			// able to interact with raygui elements again
			GuiUnlock();

			saveWorldWindow(&world, &showInputTextBox, &current_tile, saved_file_path);

			GuiWindowFileDialog(&fileDialogState);

		EndDrawing();
	}

	deinit(&world, &tile_dict);
	return 0;
}