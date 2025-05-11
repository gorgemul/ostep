#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

struct MapPair { char *key; int value; };
struct MapBucket { int count; int cap; struct MapPair *pairs; };
struct HashMap { int count; struct MapBucket *buckets; };

struct HashMap *hash_map_init(void);
void hash_map_destroy(struct HashMap *map);
int hash_map_get(struct HashMap *map, char *key);
void hash_map_add(struct HashMap *map, char *key);

#endif // _HASH_MAP_H_
