#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "raylib.h"
#include <raymath.h>
#include "headers/generation.h"

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#undef RAYGUI_IMPLEMENTATION            
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "headers/gui_window_file_dialog.h"

const int SCREEN_WIDTH = 1120;
const int SCREEN_HEIGHT = 992;
const int PANEL_HEIGHT = 24;

int mpx, mpy = 0;

World world;

// mouse position relative to the 2d camera object 
Vector2 mp = { 0 };

void deinit(Texture2D texture, std::vector<Tile>& tile_dict)
{
	for(int i = 0; i < tile_dict.size(); i++)
	{
		UnloadTexture(tile_dict[i].tx);
	}
	
	std::ofstream outFile("spawn.txt");
	outFile  << world.spawn.x << " " << world.spawn.y << std::endl;
	outFile.close();

	tile_dict.clear();

    UnloadTexture(texture);
    CloseWindow();
}

// breaks the user-selected png into tiles that user can choose from to append to the world
void readPNG(Texture2D texture, std::vector<Tile>& tile_dict, std::string fp)
{
	Vector2 cp = {0, 0};
	// surface area of the image with respect to tilesize
	int sa = (texture.width / TILE_SIZE) * (texture.height / TILE_SIZE);
	while(cp.x < texture.width && cp.y < texture.height)
	{
		tile_dict.push_back({cp, { 0 }, texture, fp});

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

// showing availible tiles that user can append the map to
void tileSelection(std::vector<Tile>& tile_dict, Rectangle side_panel, Tile& currTile)
{

	GuiPanel(side_panel, "TILES");
	// the last tile before overflowing the side panel, TODO FIX HARDCODED Y VALUE
	Vector2 lt = {side_panel.x + side_panel.width - DISPLAY_TILE_SIZE,888};
	Vector2 cp1 = {side_panel.x,float(side_panel.y + PANEL_HEIGHT)};

	for(int i = 0; i < tile_dict.size(); i++)
	{
		tile_dict[i].sp = cp1;

		// shrink to fit
		if(Vector2Equals(tile_dict[i].sp, lt))
		{
			DISPLAY_TILE_SIZE /= 2;
			currTile = { 0 };
		}
		
		DrawTexturePro(tile_dict[i].tx, {tile_dict[i].src.x, tile_dict[i].src.y, TILE_SIZE,TILE_SIZE}, {cp1.x, cp1.y, float(DISPLAY_TILE_SIZE), float(DISPLAY_TILE_SIZE)}, {0,0}, 0, WHITE);
		cp1.x += DISPLAY_TILE_SIZE;
		
		// moving to new row
		if((i + 1) % int(side_panel.width / DISPLAY_TILE_SIZE) == 0 && i != 0)
		{
			cp1.x = side_panel.x;
			cp1.y += DISPLAY_TILE_SIZE;
		}

		if(CheckCollisionPointRec(GetMousePosition(), {tile_dict[i].sp.x, tile_dict[i].sp.y, float(DISPLAY_TILE_SIZE), float(DISPLAY_TILE_SIZE)}))
		{
			// higlight tile that user is hovering over
			DrawRectangleLines(tile_dict[i].sp.x, tile_dict[i].sp.y, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, BLUE);
			// update current tile
			if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				currTile = tile_dict[i];
			}
		}
	}

	// reset tiles to choose from
	if(IsKeyPressed(KEY_F))
	{
		tile_dict.clear();
		currTile = { 0 };
		DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;
	}

	// highlight the current tile a user has selected
	if(currTile.tx.id != 0)
	{
		DrawRectangleLines(currTile.sp.x, currTile.sp.y, DISPLAY_TILE_SIZE,DISPLAY_TILE_SIZE, GREEN);
	}
}

// to edit a specific layer by tile creation and/or deletion
void editLayer(std::vector<Tile>& layer, Tile& currTile, Rectangle& worldArea, Element ce)
{
	// creation
	if(CheckCollisionPointRec(mp, worldArea))
	{
		// draw where tile will be
		DrawRectangleLines(mpx, mpy, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, BLUE);
			
		if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !currTile.fp.empty())
		{
			// create new tile
			layer.push_back({currTile.src, {float(mpx), float(mpy)}, currTile.tx, currTile.fp, ce});
		}
	}

	// deletion
	for(int i = 0; i < layer.size(); i++)
	{
		if(!layer[i].fp.empty())
		{
			if(CheckCollisionPointRec(mp, {layer[i].sp.x, layer[i].sp.y, float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}) && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && IsKeyDown(KEY_E))
			{
				layer.erase(layer.begin() + i);
			}
		}
	}
}
void editWorld(Rectangle worldArea, Tile& currTile, Element ce, int& cl)
{
	// edit corresp layer based on layer selection  
	switch (ce)
	{
		case WALL: 
			editLayer(world.walls, currTile, worldArea, ce); break;
		case FLOOR: 
			editLayer(world.floors, currTile, worldArea, ce); break;
		case DOOR:
			editLayer(world.doors, currTile, worldArea,ce); break;
		case HEALTH_BUFF:
			editLayer(world.health_buffs, currTile, worldArea, ce); break;
		case DAMAGE_BUFF:
			editLayer(world.damage_buffs, currTile, worldArea, ce); break;
		case INTERACTABLE: 
			editLayer(world.interactables, currTile, worldArea, ce); break;
		case SPAWN:
			if(CheckCollisionPointRec(mp, worldArea))
			{
				if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
				{
					world.spawn = {float(mpx), float(mpy)};
				}
			}
			break;
		default:
			break;
	}

	// toggle seeing tile descriptor or not
	if(IsKeyPressed(KEY_Y))
	{
			cl = (cl == 6) ? 0 : 6;
	}
}

