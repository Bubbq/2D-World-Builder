#include "raylib.h"
#include <cstdlib>
#include <iostream>
#include <raymath.h>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

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
int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

enum Element{
	WALL = 0,
	FLOOR = 1,
	DOOR = 2,
	BUFF = 3,
	INTERACTABLE = 4,
	UNDF = 5,
};

struct Tile
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	std::string fp;
	Element tt;
};

std::vector<Tile> walls;
std::vector<Tile> floors;
std::vector<Tile> doors;
std::vector<Tile> buffs;
std::vector<Tile> interactables;

// mouse position relative to the 2d camera object 
Vector2 mp = { 0 };

void saveLayer(std::vector<Tile>& layer, std::string filePath)
{
	std::ofstream outFile(filePath, std::ios::app);
	if(!outFile)
	{
		std::cerr << "ERROR SAVING LEVEL \n";
		return;
	}

	for(int i = 0; i < layer.size(); i++)
	{
		outFile << layer[i].src.x << ","
		 		<< layer[i].src.y << ","
				<< layer[i].sp.x << ","
				<< layer[i].sp.y << ","
				<< layer[i].fp << ","
				<< layer[i].tt << std::endl;
	}

	outFile.close();
}

void loadLayers(std::string filePath, Rectangle& worldArea)
{
    std::ifstream inFile(filePath);
	if(!inFile)
	{
		std::cerr << "NO LAYERS TO SAVE \n";
		return;
	}

	// containing the records of every peice of data in txt file
	std::vector<std::vector<std::string>> data;

	while(inFile)
	{
		std::string line;
		if(!std::getline(inFile, line)) break;

		std::stringstream ss(line);
		// splits each element in the ith line into a vector of values
		std::vector<std::string> record;

		while(ss)
		{
			std::string s;
			if(!std::getline(ss, s, ',')) break;

			record.push_back(s);
		}
		data.push_back(record);
	}

	inFile.close();

	// iterate through every line in txt file
	for(const auto& record : data)
	{
		Vector2 src = {std::stof(record[0]), std::stof(record[1])};
		Vector2 sp = {std::stof(record[2]), std::stof(record[3])};
		std::string fp = record[4];

		Element tt = Element(std::stoi(record[5]));
		Texture2D tx = LoadTexture(fp.c_str());

		Tile loadedTile = {src, sp, tx, fp, tt};
		
		// adj world size if loading world larger than current one
		if(loadedTile.sp.x + (SCREEN_TILE_SIZE * 2)  > worldArea.x + worldArea.width)
		{
			worldArea.width += SCREEN_TILE_SIZE;
		}
		
		if(loadedTile.sp.y + SCREEN_TILE_SIZE > worldArea.y + worldArea.height)
		{
			worldArea.height += SCREEN_TILE_SIZE;
		}

		// add element to appropriate layer
		switch (tt)
		{
			case WALL: walls.push_back(loadedTile); break;
			case FLOOR: floors.push_back(loadedTile); break;
			case DOOR: doors.push_back(loadedTile); break;
			case BUFF: buffs.push_back(loadedTile); break;
			case INTERACTABLE: interactables.push_back(loadedTile); break;
			default:
				break;
		}
	}
}

