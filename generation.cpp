#include "headers/generation.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <raylib.h>

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

void loadLayers(std::string filePath, Rectangle& worldArea, World& world, Vector2 spawn)
{
	std::ifstream file("spawn.txt");
	if(!file)
	{
		std::cerr << "NO SPAWN POINT SAVED 'n";
	}

	float spx, spy;
	file >> spx >> spy;
	std::cout << spx <<  " " << spy << std::endl;

	spawn = {spx, spy};
	file.close();

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
			case WALL: world.walls.push_back(loadedTile); break;
			case FLOOR: world.floors.push_back(loadedTile); break;
			case DOOR: world.doors.push_back(loadedTile); break;
			case BUFF: world.buffs.push_back(loadedTile); break;
			case INTERACTABLE: world.interactables.push_back(loadedTile); break;
			default:
				break;
		}
	}
}

void drawLayer(std::vector<Tile>& layer, Color color, Rectangle worldArea, const char * dsc, bool showDsc)
{
	for(int i = 0; i < layer.size(); i++)
	{
		// only draw tile if in bounds
		if(!layer[i].fp.empty() && layer[i].sp.x < worldArea.x + worldArea.width && layer[i].sp.y < worldArea.y + worldArea.height)
		{
			DrawTexturePro(layer[i].tx, {layer[i].src.x, layer[i].src.y, TILE_SIZE, TILE_SIZE}, {layer[i].sp.x, layer[i].sp.y, float(SCREEN_TILE_SIZE), float(SCREEN_TILE_SIZE)}, {0,0}, 0, WHITE);
			
			// char desc the type of tile it is
			if(showDsc)
			{
				DrawText(dsc, layer[i].sp.x + SCREEN_TILE_SIZE - MeasureText(dsc, 5), layer[i].sp.y + TILE_SIZE, 5, color);
			}
		}
	}
}

void clearWorld(World& world)
{
    world.walls.clear();
    world.buffs.clear();
    world.doors.clear();
    world.floors.clear();
    world.interactables.clear();
}
