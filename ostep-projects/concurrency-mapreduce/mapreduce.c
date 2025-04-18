/* simple? implementation for mapreduce */
#include "mapreduce.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BUCKET_NUM    12
#define THREAD_MAX    12

struct Pair {
    char *k;
    char *v;
};

struct PairList {
    int len;
    int cap;
    struct Pair *pair;
};

// which is just list of list
struct PairSuperList {
    int len;
    int cap;
    pthread_mutex_t lock; 
    struct PairList **pair_list;
};

typedef struct {
    char *key;
    int  value;
} H_Pair;

typedef struct {
    int count;
    H_Pair *pairs;
} Bucket;

typedef struct {
    int count;
    int var_offset;
    Bucket *buckets;
} HashMap;

struct MapperJob {
    Mapper mapper;
    char *file_name;
};

typedef void (*Shuffler)(struct PairList *pair_list);

struct ShufflerJob {
    Shuffler shuffler;
    struct PairList *pair_list;
};

struct ReducerJob {
    Reducer         reducer;
    char            *key;
    Getter          get_func;
    int             partition_number;
};

struct Thread {
    int             id;   // read-friendly id
    int             used;
    pthread_cond_t  cond;
    pthread_mutex_t lock;
    pthread_t       tid;  // underneath opaque id
};

struct ThreadPool {
    int size;
    int total_work;
    int finished_work;
    pthread_cond_t all_work_done;
    pthread_mutex_t lock;
    struct Thread  *threads;
};

static int hash_map_get(HashMap *map, char *key);
static int hash_map_put(HashMap *map, char *key, int val);

HashMap *hash_map = NULL;
struct PairSuperList *result_list = NULL;
struct ThreadPool *thread_pool = NULL;
volatile int worker_id = 0;
int *pair_super_list_len_info = NULL;

uint64_t gettid()
{
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return tid;
}

int get_worker_id()
{
    int value;
    uint64_t tid = gettid();
    char *tid_str = malloc(sizeof(char) * 64);
    snprintf(tid_str, 64, "__THREAD__%lld", tid);
    if ((value = hash_map_get(hash_map, tid_str)) == -1) {
        hash_map_put(hash_map, tid_str, worker_id);
        return worker_id++;
    }
    return value;
}

static struct PairList *pair_list_init(int cap)
{
    assert(cap > 0);
    struct PairList *pair_list = malloc(sizeof(*pair_list));
    assert(pair_list);
    struct Pair *pair = malloc(sizeof(*pair) * cap);
    assert(pair);
    memset(pair, 0, sizeof(*pair) * cap);
    pair_list->pair = pair;
    pair_list->cap = cap;
    pair_list->len = 0;
    return pair_list;
}

static int pair_list_is_empty_slot(struct Pair pair)
{
    return pair.k == NULL && pair.v == NULL;
}

static int pair_list_add(struct PairList *pair_list, char *key, char *value)
{
    if (!key || !value) return -1;
    if (pair_list->len == pair_list->cap) {
        int old_cap = pair_list->cap;
        pair_list->cap *= 2;
        struct Pair *new_pair = realloc(pair_list->pair, sizeof(struct Pair) * pair_list->cap);
        assert(new_pair);
        memset(new_pair + old_cap, 0, sizeof(struct Pair) * old_cap);
        pair_list->pair = new_pair;
    }
    for (int i = 0; i < pair_list->cap; i++) {
        if (pair_list_is_empty_slot(pair_list->pair[i])) {
            pair_list->pair[i].k = strndup(key, strlen(key));
            assert(pair_list->pair[i].k);
            pair_list->pair[i].v = strndup(value, strlen(value));
            assert(pair_list->pair[i].v);
            pair_list->len++;
            return 0;
        }
    }
    return -1;
}

