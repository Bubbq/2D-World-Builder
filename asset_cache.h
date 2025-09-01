#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include "raylib.h"
#include "uthash.h"

typedef struct
{
    Texture texture;
    char* path;
} Asset;

// asset operations
bool asset_init(Asset* dest, const Texture texture, const char* path);
void asset_free(Asset* asset);
bool asset_is_ready(const Asset* asset);

typedef struct
{
    UT_hash_handle hh;
    Asset asset;
    unsigned long int id;
} AssetEntry;

typedef AssetEntry* AssetCache;

// entry operations
AssetEntry* asset_entry_init(const Texture texture, const unsigned long int id, const char* path);
void asset_entry_free(AssetEntry* entry);
bool asset_entry_is_ready(const AssetEntry* entry);

// cache operations
void asset_cache_add(AssetCache* cache, AssetEntry* entry);
void asset_cache_remove(AssetCache* cache, AssetEntry* entry);
void asset_cache_free(AssetCache* cache);
AssetEntry* asset_cache_find(AssetCache* cache, const unsigned long int id);

#endif