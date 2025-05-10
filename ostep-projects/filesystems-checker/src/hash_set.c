#include "hash_set.h"
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

struct HashSet *hash_set_init(void)
{
    struct HashSet *set = malloc(sizeof(*set));
    assert(set && "malloc");
    struct Bucket *buckets = malloc(sizeof(*buckets) * BUCKET_NUM);
    assert(buckets && "malloc");
    memset(buckets, 0, sizeof(*buckets) * BUCKET_NUM);
    set->count = BUCKET_NUM;
    set->buckets = buckets;
    return set;
}

static struct Bucket *get_bucket(struct HashSet *set, char *key)
{
    int hashCode = adler32(key, strlen(key));
    int index = hashCode % BUCKET_NUM;
    return &set->buckets[index];
}

void hash_set_destroy(struct HashSet *set)
{
    for (int i = 0; i < set->count; i++) {
        struct Bucket *bucket = &set->buckets[i];
        if (bucket->count == 0 && bucket->cap == 0) continue;
        for (int j = 0; j < bucket->cap; j++) {
            if (bucket->pairs[j].key == NULL) continue;
            free(bucket->pairs[j].key);
        }
        free(bucket->pairs);
    }
    free(set->buckets);
    free(set);
}

int hash_set_has(struct HashSet *set, char *key)
{
    struct Bucket *bucket = get_bucket(set, key);
    if (bucket->count == 0) return 0;
    for (int i = 0; i < bucket->cap; i++)
        if (bucket->pairs[i].key != NULL && strcmp(bucket->pairs[i].key, key) == 0)
            return 1;
    return 0;
}

void hash_set_add(struct HashSet *set, char *key)
{
    if (hash_set_has(set, key)) return;
    struct Bucket *bucket = get_bucket(set, key);
    if (bucket->cap == 0) {
        bucket->cap = 4096;
        struct Pair *pairs = malloc(sizeof(*pairs) * bucket->cap);
        memset(pairs, 0, sizeof(*pairs) * bucket->cap);
        assert(pairs && "malloc");
        bucket->pairs = pairs;
    }
    for (int i = 0; i < bucket->cap; i++) {
        if (bucket->pairs[i].key == NULL) {
            bucket->pairs[i].key = strdup(key);
            bucket->count++;
            break;
        }
    }
}

void hash_set_remove(struct HashSet *set, char *key)
{
    if (!hash_set_has(set, key)) return;
    struct Bucket *bucket = get_bucket(set, key);
    for (int i = 0; i < bucket->cap; i++) {
        if (bucket->pairs[i].key != NULL && strcmp(bucket->pairs[i].key, key) == 0) {
            bucket->pairs[i].key = NULL;
            bucket->count--;
            break;
        }
    }
}

int hash_set_size(struct HashSet *set)
{
    int size = 0;
    for (int i = 0; i < BUCKET_NUM; i++)
        if (set->buckets[i].cap != 0) size += set->buckets[i].count;
    return size;
}