/* static void pair_list_dump(struct PairList *pair_list)
{
    for (int i = 0; i < pair_list->cap; i++)
        if (!pair_list_is_empty_slot(pair_list->pair[i]))
            printf("[pair list %d entry]: %s-%s\n", i+1, pair_list->pair[i].k, pair_list->pair[i].v);
} */

static void pair_list_destroy(struct PairList *pair_list)
{
    for (int i = 0; i < pair_list->cap; i++) {
        if (pair_list_is_empty_slot(pair_list->pair[i])) continue;
        free(pair_list->pair[i].k);
        free(pair_list->pair[i].v);
    }
    free(pair_list->pair);
    free(pair_list);
}

struct PairSuperList *pair_super_list_init(int cap)
{
    assert(cap > 0);
    struct PairSuperList *pair_super_list = malloc(sizeof(*pair_super_list));
    assert(pair_super_list);
    struct PairList **pair_list = malloc(sizeof(*pair_list) * cap);
    assert(pair_list);
    memset(pair_list, 0, sizeof(*pair_list) * cap);
    pair_super_list->pair_list = pair_list;
    pair_super_list->cap = cap;
    pair_super_list->len = 0;
    pthread_mutex_init(&pair_super_list->lock, NULL);
    return pair_super_list;
}

static int pair_super_list_put(struct PairSuperList *pair_super_list, struct PairList *pair_list, int index)
{
    if (!pair_list) return -1;
    while (index >= pair_super_list->cap) {
        int old_cap = pair_super_list->cap;
        pair_super_list->cap *= 2;
        struct PairList **new_pair_list = realloc(pair_super_list->pair_list, sizeof(struct PairList*) * pair_super_list->cap);
        assert(new_pair_list);
        memset(new_pair_list + old_cap, 0, sizeof(struct PairList*) * old_cap);
        pair_super_list->pair_list = new_pair_list;
    }
    pair_super_list->pair_list[index] = pair_list;
    pair_super_list->len++;
    return 0;
}

/* static int pair_super_list_remove(struct PairSuperList *pair_super_list, int index)
{
    if (index < 0 || index >= pair_super_list->cap) return -1;
    if (!pair_super_list->pair_list[index]) return 0;
    pair_list_destroy(pair_super_list->pair_list[index]);
    pair_super_list->pair_list[index] = NULL;
    return 0;
} */

/* static void pair_super_list_dump(struct PairSuperList *pair_super_list)
{
    for (int i = 0; i < pair_super_list->cap; i++) {
        if (pair_super_list->pair_list[i]) {
            printf("======[pair super list %d entry]=====\n", i+1);
            pair_list_dump(pair_super_list->pair_list[i]);
        }
    }
} */

static void pair_super_list_destroy(struct PairSuperList *pair_super_list)
{
    for (int i = 0; i < pair_super_list->cap; i++) {
        if (pair_super_list->pair_list[i]) pair_list_destroy(pair_super_list->pair_list[i]);
    }
    free(pair_super_list->pair_list);
    free(pair_super_list);
}

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

static HashMap *hash_map_init(void)
{
    HashMap *map = malloc(sizeof(*map));
    if (map == NULL) return NULL;

    Bucket *buckets = malloc(sizeof(*buckets) * BUCKET_NUM);
    if (buckets == NULL) {
        free(map);
        return NULL;
    }
    memset(buckets, 0, sizeof(*buckets) * BUCKET_NUM);

    map->count = BUCKET_NUM;
    map->var_offset = 0;
    map->buckets = buckets;

    return map;
}

static int hash_map_get(HashMap *map, char *key)
{
    int hashCode = adler32(key, strlen(key));
    int index = hashCode % BUCKET_NUM;
    Bucket *bucket = &(map->buckets[index]);

    if (bucket->count == 0) return -1;

    for (int i = 0; i < bucket->count; i++) {
        H_Pair *pair = &(bucket->pairs[i]);
        if (strcmp(pair->key, key) == 0) return pair->value;
    }

    return -1;
}

