#ifndef TILE_H
#define TILE_H

#include "entity.h"
#include "raylib.h" 
#include "animation.h"

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
	Vector2 position_in_sprite;
	Vector2 screen_position;
	Texture2D sprite;
	TileType tile_type;
	char sprite_file_path[1024];
	bool is_animated;
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
int check_collision_tilelist(Rectangle en_area, TileList* tilelist, int tile_size); 
void animate_tilelist(TileList* tilelist);
void draw_tilelist(TileList* tilelist, Rectangle world_border, int tile_size, int sprite_sheet_size);
void label_tilelist(TileList* tilelist, Rectangle world_border, Color text_color, int tile_size, const char* tile_descriptor);
void dealloc_tilelist(TileList* tilelist);
void clear_tilelist(TileList* tilelist);
void save_tilelist(TileList* tilelist, const char* file_name);
#endif