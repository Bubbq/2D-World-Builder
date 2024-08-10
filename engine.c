#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#include "headers/worldbuilder.h"
#include "headers/raylib.h"
#include <raymath.h>


#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"
#undef RAYGUI_IMPLEMENTATION

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;

// update path to world you have previously saved
const char* WORLD_PATH = "test.txt";
const char* SPAWN_PATH = "spawn.txt";
const char* PLAYER_PATH = "Assets/player.png";

const int TILE_SIZE = 16;
const int SCREEN_TILE_SIZE = 32;

const int FPS = 60;
const Animation ENTITY_ANIMATION = {3, 0, 15, 0, 0};

Vector2 mp;
World world;
Entity player;
Camera2D camera;
Status status = ALIVE;

Entity create_player()
{ 
	Entity player;

	player.name = "PLAYER";
	player.health = 100;
	player.speed = 3;
	player.pos = get_spawn_point(SPAWN_PATH);
	player.tx = add_texture(&world.textures, PLAYER_PATH);
	player.animation = ENTITY_ANIMATION;
	player.damage = 25;
	player.level = 1;

	return player;
} 

Entity create_enemy()
{ 
	Entity enemy;

	enemy.name = "ENEMY";
	enemy.health = 100;
	enemy.speed = 2;
	enemy.pos = mp;
	enemy.tx = add_texture(&world.textures, PLAYER_PATH);
	enemy.animation = ENTITY_ANIMATION;
	enemy.id = GetRandomValue(-1000, 1000);
	enemy.damage = 10;
	enemy.level = 1;

	return enemy;
}

void init()
{
	SetTargetFPS(FPS);
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");
	
	world = create_world();
	player = create_player();
	
	camera.zoom = 1.0f;
	camera.target = player.pos;
	camera.offset = (Vector2){ (GetScreenWidth() / 2.0), (GetScreenHeight() / 2.0) };
    
	load_world(&world, WORLD_PATH);
}

void move_enemy_entity(Entity* en)
{
	Vector2 delta = Vector2Scale(en->direction, en->speed);

	en->pos.x += delta.x;
	if(check_collision_tilelist(get_object_area(en->pos, SCREEN_TILE_SIZE), &world.walls) >= 0 || check_collision_entity(en->id, get_object_center(en->pos), &world.entities) >= 0) 
		en->pos.x -= delta.x;
	
	en->pos.y += delta.y;
	if(check_collision_tilelist(get_object_area(en->pos, SCREEN_TILE_SIZE), &world.walls) >= 0 || check_collision_entity(en->id, get_object_center(en->pos), &world.entities) >= 0) 
		en->pos.y -= delta.y;
}

void set_enemy_entity_direction(Entity* enemy)
{
	float angle = Vector2LineAngle(enemy->pos, player.pos) * RAD2DEG;

	if(angle < 0)
		angle += 360;

	if((angle >= 45) && (angle <=  135)) 
			enemy->animation.yfposition = 1;
		
	else if((angle >= 135) && (angle <= 225))
		enemy->animation.yfposition = 3;
	
	else if((angle >= 225) && (angle <= 315))
		enemy->animation.yfposition = 0;
	
	else enemy->animation.yfposition = 2;
	
	enemy->direction = Vector2Normalize(Vector2Subtract(player.pos, enemy->pos));
}

void player_movement(TileList* world_walls, Entity* player, EntityList* entities)
{
	player->move = (IsKeyDown(KEY_W) || 
					IsKeyDown(KEY_A) || 
					IsKeyDown(KEY_S) ||
					IsKeyDown(KEY_D));
	
    bool diagonal = (IsKeyDown(KEY_W) && IsKeyDown(KEY_D)) || 
                    (IsKeyDown(KEY_W) && IsKeyDown(KEY_A)) || 
					(IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) || 
					(IsKeyDown(KEY_S) && IsKeyDown(KEY_D));
	
	// afk 
	if(!player->move) 
		player->animation.xfposition = player->animation.yfposition = 0;

	// prevents diagonal speedup
    if (diagonal && !player->adjsp)
    {
		player->speed /= sqrtf(2);
		player->adjsp = true;
    }
	
	else if(!diagonal)
	{
        player->speed = 3.0f;
		player->adjsp = false;
    }

	// movement controls
	if(IsKeyDown(KEY_W))
	{
		player->animation.yfposition = 1;
		player->pos.y -= player->speed;
		
		if(check_collision_tilelist(get_object_area(player->pos, SCREEN_TILE_SIZE), world_walls) >= 0) 
			player->pos.y += player->speed;
	}
	
	if(IsKeyDown(KEY_A))
	{
		player->animation.yfposition = 3;
		player->pos.x -= player->speed;

		if(check_collision_tilelist(get_object_area(player->pos, SCREEN_TILE_SIZE), world_walls) >= 0) 
			player->pos.x += player->speed;
	}
	
	if(IsKeyDown(KEY_S))
	{
		player->animation.yfposition = 0;
		player->pos.y += player->speed;

		if(check_collision_tilelist(get_object_area(player->pos, SCREEN_TILE_SIZE), world_walls) >= 0) 
			player->pos.y -= player->speed;
	}
	
	if(IsKeyDown(KEY_D))
	{
		player->animation.yfposition = 2;
		player->pos.x += player->speed;

		if(check_collision_tilelist(get_object_area(player->pos, SCREEN_TILE_SIZE), world_walls) >= 0) 
			player->pos.x -= player->speed;
	}
}

