#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "2d-tile-editor.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#undef RAYGUI_IMPLEMENTATION

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;

// update path to world you have previously saved
const char* WORLD_PATH = "test.txt";
const char* SPAWN_PATH = "spawn.txt";
const char* PLAYER_PATH = "Assets/player.png";
const char* DEATH_PROMPT = "YOU DIED, RESPAWN?"; 

const int ANIMATION_SPEED = 20;
const int FPS = 60;

void StartTimer(Timer *timer, double lifetime)
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool TimerDone(Timer timer)
{
	return GetTime() - timer.startTime >= timer.lifeTime;
}

void clearEntities(Entities* world_entities)
{
	free(world_entities->entities);
	world_entities->entities = malloc(ENTITY_CAP);
	world_entities->cap = ENTITY_CAP;
	world_entities->size = 0;
}

void resizeEntities(Entities* world_entities)
{
	world_entities->cap *= 2;
	world_entities->entities = realloc(world_entities->entities, world_entities->cap);
	
	if(world_entities->entities == NULL)
	{
		printf("ERROR RESZING ENTITIES \n");
		exit(1);
	}
}

void resizeLayer(TileList* layer)
{
    layer->cap *= 2;
    layer->list = realloc(layer->list, layer->cap);
    
    if(layer == NULL)
    {
        printf("ERROR RESIZING \n");
        exit(1);
    }
}

void addEntity(Entities* world_entities, Entity entity)
{
	if(world_entities->size * sizeof(Entity) == world_entities->cap)
	{
		resizeEntities(world_entities);
	}

	world_entities->entities[world_entities->size++] = entity;
}

void addTile(TileList* layer, Tile tile)
{
    if(layer->size * sizeof(Tile) == layer->cap)
    {
        resizeLayer(layer);
    }

    layer->list[layer->size++] = tile;
}

// either adds a new texture to list of textures or returns the existing texture of the one we want
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

void loadLayers(World* world, const char* filePath)
{
    FILE* inFile = fopen(filePath, "r");
    char line[512];
    if(inFile == NULL)
    {
        printf("ERROR LOADING WORLD \n");
        return;
    }

    Vector2 src;
    Vector2 sp;
    enum Element tt;
    char* fp;
	bool anim;
	int fc;
	int frames;
	int anim_speed;

    while(fgets(line, sizeof(line), inFile))
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

        Tile tile = (Tile){src, sp, addTexture(&world->textures, fp), tt, "NULL",true, anim, fc, frames, anim_speed};
        strcpy(tile.fp, fp);

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
}