static int hash_map_put(HashMap *map, char *key, int val)
{
    int hashCode = adler32(key, strlen(key));
    int index = hashCode % BUCKET_NUM;
    Bucket *bucket = &(map->buckets[index]);

    for (int i = 0; i < bucket->count; i++) {
        H_Pair *pair = &(bucket->pairs[i]);
        if (strcmp(pair->key, key) == 0) {
            pair->value = val;
            return 0;
        }
    }

    bucket->count++;

    H_Pair *new_pairs = realloc(bucket->pairs, sizeof(*new_pairs) * bucket->count);
    if (new_pairs == NULL) return -1;

    char *new_key = malloc(sizeof(*new_key) * (strlen(key)+1));
    if (new_key == NULL) {
        free(new_pairs);
        return -1;
    }

    bucket->pairs = new_pairs;
    H_Pair *new_pair = &(bucket->pairs[(bucket->count)-1]);
    new_pair->key = new_key;
    strcpy(new_key, key);
    new_pair->value = val;

    return 0;
}

/* static void hash_map_dump(HashMap *map) */
/* { */
/*     int count = 1; */
/*     for (int i = 0; i < map->count; i++) { */
/*         if (map->buckets[i].count == 0) continue; */
/*         for (int j = 0; j < map->buckets[i].count; j++) { */
/*             if (strstr(map->buckets[i].pairs[j].key, "__THREAD__") != NULL) continue; */
/*             printf("[HASH_MAP %d ENTRY] %s %d\n", count++, map->buckets[i].pairs[j].key, map->buckets[i].pairs[j].value); */
/*         } */
/*     } */
/* } */

static void hash_map_destroy(HashMap *map)
{
    for (int i = 0; i < map->count; i++) {
        Bucket *bucket = &(map->buckets[i]);

        if (bucket->count == 0) continue;
        for (int j = 0; j < bucket->count; j++) {
            H_Pair *pair = &(bucket->pairs[j]);
            free(pair->key);
        }
        free(bucket->pairs);
    }

    free(map->buckets);
    free(map);
}

static void shuffle(struct PairList *pair_list)
{
    for (int i = 0; i < pair_list->cap; i++) {
        if (!pair_list_is_empty_slot(pair_list->pair[i])) {
            pthread_mutex_lock(&result_list->lock);
            int value = hash_map_get(hash_map, pair_list->pair[i].k);
            hash_map_put(hash_map, pair_list->pair[i].k, atoi(pair_list->pair[i].v) + ( value == -1 ? 0 : value ));
            pthread_mutex_unlock(&result_list->lock);
        }
    }
}

static void shuffle_pair_super_list(struct PairSuperList **pair_super_list, HashMap *map)
{
    struct PairSuperList *new_pair_super_list = pair_super_list_init(12);
    int counter = 0;
    for (int i = 0; i < map->count; i++) {
        if (map->buckets[i].count == 0) continue;
        for (int j = 0; j < map->buckets[i].count; j++) {
            if (strstr(map->buckets[i].pairs[j].key, "__THREAD__") != NULL) continue;
            struct PairList *new_pair_list = pair_list_init(1);
            char value_str[64];
            sprintf(value_str, "%d", map->buckets[i].pairs[j].value);
            pair_list_add(new_pair_list, map->buckets[i].pairs[j].key, value_str);
            pair_super_list_put(new_pair_super_list, new_pair_list, counter++);
        }
    }
    pair_super_list_destroy(*pair_super_list);
    *pair_super_list = new_pair_super_list;
}

static void *mapper(void *arg)
{
    struct MapperJob *job = (struct MapperJob*)arg;
    job->mapper(job->file_name);
    return NULL;
}

static void *shuffler(void *arg)
{
    struct ShufflerJob *job = (struct ShufflerJob*)arg;
    job->shuffler(job->pair_list);
    return NULL;
}

static void *reducer(void *arg)
{
    struct ReducerJob *job = (struct ReducerJob *)arg;
    job->reducer(job->key, job->get_func, job->partition_number);
    return NULL;
}

