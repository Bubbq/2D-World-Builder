#include "headers/raylib.h"
#include "headers/raymath.h"
#include "headers/entity.h"
#include <stdlib.h>

void start_timer(Timer *timer, double lifetime) 
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool is_timer_done(Timer timer)
{ 
	return (GetTime() - timer.startTime) >= timer.lifeTime; 
} 

void resize_entitylist(EntityList* entitylist)
{
	entitylist->cap *= 2;
	entitylist->entities = realloc(entitylist->entities, entitylist->cap);
}

void add_entity(EntityList* entitylist, Entity entity)
{
	if(entitylist->size * sizeof(Entity) == entitylist->cap) 
        resize_entitylist(entitylist);

	entitylist->entities[entitylist->size++] = entity;
}

void delete_entity(EntityList* entitylist, int entity_id)
{
    int i = 0;

    // find index of entity to delete
    for(; i < entitylist->size; i++) 
        if(entitylist->entities[i].id == entity_id)
            break;

    for(; i < entitylist->size; i++) 
        entitylist->entities[i] = entitylist->entities[i + 1];
    
    entitylist->size--;
}

EntityList create_entitylist()
{
    EntityList entitylist;

    entitylist.size = 0;
    entitylist.cap = sizeof(Entity);
    entitylist.entities = malloc(entitylist.cap);

    return entitylist;
}

int check_collision_entity(int entity_id, Vector2 entity_position, EntityList* entitylist)
{
    for(int i = 0; i < entitylist->size; i++)
	{
		// self collision
		if(entity_id == entitylist->entities[i].id || entitylist->entities[i].health <= 0) 
            continue;

        Rectangle entity_area = (Rectangle){ entitylist->entities[i].pos.x, entitylist->entities[i].pos.y, 32, 32 };
		
        if(CheckCollisionPointRec(entity_position, entity_area))
             return i;
	}

	return -1;
}

void reset_entitylist(EntityList* entitylist)
{
	if(entitylist->size > 0) 
        free(entitylist->entities);
    
    *entitylist = create_entitylist();
}

void draw_entities(EntityList* entitylist, Rectangle world_border)
{
    for(int i = 0; i < entitylist->size; i++)
    {
        Entity* entity = (entitylist->entities + i);
        if(entity->health <= 0)
            continue;

        if(CheckCollisionPointRec((Vector2){ entity->pos.x + 16, entity->pos.y + 16 }, world_border))
		{
			// health bar
			DrawRectangle(entity->pos.x + 5, entity->pos.y - 7, 20 * (entity->health / 100), 7, RED);

            // entity sprite
			DrawTexturePro(entity->tx, (Rectangle){entity->animation.xfposition * 16, entity->animation.yfposition * 16, 16, 16}, (Rectangle){ entity->pos.x, entity->pos.y, 32, 32}, (Vector2){0,0}, 0, WHITE);
		}
    }
}

void deal_damage(Entity* entity, Entity* target)
{
    Rectangle entity_area = (Rectangle){ entity->pos.x, entity->pos.y, 32, 32 };
    Rectangle target_area = (Rectangle){ target->pos.x, target->pos.y, 32, 32 };

    if(CheckCollisionRecs(entity_area, target_area) && is_timer_done(entity->attack_speed))
    {
        target->health -= entity->damage;

        if(target->health <= 0)
            gain_exp(entity);

        start_timer(&entity->attack_speed, 1.0f);
    }
}

void gain_exp(Entity *entity)
{
    entity->exp += 20;

    // leveling up
    if(entity->exp >= (entity->level * 100))
    {
        entity->exp = 0;
        entity->level++;
    }
}

void dealloc_entitylist(EntityList* entitylist)
{
    free(entitylist->entities);
}