// editing map size
void updateMapSize(Rectangle& worldArea, Rectangle edit_map_panel)
{
	GuiPanel(edit_map_panel, "MAP SIZE");
	DrawText("Width", edit_map_panel.x + ((edit_map_panel.width / 2) - MeasureText("Width", 15) / 2.0), edit_map_panel.y + PANEL_HEIGHT + TILE_SIZE, 15, GRAY);
	if(GuiButton({edit_map_panel.x + SCREEN_TILE_SIZE, edit_map_panel.y + PANEL_HEIGHT + SCREEN_TILE_SIZE, float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}, "+"))
	{
		
		worldArea.width += SCREEN_TILE_SIZE;
	}

	if(GuiButton({edit_map_panel.x + SCREEN_TILE_SIZE * 3, edit_map_panel.y + PANEL_HEIGHT + SCREEN_TILE_SIZE, float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}, "-"))
	{
		if(worldArea.width > SCREEN_TILE_SIZE)
		{
			worldArea.width -= SCREEN_TILE_SIZE;
		};
	}

	DrawText("Height", edit_map_panel.x + ((edit_map_panel.width / 2) - MeasureText("Height", 15) / 2.0), edit_map_panel.y + PANEL_HEIGHT + TILE_SIZE + SCREEN_TILE_SIZE * 2, 15, GRAY);
	if(GuiButton({edit_map_panel.x + SCREEN_TILE_SIZE, edit_map_panel.y + PANEL_HEIGHT + (SCREEN_TILE_SIZE * 3), float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}, "+"))
	{
		worldArea.height += SCREEN_TILE_SIZE;
	}

	if(GuiButton({edit_map_panel.x + SCREEN_TILE_SIZE * 3, edit_map_panel.y + PANEL_HEIGHT + (SCREEN_TILE_SIZE * 3), float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}, "-"))
	{
		if(worldArea.height > SCREEN_TILE_SIZE)
		{
			worldArea.height -= SCREEN_TILE_SIZE;
		}
	}
}