void encounter_heals(Entity* player, TileList* heals)
{
	const char* HEAL_PROMPT = "PRESSING H WILL HEAL";
	
	int collided_health_buff_index;
	Vector2 heal_prompt_pos = GetScreenToWorld2D((Vector2){SCREEN_WIDTH - 310, SCREEN_HEIGHT - 110}, camera);

	if( (collided_health_buff_index = check_collision_tilelist(get_object_area(player->pos, SCREEN_TILE_SIZE), heals)) != -1)
	{
		DrawText(HEAL_PROMPT, heal_prompt_pos.x, heal_prompt_pos.y, 20, RAYWHITE);
		if((IsKeyPressed(KEY_H)) && (player->health < 100) && (is_timer_done(player->heal_speed))) 
		{
			player->health += 20;

			if(player->health > 100) 
				player->health = 100;

			delete_tile(heals, collided_health_buff_index);
			start_timer(&player->heal_speed, 0.5f);
		}
	}
}

void update_entities(EntityList* world_entities) 
{
	for(int i = 0; i < world_entities->size; i++)
	{
		Entity* en = (world_entities->entities + i);
		
		if(en->health <= 0)
			continue;

		set_enemy_entity_direction(en);
		move_enemy_entity(en);
		animate(&en->animation);
		deal_damage(en, &player);
	}
}

void update_player(TileList* walls, TileList* heals, EntityList* enemies, Entity* player)
{
	animate(&player->animation);
	player_movement(walls, player, enemies);
	encounter_heals(player, &world.health_buffs);
	
	// combat
	for(int i = 0; i < enemies->size; i++) 
		if(enemies->entities[i].health > 0)
			deal_damage(player, &enemies->entities[i]);
} 

void create_mobs(World* world)
{
	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		if((check_collision_tilelist(get_object_area(mp, SCREEN_TILE_SIZE), &world->walls) == -1) && (check_collision_entity(0, get_object_center(mp), &world->entities) == -1))
			add_entity(&world->entities, create_enemy());
}

void display_player_health(float health)
{
	DrawText("HEALTH", (SCREEN_WIDTH - 300), (SCREEN_HEIGHT - 80), 20, RED);
	DrawRectangle( (SCREEN_WIDTH - 310), (SCREEN_HEIGHT - 60), (300 * (health / 100)), 50, RED);
	DrawRectangleLines( (SCREEN_WIDTH - 310), (SCREEN_HEIGHT - 60), 300, 50, RED);
}

void display_player_level(int level, float exp)
{
	char text[CHAR_LIMIT]; sprintf(text, "LEVEL: %d", level);
	DrawText(text, 10, (SCREEN_HEIGHT - 80), 20, GREEN);
	DrawRectangle( 10, (SCREEN_HEIGHT - 60), (300 * (exp / 100)), 50, GREEN);
	DrawRectangleLines( 10, (SCREEN_HEIGHT - 60), 300, 50, GREEN);
}

void death_screen()
{
	const char* DEATH_PROMPT = "YOU DIED, RESPAWN?"; 

	DrawText(DEATH_PROMPT, (GetScreenWidth() / 2.0f) - (MeasureText(DEATH_PROMPT, 30) / 2.0f),GetScreenHeight() / 2.0f, 30, RED);
	
	// respawn btn
	if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) - 125, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "RESPAWN"))
	{
		player = create_player();
		reset_entitylist(&world.entities);
	}

	// quit btn
	if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) + 50, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "QUIT")) 
		status = QUIT;
}

int main()
{
	init();
	
	while((status != QUIT) && (!WindowShouldClose()))
    {
		status = (player.health > 0) ? ALIVE : DEAD;
		camera.target = player.pos;
		mp = GetScreenToWorld2D(GetMousePosition(), camera);
		world.area = (Rectangle){camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, SCREEN_WIDTH, SCREEN_HEIGHT};
		update_entities(&world.entities);
		update_player(&world.walls, &world.health_buffs, &world.entities, &player);
		create_mobs(&world);
		BeginDrawing();
			switch(status)
			{
				case ALIVE:
					ClearBackground(BLACK);
					BeginMode2D(camera);
						draw_world(&world);
						draw_entities(&world.entities, world.area);
						DrawTexturePro(player.tx, (Rectangle){player.animation.xfposition * TILE_SIZE, player.animation.yfposition * TILE_SIZE, TILE_SIZE, TILE_SIZE},get_object_area(player.pos, SCREEN_TILE_SIZE), (Vector2){0,0}, 0, WHITE);
					EndMode2D();
					display_player_health(player.health);
					display_player_level(player.level, player.exp);
					break;
				
				case DEAD: 
					ClearBackground(GRAY);
					death_screen(); 
					break;
				
				default: 
					break;
			}
			DrawFPS(0, 0);
		EndDrawing();
    }
	dealloc_world(&world);
	CloseWindow();
    return 0;
}