#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "fdbserver/mc_queue.h"
#include <stdlib.h>
#define MC_OK        0
#define MC_ERROR    -1
#define MC_EAGAIN   -2
#define MC_ENOMEM   -3
#define MC_INVALID  -4	/* the server is ok but the call is invalid in some ways (wrong token, ...) */
typedef struct index_heap index_struct;
typedef struct heap_index_metadata index_metadata;
typedef uint64_t cost_t;	/* Cost type */
// #define item heap_index_metadata

// Forward declaration so that this file can know what pq_node_type is.
// TODO: very messy. fix this circular dependency.
struct implicit_node_t;
typedef struct implicit_node_t implicit_node;
typedef implicit_node pq_node_type;

struct implicit_heap_t;
typedef struct implicit_heap_t implicit_heap;
typedef implicit_heap pq_type;

struct item;
struct heap_index_metadata {
	uint32_t cost;
	uint32_t size;
	uint32_t key_size;
	struct item** prev;
	struct item* next;
	uint32_t priority;
	uint64_t inflated_priority;
};
struct item {
    struct heap_index_metadata meta;
    char *key;
    
};

// typedef heap_index_metadata item;
typedef struct heap_index_metadata heap_index_metadata;
/* Per-item metadata needed by the index technique */
struct index_lru_metadata {
	TAILQ_ENTRY(item) i_tqe;      /* link in lru q or free q */
	cost_t cost;
};


TAILQ_HEAD(item_tqh, item);
/* Index structure. */
struct index_lru {
	struct item_tqh lruq;
};


struct index_heap {
	uint32_t range;
	uint64_t minimum;
	uint32_t precision;
	uint32_t min_priority[2];
	/* the components of the priority queue */
	pq_type* queue;
	struct lruq {
		struct item* head;
		struct item** tail;
		pq_node_type* node;
	} * lruqs;
};

typedef struct index_heap index_struct;
typedef struct heap_index_metadata index_metadata;
/* Initialize index structure. Reads settings for initialization parameters */
int index_init(index_struct* index);

/* Destruct index structure. Cleans up any memory allocated for this structure */
int index_deinit(index_struct* index);

/* Inserts an item into the index */
int index_insert(index_struct* index, struct item* it);

/* Removes an item from the index */
int index_delete(index_struct* index, struct item* it);

/* Given an item, get's the next item with the next highest priority */
struct item* index_get_next(index_struct* index, struct item* prev_item);

/* Gets the item with the lowest priority */
struct item* index_get_lowest(index_struct* index);

/* Mark an item as recently accessed */
int index_touch(struct item* it);

int index_set_cost(struct item* it, cost_t cost);

/* Run all unit tests for this index */
void indextest_run_all(void);