#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "fdbserver/mc_queue.h"
#include <stdlib.h>
#include "fdbserver/Camp.h"

#ifdef BRANCH_16
    #define BRANCHING_FACTOR 16
#elif defined BRANCH_8
    #define BRANCHING_FACTOR 8
#elif defined BRANCH_4
    #define BRANCHING_FACTOR 4
#elif defined BRANCH_2
    #define BRANCHING_FACTOR 2
#else
    #define BRANCHING_FACTOR 8
#endif
typedef uint64_t key_type;
typedef struct lruq *item_type;



/**
 * Holds an inserted element, as well as the current index in the node array.
 * Acts as a handle to clients for the purpose of mutability.
 */
struct implicit_node_t
{
    //! Index for the item in the "tree" array
    uint32_t index;

    //! Pointer to a piece of client data
    item_type item;
    //! Key for the item
    key_type key;
} __attribute__ ((aligned(4)));

typedef struct implicit_node_t implicit_node;
typedef implicit_node pq_node_type;

/**
 * A mutable, meldable, array-based d-ary heap.  Maintains a single, complete
 * d-ary tree.  Imposes the standard heap invariant.
 */
struct implicit_heap_t
{
    //! The array of node pointers encoding the tree structure
    implicit_node **nodes;
    //! The number of items held in the queue
    uint32_t size;
    //! Current capacity of the heap
    uint32_t capacity;
} __attribute__ ((aligned(4)));

typedef struct implicit_heap_t implicit_heap;
typedef implicit_heap pq_type;
//==============================================================================
// STATIC DECLARATIONS
//==============================================================================

static void push( implicit_heap *queue, uint32_t src, uint32_t dst );
static void dump( implicit_heap *queue, implicit_node *node, uint32_t dst );
static uint32_t heapify_down( implicit_heap *queue, implicit_node *node );
static uint32_t heapify_up( implicit_heap *queue, implicit_node *node );
static void grow_heap( implicit_heap *queue );

//==============================================================================
// PUBLIC METHODS
//==============================================================================

implicit_heap* pq_create(void)
{
    implicit_heap *queue = calloc( 1, sizeof( implicit_heap ) );
    queue->size = 0;
    queue->capacity = 1;
    queue->nodes = calloc( 1, sizeof( implicit_node* ) );
    return queue;
}

void pq_clear( implicit_heap *queue )
{
    uint32_t i;
    for ( i = 0; i < queue->size; i++ )
        free( queue->nodes[i] );
    queue->size = 0;
}

void pq_destroy( implicit_heap *queue )
{
    pq_clear( queue );
    free( queue->nodes );
    free( queue );
}
#define UNUSED(x) (void)(x)
key_type pq_get_key( implicit_heap *queue, implicit_node *node )
{
    UNUSED( queue );
    return node->key;
}

item_type pq_get_item( implicit_heap *queue, implicit_node *node )
{
    UNUSED( queue );
    return node->item;
}

uint32_t pq_get_size( implicit_heap *queue )
{
    return queue->size;
}

implicit_node* pq_insert( implicit_heap *queue, item_type item, key_type key )
{
    implicit_node *node = calloc( 1, sizeof( implicit_node ) );
    node->item = item;
    node->key = key;
    node->index = queue->size++;

    if( queue->size == queue->capacity )
        grow_heap( queue );
    queue->nodes[node->index] = node;
    heapify_up( queue, node );

    return node;
}

bool pq_empty( implicit_heap *queue )
{
    return ( queue->size == 0 );
}

implicit_node* pq_find_min( implicit_heap *queue )
{
    if ( pq_empty( queue ) )
        return NULL;
    return queue->nodes[0];
}

key_type pq_delete( implicit_heap *queue, implicit_node* node )
{
    key_type key = node->key;
    implicit_node *last_node = queue->nodes[queue->size - 1];
    push( queue, last_node->index, node->index );
    queue->size--;

    if ( node != last_node )
        heapify_down( queue, last_node );

    free( node );
    return key;
}

key_type pq_delete_min( implicit_heap *queue )
{
    return pq_delete( queue, queue->nodes[0] );
}



void pq_decrease_key( implicit_heap *queue, implicit_node *node,
    key_type new_key )
{
    node->key = new_key;
    heapify_up( queue, node );
}

void pq_increase_key( implicit_heap *queue, implicit_node *node,
    key_type new_key )
{
    node->key = new_key;
    heapify_down( queue, node );
}