int main()
{
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "EDITOR");
    SetTargetFPS(60);

    GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
	Rectangle side_panel = {TILE_SIZE, float(SCREEN_TILE_SIZE), SCREEN_TILE_SIZE * 8.0f, SCREEN_TILE_SIZE * 28.0f};
	Rectangle layer_panel = {GetScreenWidth() - TILE_SIZE - SCREEN_TILE_SIZE * 5.0f, float(SCREEN_TILE_SIZE), SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE * 7.0f};
	Rectangle edit_map_panel = {GetScreenWidth() - TILE_SIZE - SCREEN_TILE_SIZE * 5.0f, layer_panel.y + layer_panel.height + SCREEN_TILE_SIZE, SCREEN_TILE_SIZE * 5.0f, SCREEN_TILE_SIZE * 5.0f};
	// current layer user selects
	int cl = -1;
	// flag to show input box to name txt file
	bool showTextInputBox = false;
	char textInput [256];
	std::vector<Tile> tile_dict;
    std::string userFilePath;
    Texture2D texture = { 0 };
	Rectangle worldArea = {side_panel.x + side_panel.width + TILE_SIZE, float(SCREEN_TILE_SIZE), 512,512};
	Camera2D camera = { 0 };
	camera.zoom = 1.0f;
	Tile currTile = { 0 };

    while (!WindowShouldClose())
    {
        mp = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        camera.target = mp;
		// snap mouse position to nearest tile by making it divisble by the screen tile size
		mpx = (((int)mp.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
   	 	mpy = (((int)mp.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));

		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f/camera.zoom);

            camera.target = Vector2Add(camera.target, delta);
        }

		// pressing "select" in file dialog
        if (fileDialogState.SelectFilePressed)
        {
            if (IsFileExtension(fileDialogState.fileNameText, ".png"))
            {
                userFilePath = TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState.dirPathText, fileDialogState.fileNameText);
                texture = LoadTexture(userFilePath.c_str());
				readPNG(texture, tile_dict, userFilePath);
            }

			// loading previously saved file
			if(IsFileExtension(fileDialogState.fileNameText, ".txt"))
			{
				clearWorld(world);
				loadLayers(fileDialogState.fileNameText, worldArea, world);
			}

            fileDialogState.SelectFilePressed = false;
        }
		
        BeginDrawing();

            ClearBackground(WHITE);

			// world moves 
			BeginMode2D(camera);
				
				DrawRectangleLines(worldArea.x, worldArea.y, worldArea.width, worldArea.height, GRAY);
				editWorld(worldArea, currTile, (Element)cl, cl);
				// drawing each layer
				bool showDsc = (cl != 6);
				drawLayer(world.floors, RED, worldArea, "F", showDsc);
				drawLayer(world.health_buffs, ORANGE, worldArea, "HB", showDsc);
				drawLayer(world.damage_buffs, YELLOW, worldArea, "DB", showDsc);
				drawLayer(world.doors, GREEN, worldArea, "D", showDsc);
				drawLayer(world.interactables,BLUE, worldArea, "I", showDsc);
				drawLayer(world.walls, PURPLE, worldArea, "W", showDsc);
				DrawText("S", world.spawn.x + SCREEN_TILE_SIZE, world.spawn.y + TILE_SIZE, 5, PINK);
				
			EndMode2D();
			// selecting tiles from left side panel
			tileSelection(tile_dict, side_panel, currTile);
			// updating the size of the map
			updateMapSize(worldArea, edit_map_panel);
			// showing layers
			GuiPanel(layer_panel, "LAYER TO EDIT");
			GuiToggleGroup((Rectangle){ GetScreenWidth() - SCREEN_TILE_SIZE * 5.0f, float(SCREEN_TILE_SIZE + PANEL_HEIGHT + TILE_SIZE), SCREEN_TILE_SIZE * 4.0f, 24}, "WALLS \n FLOORS \n DOORS \n BUFFS (HEALTH) \n BUFFS (DAMAGE) \n INTERACTABLES \n SPAWN POINT", &cl);

            // only focus on the window choosing your file
            if (fileDialogState.windowActive)
			{
				GuiLock();
				currTile = { 0 };
			} 

			// saving worlds and loading textures
			if (GuiButton((Rectangle){ TILE_SIZE, 8, float(SCREEN_TILE_SIZE * 3), float(TILE_SIZE) }, GuiIconText(ICON_FILE_SAVE, "SAVE WORLD"))) showTextInputBox = true;
            if (GuiButton((Rectangle){ float(SCREEN_TILE_SIZE  * 4), 8, float(SCREEN_TILE_SIZE * 5), float(TILE_SIZE) }, GuiIconText(ICON_FILE_OPEN, "LOAD WORLD/TEXTURE"))) fileDialogState.windowActive = true;

            // able to click other buttons once you select the desired image
            GuiUnlock();

			// updates the file dialouge window
            GuiWindowFileDialog(&fileDialogState);

			// enterting file name to save new world
			if (showTextInputBox)
            {
				currTile = {  0 };
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, 0.8f));
                int result = GuiTextInputBox((Rectangle){ (float)GetScreenWidth()/2 - 120, (float)GetScreenHeight()/2 - 60, 240, 140 }, GuiIconText(ICON_FILE_SAVE, "Save file as..."), "Introduce output file name:", "Ok;Cancel", textInput, 255, NULL);

				// save world to chosen filename
                if (result == 1)
                {
                    if(textInput[0] != '\0')
					{
						system(("touch " + std::string(textInput) + ".txt").c_str());
						
						saveLayer(world.walls, std::string(textInput) + ".txt");
						saveLayer(world.floors, std::string(textInput) + ".txt");
						saveLayer(world.doors, std::string(textInput) + ".txt");
						saveLayer(world.health_buffs, std::string(textInput) + ".txt");
						saveLayer(world.damage_buffs, std::string(textInput) + ".txt");
						saveLayer(world.interactables, std::string(textInput) + ".txt");
						cl = 6;
					}
                }
				
				// reset flag upon saving and premature exit
				if ((result == 0) || (result == 1) || (result == 2))
				{
					showTextInputBox = false;
				}
            }
		
        EndDrawing();
    }

    deinit(texture, tile_dict);
    return 0;
}
