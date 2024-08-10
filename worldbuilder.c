#include "headers/worldbuilder.h"
#include "headers/entity.h"
#include "headers/tile.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

void unload_textures(Textures* textures)
{
	for(int i = 0; i < textures->size; i++) 
		UnloadTexture(textures->better_textures[i].tx);
}

World create_world()
{
	World world;

	world.textures.size = 0;
	world.walls = create_tilelist();
	world.floors = create_tilelist();
	world.doors = create_tilelist();
	world.health_buffs = create_tilelist();
	world.damage_buffs = create_tilelist();
	world.interactables = create_tilelist();
	world.entities = create_entitylist();

	return world;
}

void clear_world(World* world)
{
	world->spawn = (Vector2){};
    clear_tilelist(&world->walls);
	clear_tilelist(&world->doors);
	clear_tilelist(&world->floors);
	clear_tilelist(&world->health_buffs);
	clear_tilelist(&world->damage_buffs);
	clear_tilelist(&world->interactables);
}

void dealloc_world(World* world)
{
	dealloc_tilelist(&world->walls);
	dealloc_tilelist(&world->floors);
	dealloc_tilelist(&world->doors);
	dealloc_tilelist(&world->health_buffs);
	dealloc_tilelist(&world->damage_buffs);
	dealloc_tilelist(&world->interactables);
	dealloc_entitylist(&world->entities);

	unload_textures(&world->textures);
}

void load_world(World* world, const char* file_path)
{
    char line[CHAR_LIMIT];
    FILE* file = fopen(file_path, "r");
    
	if(file == NULL) 
	{
		printf("error loading world\n");
		return;
	}

    while(fgets(line, sizeof(line), file))
    {
		Tile tile;

		tile.src.x = atoi(strtok(line, ","));
		tile.src.y = atoi(strtok(NULL, ","));

		tile.sp.x = atoi(strtok(NULL, ","));
        tile.sp.y = atoi(strtok(NULL, ","));

		tile.tt = (TileType)atoi(strtok(NULL, ","));

		strcpy(tile.fp, strtok(NULL, ","));
		tile.tx = add_texture(&world->textures, tile.fp);

		tile.animated = atoi(strtok(NULL, ","));
		tile.animtaion.nframes = atoi(strtok(NULL, ","));
		tile.animtaion.fspeed = atoi(strtok(NULL, "\n"));
		tile.animtaion.xfposition = tile.animtaion.yfposition = 0;

        switch (tile.tt)
		{
			case WALL: add_tile(&world->walls, tile); break;
			case FLOOR: add_tile(&world->floors, tile); break;
			case DOOR: add_tile(&world->doors, tile); break;
			case HEALTH_BUFF: add_tile(&world->health_buffs, tile); break;
			case DAMAGE_BUFF: add_tile(&world->damage_buffs, tile); break;
			case INTERACTABLE: add_tile(&world->interactables, tile); break;
			
			default: break;
		}
    }

    fclose(file);
}

Vector2 get_spawn_point(const char* spawn_path)
{
	char info[CHAR_LIMIT];
	FILE* file = fopen(spawn_path, "r");
	
	if(file == NULL) 
		return (Vector2){};	
	
	else
	{
		fgets(info, sizeof(info), file);
		return (Vector2){ atof(strtok(info, ",")), atof(strtok(NULL, "\n")) };
	}
}	

Texture2D add_texture(Textures* textures, const char* file_path)
{
	// returning non unique texture
	for(int i = 0; i < textures->size; i++)
		if(strcmp(textures->better_textures[i].fp, file_path) == 0)
			return textures->better_textures[i].tx;

	// adding new texture
	textures->better_textures[textures->size] = (BetterTexture){LoadTexture(file_path), ""};
	strcpy(textures->better_textures[textures->size].fp, file_path);
	textures->size++;

	return textures->better_textures[textures->size - 1].tx;
}

Rectangle get_object_area(Vector2 position, int tile_size)
{
	return (Rectangle){ position.x, position.y, tile_size, tile_size };
}

Vector2 get_object_center(Vector2 position)
{
	return (Vector2){ (position.x + 16), (position.y + 16) };
}