#ifndef TILE_H
#define TILE_H

#include "entity.h"
#include "raylib.h" 
#include "animation.h"
#include <stdlib.h>

typedef enum 
{
	UNDF = -1,
	WALL = 0,
	FLOOR = 1,
	DOOR = 2,
	HEALTH_BUFF = 3,
	DAMAGE_BUFF = 4,
	INTERACTABLE = 5,
} TileType;

typedef struct
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	TileType tt;
	char fp[1024];
	bool animated;
	Animation animtaion;
} Tile;

typedef struct
{
	int size;
	size_t cap;
	Tile* list;
} TileList;


void resize_tilelist(TileList* tilelist);
void add_tile(TileList* tilelist, Tile tile);
void delete_tile(TileList* tilelist, int pos);
TileList create_tilelist();
int check_collision_tilelist(Rectangle en_area, TileList* tilelist); 
void draw_tilelist(TileList* tilelist, Rectangle world_border);
void dealloc_tilelist(TileList* tilelist);
void clear_tilelist(TileList* tilelist);

#endif