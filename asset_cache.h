#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include "raylib.h"
#include "uthash.h"

typedef struct
{
    UT_hash_handle hh;    // for hashing operations, https://troydhanson.github.io/uthash/ for more information
    Texture texture;      // image data stored in GPU
    char* path;           // asset path, allocated
    unsigned long int id; // the hashcode (generated from the path of the texture)
} AssetEntry;

typedef AssetEntry* AssetCache;

// entry operations
AssetEntry* asset_entry_init(const char* asset_path);
void asset_entry_free(AssetEntry* entry);
bool asset_entry_is_ready(const AssetEntry* entry);

// cache operations
void asset_cache_add(AssetCache* cache, AssetEntry* entry);
void asset_cache_remove(AssetCache* cache, AssetEntry* entry);
void asset_cache_free(AssetCache* cache);
AssetEntry* asset_cache_find(AssetCache* cache, const unsigned long int id);

#endif