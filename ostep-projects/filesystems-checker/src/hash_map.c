#include "hash_map.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BUCKET_NUM 12

static int adler32(const void *buf, size_t buflength)
{
     const char *buffer = (const char*)buf;
     int s1 = 1;
     int s2 = 0;
     for (size_t n = 0; n < buflength; n++) {
        s1 = (s1 + buffer[n]) % 65521;
        s2 = (s2 + s1) % 65521;
     }
     return (s2 << 16) | s1;
}

struct HashMap *hash_map_init(void)
{
    struct HashMap *map = malloc(sizeof(*map));
    assert(map && "malloc");
    struct MapBucket *buckets = malloc(sizeof(*buckets) * BUCKET_NUM);
    assert(buckets && "malloc");
    memset(buckets, 0, sizeof(*buckets) * BUCKET_NUM);
    map->count = BUCKET_NUM;
    map->buckets = buckets;
    return map;
}

static struct MapBucket *get_bucket(struct HashMap *map, char *key)
{
    int hashCode = adler32(key, strlen(key));
    int index = hashCode % BUCKET_NUM;
    return &map->buckets[index];
}

void hash_map_destroy(struct HashMap *map)
{
    for (int i = 0; i < map->count; i++) {
        struct MapBucket *bucket = &map->buckets[i];
        if (bucket->count == 0 && bucket->cap == 0) continue;
        for (int j = 0; j < bucket->cap; j++) {
            if (bucket->pairs[j].key == NULL) continue;
            free(bucket->pairs[j].key);
        }
        free(bucket->pairs);
    }
    free(map->buckets);
    free(map);
}

// set key++, if not exist then 1
void hash_map_add(struct HashMap *map, char *key)
{
    struct MapBucket *bucket = get_bucket(map, key);
    for (int i = 0; i < bucket->cap; i++) {
        if (bucket->pairs[i].key != NULL && strcmp(bucket->pairs[i].key, key) == 0) {
            bucket->pairs[i].value++;
            return;
        }
    }
    if (bucket->cap == 0) {
        bucket->cap = 4096;
        struct MapPair *pairs = malloc(sizeof(*pairs) * bucket->cap);
        memset(pairs, 0, sizeof(*pairs) * bucket->cap);
        assert(pairs && "malloc");
        bucket->pairs = pairs;
    }
    for (int i = 0; i < bucket->cap; i++) {
        if (bucket->pairs[i].key == NULL) {
            bucket->pairs[i].key = strdup(key);
            bucket->pairs[i].value = 1;
            bucket->count++;
            break;
        }
    }
}

int hash_map_get(struct HashMap *map, char *key)
{
    struct MapBucket *bucket = get_bucket(map, key);
    for (int i = 0; i < bucket->cap; i++)
        if (bucket->pairs[i].key != NULL && strcmp(bucket->pairs[i].key, key) == 0)
            return bucket->pairs[i].value;
    return -1;
}