//==============================================================================
// STATIC METHODS
//==============================================================================

/**
 * Takes two node positions and pushes the src pointer into the second.
 * Essentially this is a single-sided swap, and produces a duplicate
 * record which is meant to be overwritten later.  A chain of these
 * operations will make up a heapify operation, and will be followed by
 * a @ref <dump> operation to finish the simulated "swapping" effect.
 *
 * @param queue Queue to which both nodes belong
 * @param src   Index of data to be duplicated
 * @param dst   Index of data to overwrite
 */
static void push( implicit_heap *queue, uint32_t src, uint32_t dst )
{
    if ( ( src >= queue->size ) || ( dst >= queue->size ) || ( src == dst ) )
        return;

    queue->nodes[dst] = queue->nodes[src];
    queue->nodes[dst]->index = dst;
}

/**
 * Places a node in a certain location in the tree, updating both the
 * queue structure and the node record.
 *
 * @param queue Queue to which the node belongs
 * @param node  Pointer to node to be dumped
 * @param dst   Index of location to dump node
 */
static void dump( implicit_heap *queue, implicit_node *node, uint32_t dst )
{
    queue->nodes[dst] = node;
    node->index = dst;
}

/**
 * Takes a node that is potentially at a higher position in the tree
 * than it should be, and pulls it up to the correct location.
 *
 * @param queue Queue to which node belongs
 * @param node  Potentially violating node
 */
static uint32_t heapify_down( implicit_heap *queue, implicit_node *node )
{
    if ( node == NULL )
        return -1;

    uint32_t sentinel, i, min;
    uint32_t base = node->index;
    while( base * BRANCHING_FACTOR + 1 < queue->size )
    {
        i = base * BRANCHING_FACTOR + 1;
        sentinel = i + BRANCHING_FACTOR;
        if( sentinel > queue->size )
            sentinel = queue->size;

        min = i++;
        for( i = i; i < sentinel; i++ )
        {
            if( queue->nodes[i]->key < queue->nodes[min]->key )
                min = i;
        }

        if ( queue->nodes[min]->key < node->key )
            push( queue, min, base );
        else
            break;

        base = min;
    }

    dump( queue, node, base );

    return node->index;
}

/**
 * Takes a node that is potentially at a lower position in the tree
 * than it should be, and pulls it up to the correct location.
 *
 * @param queue Queue to which node belongs
 * @param node  Potentially violating node
 */
static uint32_t heapify_up( implicit_heap *queue, implicit_node *node )
{
    if ( node == NULL )
        return -1;

    uint32_t i;
    for( i = node->index; i > 0; i = (i-1)/BRANCHING_FACTOR )
    {
        if ( node->key < queue->nodes[(i-1)/BRANCHING_FACTOR]->key )
            push( queue, (i-1)/BRANCHING_FACTOR, i );
        else
            break;
    }
    dump( queue, node, i );

    return node->index;
}

static void grow_heap( implicit_heap *queue )
{
    uint32_t new_capacity = queue->capacity * 2;
    implicit_node **new_array = realloc( queue->nodes, new_capacity *
        sizeof( implicit_node* ) );

    if( new_array == NULL )
        exit( -1 );

    queue->capacity = new_capacity;
    queue->nodes = new_array;
}





static void update_minimum(index_struct* index);
static uint32_t index_priority(index_struct* index, struct item* it);
static void lru_append(struct lruq* queue, struct item* it);
static void lru_delete(struct lruq* queue, struct item* it);
static void item_set_prev(struct item* it, struct item** prev);
static void item_set_next(struct item* it, struct item* next);
static struct item** item_prev(struct item* it);
static struct item** item_next(struct item* it);
static uint32_t item_priority(struct item* it);
static uint64_t item_inflated_priority(struct item* it);

/* Public methods */

int index_init(index_struct* index) {
	/* range = max_priority / min_priority */
	uint32_t precision = 11;
	/* min cost/size as a fraction: numerator, denominator */
	uint32_t min_priority[] = { 10000, 10 };
	/* max cost/size as a fraction: numerator, denominator */
	uint32_t max_priority[] = { 25000000, 1 };

  /* range = max_priority / min_priority */
  index->range = (max_priority[0] * min_priority[1]) /
    (max_priority[1] * min_priority[0]) + 1;
  index->minimum = 0;
  index->precision = precision;
  index->min_priority[0] = min_priority[0];
  index->min_priority[1] = min_priority[1];
  index->queue = pq_create();
  index->lruqs = (struct lruq *) calloc(index->range, sizeof(struct lruq));
  if (index->lruqs == NULL)
    return MC_ERROR;
  uint32_t i;
  for (i = 0; i < index->range; i++)
    index->lruqs[i].tail = &index->lruqs[i].head;
  return MC_OK;
}