void deinit(Texture2D texture, std::vector<Tile>& tile_dict)
{
	for(int i = 0; i < tile_dict.size(); i++)
	{
		UnloadTexture(tile_dict[i].tx);
	}
	
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
void editLayer(std::vector<Tile>& layer, Tile& currTile, Rectangle& worldArea)
{
	// snap mouse position to nearest tile by making it divisble by the screen tile size
	int mpx = (((int)mp.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
    int mpy = (((int)mp.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));

	// creation
	if(CheckCollisionPointRec(mp, worldArea))
	{
		// draw where tile will be
		DrawRectangleLines(mpx, mpy, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, BLUE);
			
		if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !currTile.fp.empty())
		{
			// create new tile
			layer.push_back({currTile.src, {float(mpx), float(mpy)}, currTile.tx, currTile.fp, currTile.tt});
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

void drawLayer(std::vector<Tile>& layer, Color color, Rectangle worldArea, const char * dsc)
{
	for(int i = 0; i < layer.size(); i++)
	{
		// only draw tile if in bounds
		if(!layer[i].fp.empty() && layer[i].sp.x < worldArea.x + worldArea.width && layer[i].sp.y < worldArea.y + worldArea.height)
		{
			DrawTexturePro(layer[i].tx, {layer[i].src.x, layer[i].src.y, TILE_SIZE, TILE_SIZE}, {layer[i].sp.x, layer[i].sp.y, float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}, {0,0}, 0, WHITE);
			
			// char desc the type of tile it is
			DrawText(dsc, layer[i].sp.x + SCREEN_TILE_SIZE - MeasureText(dsc, 5), layer[i].sp.y + TILE_SIZE, 5, color);
		}
	}
}

void editWorld(Rectangle worldArea, Tile& currTile, int ce)
{
	// snap mouse position to nearest tile by making it divisble by the screen tile size
	int mpx = (((int)mp.x >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
    int mpy = (((int)mp.y >> (int)log2(SCREEN_TILE_SIZE)) << (int)log2(SCREEN_TILE_SIZE));
	
	// edit corresp layer based on layer selection  
	switch (ce)
	{
		case WALL: 
			editLayer(walls, currTile, worldArea); break;
		case FLOOR: 
			editLayer(floors, currTile, worldArea); break;
		case DOOR:
			editLayer(doors, currTile, worldArea); break;
		case BUFF:
			editLayer(buffs, currTile, worldArea); break;
		case INTERACTABLE: 
			editLayer(interactables, currTile, worldArea); break;
		default:
			break;
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

	// Shrink world width button logic
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

	// shrink world height
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
	int cl = 5;
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
				loadLayers(fileDialogState.fileNameText, worldArea);
			}

            fileDialogState.SelectFilePressed = false;
        }
		
        BeginDrawing();

            ClearBackground(WHITE);

			// world moves 
			BeginMode2D(camera);
				
				DrawRectangleLines(worldArea.x, worldArea.y, worldArea.width, worldArea.height, GRAY);
				editWorld(worldArea, currTile, (Element)cl);
				
				// draw each layer, dont show tile descriptor if in "player mode"
				drawLayer(floors, RED, worldArea, "F");
				drawLayer(buffs, YELLOW, worldArea, "B");
				drawLayer(doors, ORANGE, worldArea, "D");
				drawLayer(interactables,GREEN, worldArea, "I");
				drawLayer(walls, BLUE, worldArea, "W");
				
			EndMode2D();

			tileSelection(tile_dict, side_panel, currTile);
			updateMapSize(worldArea, edit_map_panel);
			// showing layers
			GuiPanel(layer_panel, "LAYER TO EDIT");
			GuiToggleGroup((Rectangle){ GetScreenWidth() - SCREEN_TILE_SIZE * 5.0f, float(SCREEN_TILE_SIZE + PANEL_HEIGHT + TILE_SIZE), SCREEN_TILE_SIZE * 4.0f, 24}, "WALLS \n FLOORS \n DOORS \n BUFFS \n INTERACTABLES \n PLAYER VIEW", &cl);
            

            // only focus on the window choosing your file
            if (fileDialogState.windowActive) GuiLock();

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
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, 0.8f));
                int result = GuiTextInputBox((Rectangle){ (float)GetScreenWidth()/2 - 120, (float)GetScreenHeight()/2 - 60, 240, 140 }, GuiIconText(ICON_FILE_SAVE, "Save file as..."), "Introduce output file name:", "Ok;Cancel", textInput, 255, NULL);

				// save world to chosen filename
                if (result == 1)
                {
                    if(textInput[0] != '\0')
					{
						system(("touch " + std::string(textInput) + ".txt").c_str());
						
						saveLayer(walls, std::string(textInput) + ".txt");
						saveLayer(floors, std::string(textInput) + ".txt");
						saveLayer(doors, std::string(textInput) + ".txt");
						saveLayer(buffs, std::string(textInput) + ".txt");
						saveLayer(interactables, std::string(textInput) + ".txt");
						cl = 5;
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
