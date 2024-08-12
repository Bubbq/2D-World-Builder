#include <stdio.h>

#include "headers/animation.h"
#include "headers/raylib.h"
#include "headers/tile.h"

void resize_tilelist(TileList* tilelist) 
{
    tilelist->cap *= 2;
    tilelist->list = realloc(tilelist->list, tilelist->cap);
}

void add_tile(TileList* tilelist, Tile tile) 
{
    if(tilelist->size * sizeof(Tile) == tilelist->cap) 
        resize_tilelist(tilelist);
    
    tilelist->list[tilelist->size++] = tile;
}

void delete_tile(TileList* tl, int pos)
{
	if(tl->size == 0) 
        return;
	
    for(int i = pos; i < tl->size; i++) 
        tl->list[i] = tl->list[i + 1];
	
    tl->size--;
}

TileList create_tilelist()
{ 
    TileList tilelist;

    tilelist.size = 0;
    tilelist.cap = sizeof(Tile);
    tilelist.list = malloc(tilelist.cap);

    return tilelist;
}

int check_collision_tilelist(Rectangle object_area, TileList* tilelist, int tile_size) 
{
	for(int i = 0; i < tilelist->size; i++)
    {
        Rectangle tile_area = (Rectangle){ tilelist->list[i].screen_position.x, tilelist->list[i].screen_position.y, tile_size, tile_size };
        
        if(CheckCollisionRecs(object_area, tile_area)) 
            return i;
    } 
	
    return -1;
}

void animate_tilelist(TileList *tilelist)
{
    for(int i = 0; i < tilelist->size; i++)
    {
        if(tilelist->list[i].is_animated)
            animate(&tilelist->list[i].animtaion);
    }
}

void draw_tilelist(TileList* tilelist, Rectangle world_border, int tile_size, int sprite_sheet_size)
{	
    for(int i = 0; i < tilelist->size; i++)
    {
		Tile* tile = &tilelist->list[i];

		if(CheckCollisionPointRec((Vector2){ tile->screen_position.x + (tile_size / 2.0), tile->screen_position.y + (tile_size / 2.0) }, world_border))
			DrawTexturePro(tile->sprite, (Rectangle){tile->position_in_sprite.x + (tile->animtaion.xfposition * sprite_sheet_size), tile->position_in_sprite.y, sprite_sheet_size, sprite_sheet_size}, (Rectangle){ tile->screen_position.x, tile->screen_position.y, tile_size, tile_size}, (Vector2){}, 0, WHITE);
    }
}

void label_tilelist(TileList* tilelist, Rectangle world_border, Color text_color, int tile_size, const char* tile_descriptor)
{
    for(int i = 0; i < tilelist->size; i++)
    {
        Tile* tile = tilelist->list + i;

        if(CheckCollisionPointRec(tile->screen_position, world_border))
            DrawText(tile_descriptor, tile->screen_position.x, tile->screen_position.y, 3, text_color);
    }
}

void dealloc_tilelist(TileList* tilelist)
{
    free(tilelist->list);
}

void clear_tilelist(TileList* tilelist)
{
    while(tilelist->size != 0)
        delete_tile(tilelist, 0);
}

void save_tilelist(TileList* tilelist, const char* file_name)
{
    FILE* file = fopen(file_name, "a");
    if(file == NULL)
    { 
        printf("error saving layer\n"); 
        return; 
    }

    for(int i = 0; i < tilelist->size; i++)
    {
        Tile tile = tilelist->list[i];
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,%d,%s,%d,%d,%d\n", tile.position_in_sprite.x, tile.position_in_sprite.y, tile.screen_position.x, tile.screen_position.y, tile.tile_type, tile.sprite_file_path, tile.is_animated, tile.animtaion.nframes, tile.animtaion.fspeed); 
    }

    fclose(file);
}