void drawLayer(TileList* layer)
{
    for(int i = 0; i < layer->size; i++)
    {
		Tile* tile = &layer->list[i];

        if(tile->tx.id > 0 && tile->active)
        {
			int frame_pos = 0;

			// animate tiles
			if(tile->anim)
			{
				tile->fc++;
				frame_pos = tile->fc / tile->anim_speed;

				if(frame_pos > tile->frames)
				{
					frame_pos = 0;
					tile->fc = 0;
				}
			}

            DrawTexturePro(tile->tx, (Rectangle){tile->src.x + (frame_pos * TILE_SIZE), tile->src.y, TILE_SIZE, TILE_SIZE},
													 (Rectangle){tile->sp.x, tile->sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
        }
    }
}

void init(World* world, Entity* player)
{
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");
	SetTargetFPS(FPS);

	world->walls.size = 0;
	world->floors.size = 0;
	world->doors.size = 0;
	world->health_buffs.size = 0;
	world->damage_buffs.size = 0;
	world->interactables.size = 0;
	world->entities.size = 0;
	world->textures.size = 0;

	world->entities.entities = malloc(ENTITY_CAP);
	world->walls.list = malloc(TILE_CAP);
	world->floors.list = malloc(TILE_CAP);
	world->doors.list = malloc(TILE_CAP);
	world->health_buffs.list = malloc(TILE_CAP);
	world->damage_buffs.list = malloc(TILE_CAP);
	world->interactables.list = malloc(TILE_CAP);

	world->entities.cap = ENTITY_CAP;
    world->interactables.cap = TILE_CAP;
    world->walls.cap = TILE_CAP;
    world->floors.cap = TILE_CAP;
    world->doors.cap = TILE_CAP;
    world->health_buffs.cap = TILE_CAP;
    world->damage_buffs.cap = TILE_CAP;

    loadLayers(world, WORLD_PATH);
	
	// getting spawn point
	FILE* inFile = fopen(SPAWN_PATH, "r");

	if(inFile == NULL)
	{
		printf("NO SPAWN POINT SAVED \n");
		world->spawn = (Vector2){0, 0};
	}

	else
	{
		char line[512];
		while(fgets(line, sizeof(line), inFile))
		{
			world->spawn.x = atoi(strtok(line, " "));
			world->spawn.y = atoi(strtok(NULL, "\n"));
		}

		fclose(inFile);
	}

	player->name = "player";
	player->frame_count = 0;
	player->health = 100;
	player->speed = 5;
	player->anim_speed = ANIMATION_SPEED;
	player->pos = (Vector2){world->spawn.x, world->spawn.y};
	player->alive = true;
	player->adjsp = false;
	player->move = false;
	player->tx = addTexture(&world->textures, PLAYER_PATH);
	
	player->camera.target = player->pos;
	player->camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	player->camera.zoom = 1.0f;
}

void deinit(World *world)
{
	free(world->walls.list);
	free(world->floors.list);
	free(world->doors.list);
	free(world->health_buffs.list);
	free(world->damage_buffs.list);
	free(world->interactables.list);
	free(world->entities.entities);

	for(int i = 0; i < world->textures.size; i++)
	{
		UnloadTexture(world->textures.better_textures[i].tx);
	}

	CloseWindow();
}

// returns the index of the tile of a layer you collided with
int entityCollisionWorld(Entity* en, TileList* layer)
{
	for(int i = 0; i < layer->size; i++)
	{
		if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
							(Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && layer->list[i].active)
		{
			return i;
		}
	}

	return -1;
}

// returns index of entity that an entity 'en' has collided with
int entityCollisionEntity(Entity* en, Entities* ents)
{
	for(int i = 0; i < ents->size; i++)
	{
		if(CheckCollisionPointRec((Vector2){en->pos.x + TILE_SIZE, en->pos.y + TILE_SIZE},
							(Rectangle){ents->entities[i].pos.x, ents->entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && ents->entities[i].id != en->id)
		{
			return i;
		}
	}

	return -1;
}

void updateEntities(Entities* world_entities, Entity* player, World* world)
{
	int wall_col, en_col = -1;

	for(int i = 0; i < world_entities->size; i++)
	{
		Entity* en = &world_entities->entities[i];
		
		if(!en->alive)
		{
			continue;
		}

		en->frame_count++;
		en->xfp = (en->frame_count / en->anim_speed);
		if(en->xfp > 3)
		{
			en->frame_count = 0;
			en->xfp = 0;
		}

		// find entities angle 
		float dx = player->pos.x - en->pos.x;
		float dy = player->pos.y - en->pos.y;

		// getting angular speed
		en->dx = cosf(en->angle * DEG2RAD) * en->speed;
		en->dy = sinf(en->angle * DEG2RAD) * en->speed;

		// make it nearest number divisible by 32
		en->angle = atan2f(dy, dx) * RAD2DEG;

		// angle correction
		if(en->angle > 360)
		{
			en->angle -= 360;
		}

		if(en->angle < 0)
		{
			en->angle += 360;
		}

		// setting sprite direction based on angle
		if(en->angle > 45 && en->angle < 135)
		{
			en->yfp = 0;
		}
		
		else if(en->angle > 225 && en->angle < 315)
		{
			en->yfp = 1;
		}

		else if(en->angle > 135 && en->angle < 215)
		{
			en->yfp = 3;
		}

		else
		{
			en->yfp = 2;
		}

		// collisions between walls and other enitites
		en->pos.x += en->dx;
		wall_col = entityCollisionWorld(en, &world->walls);
		en_col = entityCollisionEntity(en, &world->entities);

		if(wall_col >= 0 || en_col >= 0)
		{
			en->pos.x -= en->dx;
		}

		en->pos.y += en->dy;
		wall_col = entityCollisionWorld(en, &world->walls);
		en_col = entityCollisionEntity(en, &world->entities);

		if(wall_col >= 0 || en_col >= 0)
		{
			en->pos.y -= en->dy;
		}

		// displaying health bar
		DrawRectangle(en->pos.x + 5, en->pos.y - 7, 20 * (en->health / 100), 7, RED);
		DrawRectangleLines(en->pos.x + 5, en->pos.y - 7, 20, 7, BLACK);

		// only draw entity when in bounds
		if(CheckCollisionPointRec((Vector2){en->pos.x + SCREEN_TILE_SIZE, en->pos.y + SCREEN_TILE_SIZE}, world->area))
		{
			DrawTexturePro(en->tx, (Rectangle){en->xfp * TILE_SIZE, en->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										(Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
		}
	}
}

void move(TileList* world_walls, Entity* player)
{
	// flag for diagonal movement
    bool diagonal = (IsKeyDown(KEY_W) && IsKeyDown(KEY_D)) || 
                      (IsKeyDown(KEY_W) && IsKeyDown(KEY_A)) || 
                      	(IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) || 
                      		(IsKeyDown(KEY_S) && IsKeyDown(KEY_D));

    if (diagonal)
    {
        if (!player->adjsp)
        {
			// through pythag thm: 2x^2 - player's speed^2 = 0, solve for x to find diag speed movement to prevent moving faster diagonly
            player->speed = sqrt(8 * (pow(player->speed, 2))) / 4;
			player->adjsp = true;
        }
    } else {
        player->speed = 3;
		player->adjsp = false;
    }
	
	int wall_col = -1;

	if(IsKeyDown(KEY_W))
	{
		player->pos.y -= player->speed;
		player->yfp = 1;
		player->move = true;

		wall_col = entityCollisionWorld(player, world_walls);
		if(wall_col >= 0)
		{
			player->pos.y += player->speed;
		}
	}

	if(IsKeyDown(KEY_A))
	{
		player->pos.x -= player->speed;
		player->yfp = 3;
		player->move = true;

		wall_col = entityCollisionWorld(player, world_walls);
		if(wall_col >= 0)
		{
			player->pos.x += player->speed;
		}
	}

	if(IsKeyDown(KEY_S))
	{
		player->pos.y += player->speed;
		player->yfp = 0;
		player->move = true;

		wall_col = entityCollisionWorld(player, world_walls);
		if(wall_col >= 0)
		{
			player->pos.y -= player->speed;
		}
	}

	if(IsKeyDown(KEY_D))
	{
		player->pos.x += player->speed;
		player->yfp = 2;
		player->move = true;

		wall_col = entityCollisionWorld(player, world_walls);
		if(wall_col >= 0)
		{
			player->pos.x -= player->speed;
		}
	}

	// afk
	if(!player->move)
	{
		player->xfp = player->yfp = 0;
	}

	// resetting movement flag
	player->move = false;
}

void heal(TileList* world_heals, Entity* player)
{
	int h_col =  entityCollisionWorld(player, world_heals);
	if(h_col >= 0)
	{
		DrawText("PRESSING H WILL HEAL", GetScreenWidth() - 310, GetScreenHeight() - 110, 20, GREEN);
		if(IsKeyPressed(KEY_H) && player->health < 100)
		{
			player->health = player->health + 20 > 100 ? 100 : player->health + 20;
			world_heals->list[h_col].active = false;
		}
	}

	// drawing the player's health bar
	DrawRectangle(GetScreenWidth() - 310, GetScreenHeight() - 60, (300) * (player->health / 100),50, RED);
	DrawRectangleLines(GetScreenWidth() - 310, GetScreenHeight() - 60, 300,50, BLACK);
	DrawText("HEALTH", GetScreenWidth() - 300, GetScreenHeight() - 80, 20, RED);
}

void fight(Entity* player, Entities* enemies)
{
		int mob_col = entityCollisionEntity(player, enemies);
		if(mob_col >= 0)
		{
			Entity* fight_en = &enemies->entities[mob_col];
			
			// small window of invincibility after getting hit
			if(TimerDone(player->timer))
			{
				StartTimer(&player->timer, 1.5);
				player->health = (player->health - 20 < 0) ? 0 : player->health - 20;
				
				// when you die
				if(player->health == 0)
				{
					player->alive = false;
				}
			}

			// doing damage to mob
			if(IsKeyPressed(KEY_E))
			{
				fight_en->health = fight_en->health - 20 > 0 ? fight_en->health - 20 : 0;
				
				// killing an enemy
				if(fight_en->health == 0)
				{
					fight_en->alive = false;
					fight_en->pos = (Vector2){-1000000, -1000000};
					
					// gaining experience/leveling up
					player->exp += 20;
					if(player->exp >= 100)
					{
						player->exp = 0;
						player->level++;
					}
				}
			}
		}
		
		// drawing and updating players level
		DrawRectangle(10, GetScreenHeight() - 60, (300) * (player->exp / 100), 50, GREEN);
		DrawRectangleLines(10, GetScreenHeight() - 60, 300, 50, BLACK);
		
		char lvl_dsc[100]; sprintf(lvl_dsc, "LEVEL: %d", player->level);
		DrawText(lvl_dsc, 10, GetScreenHeight() - 80, 20, GREEN);
}

void mobCreation(Vector2 mp, World* world)
{
	if(CheckCollisionPointRec(mp, world->area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		// cant spawn mob in wall
		bool valid_spawn = true;
		for(int i = 0; i < world->walls.size; i++)
		{
			if(CheckCollisionRecs((Rectangle){mp.x, mp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
								(Rectangle){world->walls.list[i].sp.x, world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
			{
				valid_spawn = false;
			}
		}

		// cant spawn mobs on top of each other
		for(int i = 0; i < world->entities.size; i++)
		{
			if(CheckCollisionRecs((Rectangle){mp.x, mp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
								(Rectangle){world->entities.entities[i].pos.x, world->entities.entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
			{
				valid_spawn = false;
			}
		}

		if(valid_spawn)
		{
			Entity new_en;
			new_en.name = "mob";
			new_en.health = 100;
			new_en.speed = 1;
			new_en.pos = mp;
			new_en.alive = true;
			new_en.anim_speed = ANIMATION_SPEED;
			new_en.tx = addTexture(&world->textures, PLAYER_PATH);
			new_en.id = GetRandomValue(0, 1000000000);
			addEntity(&world->entities, new_en);
		}
	}
}

void updatePlayer(TileList* world_walls, Entity* player)
{
	// animation
	player->frame_count++;
	player->xfp = (player->frame_count / player->anim_speed);
	if(player->xfp > 3)
	{
		player->xfp = 0;
		player->frame_count = 0;
	}
	
	move(world_walls, player);

	// drawing player
	DrawTexturePro(player->tx, (Rectangle){player->xfp * TILE_SIZE, player->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										(Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
}

int main()
{
	Vector2 mp;
    World world;
	Entity player;
	init(&world, &player);

	while(!WindowShouldClose())
    {
		mp = GetScreenToWorld2D(GetMousePosition(), player.camera);
		player.camera.target = player.pos;
		world.area = (Rectangle){player.camera.target.x - player.camera.offset.x, player.camera.target.y - player.camera.offset.y,SCREEN_WIDTH,SCREEN_HEIGHT};
	   
		mobCreation(mp, &world);
		heal(&world.health_buffs, &player);
		fight(&player, &world.entities);

	    BeginDrawing();

			if(player.alive)
			{
				ClearBackground(BLACK);
				BeginMode2D(player.camera);
					drawLayer(&world.floors);
					drawLayer(&world.health_buffs);
					drawLayer(&world.damage_buffs);
					drawLayer(&world.doors);
					drawLayer(&world.interactables);
					drawLayer(&world.walls);
					updatePlayer(&world.walls, &player);
					updateEntities(&world.entities, &player, &world);
				EndMode2D();
			}

			// death screen
			else
			{
				ClearBackground(GRAY);

				DrawText(DEATH_PROMPT, (GetScreenWidth() / 2.0f) - (MeasureText(DEATH_PROMPT, 30) / 2.0f),
							 GetScreenHeight() / 2.0f, 30, RED);

				if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) - 125, 
								(GetScreenHeight() / 2.0f) + 50, 100, 50}, "RESPAWN"))
				{
					clearEntities(&world.entities);
					player.pos = world.spawn;
					player.alive = true;
					player.health = 100;
					player.level = 0;
				}

				if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) + 50,
							(GetScreenHeight() / 2.0f) + 50, 100, 50}, "QUIT"))
				{
					break;
				}
			}

		DrawFPS(0, 0);
		EndDrawing();
    }

    deinit(&world);
    return 0;
}