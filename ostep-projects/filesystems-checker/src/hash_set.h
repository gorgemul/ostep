#ifndef _HASH_SET_H_
#define _HASH_SET_H_

struct Pair { char *key; int  value; };
struct Bucket { int count; int cap; struct Pair *pairs; };
struct HashSet { int count; struct Bucket *buckets; };

struct HashSet *hash_set_init(void);
void hash_set_destroy(struct HashSet *set);
int hash_set_has(struct HashSet *set, char *key);
void hash_set_add(struct HashSet *set, char *key);
void hash_set_remove(struct HashSet *set, char *key);
int hash_set_size(struct HashSet *set);

#endif // _HASH_SET_H_
