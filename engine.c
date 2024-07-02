#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include "worldbuilder.h"

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
const char* HEAL_PROMPT = "PRESSING H WILL HEAL";

const int FPS = 60;
const int PLAYER_ID = 0;

const Animation ENTITY_ANIMATION = {3, 0, 15, 0, 0};

Vector2 mp;
World world;
Entity player;
Camera2D camera;
Status status = ALIVE;

Vector2 centerObjectPos(Vector2 pos) { return (Vector2){pos.x + TILE_SIZE, pos.y + TILE_SIZE}; }

Rectangle getObjectArea(Vector2 pos) { return (Rectangle){pos.x, pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}; }

float getAngle(Vector2 v1, Vector2 v2) 
{
	float angle = (atan2f((v2.y - v1.y), (v2.x - v1.x)) * RAD2DEG);
	
	if(angle > 360) angle -= 360;
	else if (angle < 0) angle += 360;

	return angle;
}

void StartTimer(Timer *timer, double lifetime) 
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool TimerDone(Timer timer) { return GetTime() - timer.startTime >= timer.lifeTime; } 

void resizeEntities(EntityList* el) 
{
	el->cap *= 2;
	el->entities = realloc(el->entities, el->cap);
}

void resizeLayer(TileList* tl) 
{
    tl->cap *= 2;
    tl->list = realloc(tl->list, tl->cap);
}

void addEntity(EntityList* el, Entity en) 
{
	if(el->size * sizeof(Entity) == el->cap) resizeEntities(el);
	el->entities[el->size++] = en;
}

void addTile(TileList* tl, Tile t) 
{
    if(tl->size * sizeof(Tile) == tl->cap) resizeLayer(tl);
    tl->list[tl->size++] = t;
}

void deleteTile(TileList* tl, int pos)
{
	if(tl->size == 0) return;
	for(int i = pos; i < tl->size; i++) tl->list[i] = tl->list[i + 1];
	tl->size--;
}

void deleteEntity(EntityList* el, int id)
{
	if(el->size == 0) return;
	else
	{
		int i = 0;
		for(; i < el->size; i++) if(el->entities[i].id == id) break;
		for(; i < el->size; i++) el->entities[i] = el->entities[i + 1];
		el->size--;
	}
}

Texture2D addTexture(Textures* textures, const char* FP)
{
	// returning non unique texture
	for(int i = 0; i < textures->size; i++) { if(strcmp(textures->better_textures[i].fp, FP) == 0) return textures->better_textures[i].tx; }

	// adding new texture
	textures->better_textures[textures->size] = (BetterTexture){LoadTexture(FP), ""};
	strcpy(textures->better_textures[textures->size].fp, FP);
	textures->size++;

	return textures->better_textures[textures->size - 1].tx;
}

void loadLayers(const char* FP)
{
    char line[CHAR_LIMIT];
    FILE* inFile = fopen(FP, "r");
    
	if(inFile == NULL) return;

    Vector2 src;
    Vector2 sp;

    TileType tt;
    char fp[CHAR_LIMIT];

	bool animated;
	int frames;
	int animation_speed;	

    while(fgets(line, sizeof(line), inFile))
    {
        src.x = atoi(strtok(line, ","));
        src.y = atoi(strtok(NULL, ","));
        sp.x = atoi(strtok(NULL, ","));
        sp.y = atoi(strtok(NULL, ","));

        tt = (TileType)atoi(strtok(NULL, ","));
		strcpy(fp, strtok(NULL, ","));

		animated = atoi(strtok(NULL, ","));
		frames = atoi(strtok(NULL, ","));
		animation_speed = atoi(strtok(NULL, "\n"));
        Tile tile = (Tile){src, sp, addTexture(&world.textures, fp), tt, "", animated, (Animation){frames, 0, animation_speed, 0, 0}};

        switch (tt)
		{
			case WALL: addTile(&world.walls, tile); break;
			case FLOOR: addTile(&world.floors, tile); break;
			case DOOR: addTile(&world.doors, tile); break;
			case HEALTH_BUFF: addTile(&world.health_buffs, tile); break;
			case DAMAGE_BUFF: addTile(&world.damage_buffs, tile); break;
			case INTERACTABLE: addTile(&world.interactables, tile); break;
			default:
				break;
		}
    }

    fclose(inFile);
}