int index_deinit(index_struct* index) {
  pq_destroy(index->queue); 
  free(index->lruqs);
  return MC_OK;
}

index_metadata* item_get_metadata(struct item* it) {
  return (index_metadata*)it;;
}
int index_insert(index_struct* index, struct item* it) {
  index_metadata *meta = (index_metadata *) item_get_metadata(it);
  meta->priority = index_priority(index, it);
  // printf("%u ",  meta->priority);
  meta->inflated_priority = meta->priority + index->minimum;
  struct lruq* lru = &index->lruqs[meta->priority];
  lru_append(lru, it);
  if (lru->node == NULL)
    lru->node = pq_insert(index->queue, lru, meta->inflated_priority);
  return meta->priority;
}

int index_delete(index_struct* index, struct item* it) {
  uint64_t old_priority, new_priority;
  struct lruq* lru = &index->lruqs[item_priority(it)];
  old_priority = item_inflated_priority(lru->head);
  lru_delete(lru, it);
  /* update heap if necessary */
  if (lru->head == NULL) {
    pq_delete(index->queue, lru->node);
    lru->node = NULL;
  } else {
    new_priority = item_inflated_priority(lru->head);
    if (new_priority != old_priority) {
      pq_increase_key(index->queue, lru->node, new_priority);
    }
  }
  update_minimum(index);
  return MC_OK;
}

struct item* index_get_next(index_struct* index, struct item* prev_item) {
  UNUSED(index);
  struct item** next = item_next(prev_item);
  /* try to return next item with same key */
  if (next != NULL)
    return *next;
  /* try to return strict successor of current key if any */
  uint32_t oldcost = item_priority(prev_item);
  uint32_t cost = oldcost;
  while (++cost < index->range && index->lruqs[cost].head == NULL);
  if (cost < index->range)
    return index->lruqs[cost].head;
  /* try to return minimum */
  cost = -1;
  while (++cost < oldcost && index->lruqs[cost].head == NULL);
  if (cost < oldcost)
    return index->lruqs[cost].head;
  return NULL; /* empty */
}

struct item* index_get_lowest(index_struct* index) {
  pq_node_type* node = pq_find_min(index->queue);
  if (node == NULL)
    return NULL;
  return pq_get_item(index->queue, node)->head;
}

int index_touch(struct item* it) {
  UNUSED(it);
  /* No-op. This technique does not require any action for index_touch. */
  return MC_OK;
}

/* Static methods */

static void update_minimum(index_struct* index) {
  if (pq_empty(index->queue))
    index->minimum = 0;
  else
    index->minimum = pq_get_key(index->queue, pq_find_min(index->queue));
}

static uint32_t item_size(struct item* it) {
  return ((index_metadata*) it) -> size;
}
#define BO_WORD_SIZE 32
#define BO_LOG_WORD_SIZE 5
#define BO_NULL_BIT_POS (uint32_t) ~0
/* number of leading zeros
   Harley's algorithm, based on table lookup, code from Hacker's Delight, 
   requires 32-bit ints. */
  #if defined __GNUC__
/* using built-in gcc functions*/
#  define bo_nlz_32(x) (uint32_t)__builtin_clz(x)
#  define bo_ntz_32(x) (uint32_t)__builtin_ctz(x)
#else
/* using table lookup methods */
static uint32_t bo_nlz_32(uint32_t word);
static uint32_t bo_ntz_32(uint32_t word);
#endif
#ifndef __GNUC__
static uint32_t bo_nlz_32(uint32_t x) {
#define u 99
  static uint32_t table[64] = {
    32, 31, u, 16, u, 30, 3, u,  15, u, u, u, 29, 10, 2, u, 
    u, u, 12, 14, 21, u, 19, u,  u, 28, u, 25, u, 9, 1, u, 
    17, u, 4, u, u, u, 11 ,u,    13, 22, 20, u, 26, u, u, 18,
    5, u, u, 23, u, 27, u, 6,    u, 24, 7, u, 8, u, 0, u };
#undef u
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >>16);
  x = x * 0x06EB14F9;  /* multiplier is 7 * 255**3 */
  return table[x >> 26];
}
#endif
/* most significant bit set or BO_NULL_BIT_POS if input is 0 */
uint32_t bo_msb(uint32_t word) {
  return (word == 0) ? BO_NULL_BIT_POS : BO_WORD_SIZE - 1 - bo_nlz_32(word);
}

