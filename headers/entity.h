#ifndef  ENTITY_H
#define ENTITY_H

#include <stdlib.h>

#include "raylib.h"
#include "animation.h"
#include "timer.h"

typedef struct
{
	char* name;
	float health;
	float speed;
	Vector2 pos;
	bool adjsp;
	bool move;
	Texture2D tx;
	Animation animation;
	int id;
	int damage;
	Timer attack_speed;
	Timer heal_speed;
	float exp;
	int level;
	float angle;
	Vector2 direction;
} Entity;

typedef struct
{
	int size;
	size_t cap;
	Entity* entities;
} EntityList;

void resize_entitylist(EntityList* entitylist);
void add_entity(EntityList* entitylist, Entity entity); 
EntityList create_entitylist();
int check_collision_entity(int entity_id, Vector2 entity_position, EntityList* entitylist);
void clear_entitylist(EntityList* entitylist);
void draw_entities(EntityList* entitylist, Rectangle world_border, int entity_size, int sprite_sheet_size);
void deal_damage(Entity* entity, Entity* target, int entity_size);
void heal(Entity* entity, int max_health, int heal_amount);
void gain_exp(Entity* entity);
void dealloc_entitylist(EntityList* entitylist);

#endif