void animate(Animation* animation)
{
	animation->count++;
	animation->xfp = animation->count / animation->speed;
	if(animation->xfp > animation->frames) animation->xfp = animation->count = 0;
}

void drawLayer(TileList* tl)
{	
    for(int i = 0; i < tl->size; i++)
    {
		// int frame_pos = 0;
		Tile* t = &tl->list[i];

		// animate tiles
		if(t->animated) animate(&t->animtaion);

		// draw tiles in screen		
		if(CheckCollisionPointRec(centerObjectPos(t->sp), world.area))
		{
			DrawTexturePro(t->tx, (Rectangle){t->src.x + (t->animtaion.xfp * TILE_SIZE), t->src.y, TILE_SIZE, TILE_SIZE}, 
										getObjectArea(t->sp), (Vector2){0,0}, 0, WHITE);
		}
    }
}

TileList initTileList() { return (TileList){0, TILE_CAP, malloc(TILE_CAP)}; }

EntityList initEntityList(){ return (EntityList){0, ENTITY_CAP, malloc(ENTITY_CAP)}; } 

void resetEntityList(EntityList* el) 
{
	if(el->size > 0) free(el->entities);
	el->entities = malloc(ENTITY_CAP);
	el->cap = ENTITY_CAP;
	el->size = 0;
}

Vector2 getSpawnPoint() 
{
	char info[CHAR_LIMIT];
	FILE* file = fopen(SPAWN_PATH, "r");
	
	if(file == NULL) return (Vector2){0, 0};
	else
	{
		fgets(info, sizeof(info), file);
		return (Vector2){atof(strtok(info, ",")), atof(strtok(NULL, "\n"))};
	}
}	

Entity createPlayer() { return (Entity){"player", 100.0f, 3.0f, getSpawnPoint(), false, false, addTexture(&world.textures, PLAYER_PATH), ENTITY_ANIMATION, PLAYER_ID, 25.0f}; } 

Entity createEnemy(){ return (Entity){"mob", 100, 1.50, mp, false, true, addTexture(&world.textures, PLAYER_PATH), ENTITY_ANIMATION, GetRandomValue(INT_MIN, INT_MAX), 10}; }

