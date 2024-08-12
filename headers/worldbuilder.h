#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"
#include "animation.h"
#include "entity.h"
#include "tile.h"

#define CHAR_LIMIT 1024

typedef enum
{
	EDIT = 0,
	SPAWN = 1,
	FREE = 2,
} EditState;

typedef enum 
{
	QUIT = -1,
	ALIVE = 0, 
	DEAD = 1, 
} Status;

typedef struct
{
	Texture2D tx;
	char fp[CHAR_LIMIT];
} BetterTexture;

typedef struct
{
	BetterTexture better_textures[25];
	int size;
} Textures;

void unload_textures(Textures* textures);

typedef struct
{
	TileList walls;
	TileList doors;
	TileList floors;
	TileList health_buffs;
	TileList damage_buffs;
	TileList interactables;
	EntityList entities;
	Textures textures;
	Rectangle area;
	Vector2 spawn;
} World;

World create_world();
void clear_world(World* world);
void dealloc_world(World* world);
void load_world(World* world, const char* file_path);
void draw_world(World* world, int tile_size, int sprite_sheet_size);
void animate_world(World* world);
Vector2 get_spawn_point(const char* spawn_path);
Texture2D add_texture(Textures* textures, const char* file_path);
Rectangle get_object_area(Vector2 position, int tile_size);
Vector2 get_object_center(Vector2 position);
#endif