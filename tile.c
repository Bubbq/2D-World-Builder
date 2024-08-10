#include <stdio.h>
#include <stdlib.h>
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

int check_collision_tilelist(Rectangle object_area, TileList* tilelist) 
{
	for(int i = 0; i < tilelist->size; i++)
    {
        Rectangle tile_area = (Rectangle){ tilelist->list[i].sp.x, tilelist->list[i].sp.y, 32, 32 };
        
        if(CheckCollisionRecs(object_area, tile_area)) 
            return i;
    } 
	
    return -1;
}

void draw_tilelist(TileList* tilelist, Rectangle world_border, Color text_color, int tile_size, const char* tile_desciptor)
{	
    for(int i = 0; i < tilelist->size; i++)
    {
		Tile* tile = &tilelist->list[i];

		if(tile->animated) 
            animate(&tile->animtaion);

		if(CheckCollisionPointRec((Vector2){ tile->sp.x + (tile_size / 2.0), tile->sp.y + (tile_size / 2.0) }, world_border))
        {
			DrawTexturePro(tile->tx, (Rectangle){tile->src.x + (tile->animtaion.xfposition * 16), tile->src.y, 16, 16}, (Rectangle){ tile->sp.x, tile->sp.y, tile_size, tile_size}, (Vector2){0,0}, 0, WHITE);
            DrawText(tile_desciptor, tile->sp.x, tile->sp.y, 3, text_color);
        }
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
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,%d,%s,%d,%d,%d\n", tile.src.x, tile.src.y, tile.sp.x, tile.sp.y, tile.tt, tile.fp, tile.animated, tile.animtaion.nframes, tile.animtaion.fspeed); 
    }

    fclose(file);
}