/* least significant bit set or BO_NULL_BIT_POS if word is 0 */
uint32_t bo_lsb(uint32_t word) {
  return (word == 0) ? BO_NULL_BIT_POS : bo_ntz_32(word);
}

/* nearest bit set to 1 that is to the left of given bit or 
 * BO_NULL_BIT_POS if there is no such set bit */
uint32_t bo_nbl(uint32_t word, uint32_t index) {
  if (index == BO_WORD_SIZE - 1) {
    return BO_NULL_BIT_POS;
  }
  uint32_t mask = (uint32_t) ~0 << (index + 1);
  return bo_lsb(word & mask);
}

/* nearest bit set to 1 that is to the right of given bit or
 * BO_NULL_BIT_POS if there is no such set bit */
uint32_t bo_nbr(uint32_t word, uint32_t index) {
  if (index == 0) {
    return BO_NULL_BIT_POS;
  }
  uint32_t mask = (uint32_t) ~0 >> (BO_WORD_SIZE - index);
  return bo_msb(word & mask);
}

/* ceiling of the log (base 2) of given integer */
uint32_t bo_ceil_log(uint32_t word) {
  return BO_WORD_SIZE - bo_nlz_32(word - 1);
}

/* number of significant bits = msb + 1 */
uint32_t bo_bits_in_32(uint32_t word) {
  return BO_WORD_SIZE - bo_nlz_32(word);
}
static uint32_t index_priority(index_struct* index, struct item* it) {
  index_metadata* meta = (index_metadata*) item_get_metadata(it);
  /* calculate priority: (cost/size) / min_priority */
  uint32_t size = item_size(it);
  uint32_t priority = (meta->cost * index->min_priority[1]) / 
    (size * index->min_priority[0]);
  if (priority >= index->range)
    priority = index->range - 1;
  if(meta->cost * index->min_priority[1]  < index->min_priority[0] * size ) {
    // printf("%u %u\n", meta->cost, size);
    index->min_priority[0] = meta->cost;
    index->min_priority[1] = size;;
  }
  uint32_t p = index->precision;
  if (p != (uint32_t) -1) {
    /* round down, keeping only a number of bits specified by precision. */
    uint32_t m = bo_bits_in_32(priority);
    uint32_t n = (m > p) ? m - p : 0;
    priority = (priority >> n) << n;
  }
  return priority;
}

static void lru_append(struct lruq* queue, struct item* it) {
  index_metadata *meta = (index_metadata *) item_get_metadata(it);
  meta->next = NULL;
  meta->prev = queue->tail;
  *queue->tail = it;
  queue->tail = &meta->next;
}

static void lru_delete(struct lruq* queue, struct item* it) {
  index_metadata *meta = (index_metadata *) item_get_metadata(it);
  struct item** prev = meta->prev;
  struct item* next = meta->next;
  if (next != NULL)
    item_set_prev(next, prev);
  else
    queue->tail = prev;
  *prev = next;
}

static void item_set_prev(struct item* it, struct item** prev) {
  ((index_metadata*) item_get_metadata(it))->prev = prev;
}

static void item_set_next(struct item* it, struct item* next) {
  ((index_metadata*) item_get_metadata(it))->next = next;
}

static struct item** item_prev(struct item* it) {
	return ((index_metadata*) item_get_metadata(it))->prev;
}

static struct item** item_next(struct item* it) {
	return &((index_metadata*) item_get_metadata(it))->next;
}

static uint32_t item_priority(struct item* it) {
  return ((index_metadata*) item_get_metadata(it))->priority;
}

static uint64_t item_inflated_priority(struct item* it) {
  return ((index_metadata*) item_get_metadata(it))->inflated_priority;
}

int index_set_cost(struct item* it, cost_t cost) {
	index_metadata* meta = (index_metadata*) item_get_metadata(it);
	meta->cost = cost;
	return MC_OK;
}