static struct ThreadPool *thread_pool_init(int size, int total_work)
{
    assert(size > 0);
    struct ThreadPool *thread_pool = malloc(sizeof(*thread_pool)); 
    assert(thread_pool);
    struct Thread *threads = malloc(sizeof(*threads) * size);
    assert(threads);
    for (int i = 0; i < size; i++) {
        threads[i].id = i;
        threads[i].used = 0;
        pthread_cond_init(&threads[i].cond, NULL);
        pthread_mutex_init(&threads[i].lock, NULL);
    }
    thread_pool->threads = threads;
    thread_pool->size = size;
    thread_pool->total_work = total_work;
    thread_pool->finished_work = 0;
    pthread_cond_init(&thread_pool->all_work_done, NULL);
    pthread_mutex_init(&thread_pool->lock, NULL);
    return thread_pool;
}

static void thread_pool_add_work(struct ThreadPool *thread_pool, int partition_number, void*(*worker)(void *), void *arg)
{
    assert(partition_number >= 0 && partition_number < thread_pool->size);
    pthread_mutex_lock(&thread_pool->threads[partition_number].lock);
    while (thread_pool->threads[partition_number].used)
        pthread_cond_wait(&thread_pool->threads[partition_number].cond, &thread_pool->threads[partition_number].lock);
    thread_pool->threads[partition_number].used = 1;
    pthread_create(&thread_pool->threads[partition_number].tid, NULL, worker, arg);
    pthread_detach(thread_pool->threads[partition_number].tid);
    pthread_mutex_unlock(&thread_pool->threads[partition_number].lock);
}

static int get_total_work(struct PairSuperList *pair_super_list)
{
    int counter = 0;
    for (int i = 0; i < pair_super_list->cap; i++) {
        if (!pair_super_list->pair_list[i]) continue;
        for (int j = 0; j < pair_super_list->pair_list[i]->cap; j++)
            if (!pair_list_is_empty_slot(pair_super_list->pair_list[i]->pair[j]))
                counter++;
    }
    return counter;
}

static void thread_pool_wait_work_done(struct ThreadPool *thread_pool)
{
    pthread_mutex_lock(&thread_pool->lock);
    while (thread_pool->finished_work != thread_pool->total_work) 
        pthread_cond_wait(&thread_pool->all_work_done, &thread_pool->lock);
    pthread_mutex_unlock(&thread_pool->lock);
}

static void thread_pool_destroy(struct ThreadPool *thread_pool)
{
    assert(thread_pool);
    free(thread_pool->threads);
    free(thread_pool);
}

static int get_key_index(struct PairSuperList *shuffled_pair_super_list, char *key)
{
    for (int i = 0; i < shuffled_pair_super_list->len; i++) {
        if (i >= shuffled_pair_super_list->cap || !shuffled_pair_super_list->pair_list[i] || !shuffled_pair_super_list->pair_list[i]->pair) continue;
        if (strcmp(key, shuffled_pair_super_list->pair_list[i]->pair[0].k) == 0) return i;
    }
    return -1;
}

static char *get_next(char *key, int partition_number)
{
    pthread_mutex_lock(&result_list->lock);
    if (!pair_super_list_len_info) {
        pair_super_list_len_info = malloc(sizeof(int) * result_list->len);
        assert(pair_super_list_len_info);
        memset(pair_super_list_len_info, 0, sizeof(int) * result_list->len);
    }
    char *value = NULL;
    int index = get_key_index(result_list, key);
    int total_num = result_list->pair_list[index]->len;
    int current_num = pair_super_list_len_info[index];
    if (current_num < total_num) {
        value = result_list->pair_list[index]->pair[current_num].v;
        pair_super_list_len_info[index]++;
        pthread_mutex_lock(&thread_pool->lock);
        if (++thread_pool->finished_work == thread_pool->total_work)
            pthread_cond_signal(&thread_pool->all_work_done);
        pthread_mutex_unlock(&thread_pool->lock);
    } else {
        pthread_mutex_lock(&thread_pool->threads[partition_number].lock);
        thread_pool->threads[partition_number].used = 0;
        pthread_cond_signal(&thread_pool->threads[partition_number].cond);
        pthread_mutex_unlock(&thread_pool->threads[partition_number].lock);
    }
    pthread_mutex_unlock(&result_list->lock);
    return value;
}