void init()
{
	// game settings
	SetTargetFPS(FPS);
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");
	
	world.textures.size = 0;

	// layer inititalization
	world.walls = initTileList();
	world.floors = initTileList();
	world.doors = initTileList();
	world.health_buffs = initTileList();
	world.damage_buffs = initTileList();
	world.interactables = initTileList();
	world.entities = initEntityList();
    loadLayers(WORLD_PATH);

	// player instantiation
	player = createPlayer();
	
	// camera setup
	camera.zoom = 1.0f;
	camera.target = player.pos;
	camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

void deinit() 
{
	// free alloc mem from dynamic lists
	free(world.walls.list);
	free(world.floors.list);
	free(world.doors.list);
	free(world.health_buffs.list);
	free(world.damage_buffs.list);
	free(world.interactables.list);
	free(world.entities.entities);

	// unload all textures
	for(int i = 0; i < world.textures.size; i++) UnloadTexture(world.textures.better_textures[i].tx);

	CloseWindow();
}

int collisionLayer(Rectangle en_area, TileList* layer) 
{
	for(int i = 0; i < layer->size; i++) if(CheckCollisionRecs(en_area, getObjectArea(layer->list[i].sp))) return i;
	return -1;
}

int collisionEntity(int en_id, Vector2 en_pos, EntityList* ents) 
{
	for(int i = 0; i < ents->size; i++)
	{
		// self collision
		if(en_id == ents->entities[i].id) continue;
		if(CheckCollisionPointRec(en_pos, getObjectArea(ents->entities[i].pos))) return i;
	}

	return -1;
}

void moveEntity(Entity* en)
{
	float x = cos(en->angle * DEG2RAD) * en->speed;
	float y = sin(en->angle * DEG2RAD) * en->speed;

	// move by components
	en->pos.x += x;
	if(collisionLayer(getObjectArea(en->pos), &world.walls) >= 0 || 
		collisionEntity(en->id, centerObjectPos(en->pos), &world.entities) >= 0) 
	{
		en->pos.x -= x;
	}
	
	en->pos.y += y;
	if(collisionLayer(getObjectArea(en->pos), &world.walls) >= 0 || 
		collisionEntity(en->id, centerObjectPos(en->pos), &world.entities) >= 0)
	{
		en->pos.y -= y;
	}
}

void displayStatistic(Vector2 pos, int width, int height, float f, Color c) 
{
	DrawRectangle(pos.x, pos.y, width * (f / 100), height, c);
	DrawRectangleLines(pos.x, pos.y, width, height, RAYWHITE);
}

void awardXP(Entity* player)
{
	player->exp += 20;
	if(player->exp >= 100) { player->exp = 0; player->level++; }
}

void dealDamage(Entity* target, Entity* en)
{
	if(CheckCollisionPointRec(centerObjectPos(en->pos), getObjectArea(target->pos)) && (TimerDone(en->attack_speed)))
	{
		target->health -= en->damage;
		if(target->health <= 0)
		{
			if(target->id == PLAYER_ID) status = DEAD;
			else
			{ 
				deleteEntity(&world.entities, target->id); 
				awardXP(en);
			}
		}
		StartTimer(&en->attack_speed, 1.0f);
	}
}

void updateEntities(EntityList* world_entities) 
{
	for(int i = 0; i < world_entities->size; i++)
	{
		Entity* en = world_entities->entities + i;

		// relative to player
		en->angle = getAngle(en->pos, player.pos);

		moveEntity(en);
		animate(&en->animation);
		dealDamage(&player, en);

		// sprite direction
		if((en->angle >= 45) && (en->angle <=  135)) en->animation.yfp = 0;
		else if((en->angle >= 135) && (en->angle <= 225)) en->animation.yfp = 3;
		else if((en->angle >= 225) && (en->angle <= 315)) en->animation.yfp = 1;
		else en->animation.yfp = 2;

		// only draw entity when in bounds
		if(CheckCollisionPointRec(centerObjectPos(en->pos), world.area))
		{
			// health bar
			displayStatistic((Vector2){en->pos.x + 5, en->pos.y - 7}, 20, 7, en->health, RED);
			DrawTexturePro(en->tx, (Rectangle){en->animation.xfp * TILE_SIZE, en->animation.yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										getObjectArea(en->pos), (Vector2){0,0}, 0, WHITE);
		}
	}
}

void movement(TileList* world_walls, Entity* player, EntityList* entities)
{
	player->move = (IsKeyDown(KEY_W) || 
					IsKeyDown(KEY_A) || 
					IsKeyDown(KEY_S) ||
					IsKeyDown(KEY_D));
	
	// afk case
	if(!player->move) player->animation.xfp = player->animation.yfp = 0;

    bool diagonal = (IsKeyDown(KEY_W) && IsKeyDown(KEY_D)) || 
                    (IsKeyDown(KEY_W) && IsKeyDown(KEY_A)) || 
					(IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) || 
					(IsKeyDown(KEY_S) && IsKeyDown(KEY_D));

	// using pythag thrm to prevent speedup when moving diagonally
    if (diagonal && !player->adjsp)
    {
		player->speed = (sqrt(8 * (pow(player->speed, 2))) / 4);
		player->adjsp = true;
    }

	if(!diagonal)
	{
        player->speed = 3.0f;
		player->adjsp = false;
    }

	// movement controls
	if(IsKeyDown(KEY_W))
	{
		player->animation.yfp = 1;
		player->pos.y -= player->speed;
		if(collisionLayer(getObjectArea(player->pos), world_walls) >= 0) player->pos.y += player->speed;
	}
	
	if(IsKeyDown(KEY_A))
	{
		player->animation.yfp = 3;
		player->pos.x -= player->speed;
		if(collisionLayer(getObjectArea(player->pos), world_walls) >= 0) player->pos.x += player->speed;
	}
	
	if(IsKeyDown(KEY_S))
	{
		player->animation.yfp = 0;
		player->pos.y += player->speed;
		if(collisionLayer(getObjectArea(player->pos), world_walls) >= 0) player->pos.y -= player->speed;
	}
	
	if(IsKeyDown(KEY_D))
	{
		player->pos.x += player->speed;
		player->animation.yfp = 2;
		if(collisionLayer(getObjectArea(player->pos), world_walls) >= 0) player->pos.x -= player->speed;
	}
}

void encounterHeals(Entity* player, TileList* heals)
{
	Vector2 heal_prompt_pos = GetScreenToWorld2D((Vector2){SCREEN_WIDTH - 310, SCREEN_HEIGHT - 110}, camera);
	int col = collisionLayer(getObjectArea(player->pos), heals);

	if(col < 0 ) return;

	DrawText(HEAL_PROMPT, heal_prompt_pos.x, heal_prompt_pos.y, 20, RAYWHITE);
	if(IsKeyDown(KEY_H) && player->health < 100 && TimerDone(player->heal_speed)) 
	{
		player->health += 20;
		if(player->health > 100) player->health = 100;
		deleteTile(heals, col);
		StartTimer(&player->heal_speed, 0.5f);
	}
}

void createMobs(World* world)
{
	if(!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

	// prevent spawing on walls and ontop of each other
	bool invalid_spawn = (collisionLayer(getObjectArea(mp), &world->walls) >= 0) || 
					(collisionEntity(0, centerObjectPos(mp), &world->entities) >= 0);
	
	if(!invalid_spawn) addEntity(&world->entities, createEnemy());
}

void showHealth(float health)
{
	Vector2 text_pos = (Vector2){SCREEN_WIDTH - 300, SCREEN_HEIGHT - 80};
	Vector2 heal_bar_pos = (Vector2){SCREEN_WIDTH - 310, SCREEN_HEIGHT - 60};

	DrawText("HEALTH", text_pos.x, text_pos.y, 20, RED);
	displayStatistic(heal_bar_pos, 300, 50, health, RED);					
}

void showLevels(int level, float exp)
{
	Vector2 text_pos = (Vector2){10, SCREEN_HEIGHT - 80};
	Vector2 level_bar_pos = (Vector2){10, SCREEN_HEIGHT - 60};
	
	char text[CHAR_LIMIT]; sprintf(text, "LEVEL: %d", level);
	DrawText(text, text_pos.x, text_pos.y, 20, GREEN);
	displayStatistic(level_bar_pos, 300, 50, exp, GREEN);
}

void updatePlayer(TileList* walls, TileList* heals, EntityList* enemies, Entity* player)
{
	animate(&player->animation);
	movement(walls, player, enemies);
	encounterHeals(player, &world.health_buffs);
	
	// player combat
	for(int i = 0; i < enemies->size; i++) dealDamage(&enemies->entities[i], player);

	DrawTexturePro(player->tx, (Rectangle){player->animation.xfp * TILE_SIZE, player->animation.yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
									getObjectArea(player->pos), (Vector2){0,0}, 0, WHITE);
} 

void deathScreen()
{
	ClearBackground(GRAY);
	DrawText(DEATH_PROMPT, (GetScreenWidth() / 2.0f) - (MeasureText(DEATH_PROMPT, 30) / 2.0f),GetScreenHeight() / 2.0f, 30, RED);
	
	// respawn btn
	if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) - 125, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "RESPAWN"))
	{
		status = ALIVE;
		player = createPlayer();
		resetEntityList(&world.entities);
	}

	// quit btn
	if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) + 50, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "QUIT")) status = QUIT;
}

int main()
{
	init();
	while((status != QUIT) && (!WindowShouldClose()))
    {
		camera.target = player.pos;
		mp = GetScreenToWorld2D(GetMousePosition(), camera);
		world.area = (Rectangle){camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, SCREEN_WIDTH, SCREEN_HEIGHT};
		BeginDrawing();
			switch(status)
			{
				case ALIVE:
					ClearBackground(BLACK);
					BeginMode2D(camera);
						// drawing world 	
						drawLayer(&world.floors);
						drawLayer(&world.health_buffs);
						drawLayer(&world.damage_buffs);
						drawLayer(&world.doors);
						drawLayer(&world.interactables);
						drawLayer(&world.walls);
						// updating entities
						updateEntities(&world.entities);
						updatePlayer(&world.walls, &world.health_buffs, &world.entities, &player);
					EndMode2D();
					// game mechanics
					createMobs(&world);
					showHealth(player.health);
					showLevels(player.level, player.exp);
					break;
				case DEAD: deathScreen(); break;
				default:
					break;
			}
		DrawFPS(0, 0);
		EndDrawing();
    }
    deinit();
    return 0;
}

// add damage buff door mechanic and interactable?