void MR_Emit(char *key, char *value)
{
    pthread_mutex_lock(&result_list->lock);
    int id = get_worker_id();
    if (id >= result_list->cap || !result_list->pair_list[id]) {
        struct PairList *pair_list = pair_list_init(12);
        pair_super_list_put(result_list, pair_list, id);
    }
    pair_list_add(result_list->pair_list[id], key, value);
    pthread_mutex_unlock(&result_list->lock);
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce, int num_reducers, Partitioner partition)
{
    assert(num_mappers >= 1 && num_mappers <= THREAD_MAX);
    assert(num_reducers >= 1 && num_reducers <= THREAD_MAX);
    assert(argc > 1);
    int file_num = argc - 1;
    
    pthread_t *threads = malloc(sizeof(*threads) * num_mappers);
    assert(threads);
    struct MapperJob *mapper_jobs = malloc(sizeof(*mapper_jobs) * num_mappers);
    assert(mapper_jobs);
    struct ShufflerJob *shuffler_jobs = malloc(sizeof(*shuffler_jobs) * num_mappers);
    assert(shuffler_jobs);
    result_list = pair_super_list_init(num_mappers);
    hash_map = hash_map_init();
    int mapper_job_num = file_num;
    int file_index = 1;
    while (mapper_job_num > 0) {
        for (int i = 0; (i < num_mappers && i < mapper_job_num); i++) {
            mapper_jobs[i].mapper = map;
            mapper_jobs[i].file_name = argv[file_index++];
            pthread_create(&threads[i], NULL, mapper, (void*)&mapper_jobs[i]);
        }
        for (int i = 0; (i < num_mappers && i < mapper_job_num); i++) pthread_join(threads[i], NULL);
        mapper_job_num -= num_mappers;
    }
    int shuffler_job_num = file_num;
    int shffle_offset = 0;
    while (shuffler_job_num > 0) {
        for (int i = 0; (i < num_mappers && i < shuffler_job_num); i++) {
            shuffler_jobs[i].shuffler = shuffle;
            shuffler_jobs[i].pair_list = result_list->pair_list[shffle_offset++];
            pthread_create(&threads[i], NULL, shuffler, (void*)&shuffler_jobs[i]);
        }
        for (int i = 0; (i < num_mappers && i < shuffler_job_num); i++) pthread_join(threads[i], NULL);
        shuffler_job_num -= num_mappers;
    }
    shuffle_pair_super_list(&result_list, hash_map);
    thread_pool = thread_pool_init(num_reducers, get_total_work(result_list));
    struct ReducerJob *reducer_jobs = malloc(sizeof(*reducer_jobs) * result_list->len); 
    assert(reducer_jobs);
    for (int i = 0; i < result_list->len; i++) {
        int partition_number = partition(result_list->pair_list[i]->pair[0].k, num_reducers);
        reducer_jobs[i].reducer = reduce;
        reducer_jobs[i].key = result_list->pair_list[i]->pair[0].k;
        reducer_jobs[i].get_func = get_next;
        reducer_jobs[i].partition_number = partition_number;
        thread_pool_add_work(thread_pool, partition_number, reducer, (void*)&reducer_jobs[i]);
    }
    thread_pool_wait_work_done(thread_pool);
    pair_super_list_destroy(result_list);
    thread_pool_destroy(thread_pool);
    hash_map_destroy(hash_map);
    free(threads);
    free(mapper_jobs);
    free(shuffler_jobs);
    free(reducer_jobs);
}
