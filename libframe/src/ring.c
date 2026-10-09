#include "frame/ring.h"

#include <stdatomic.h>
#include <stdlib.h>

struct fs_ring {
    _Atomic size_t   head; /* next slot the producer will write */
    _Atomic size_t   tail; /* next slot the consumer will read  */
    size_t           mask;
    size_t           capacity;
    void *_Atomic   *slots;
};

static size_t next_pow2(size_t value)
{
    size_t p = 1u;
    while (p < value)
        p <<= 1u;
    return p;
}

fs_ring *fs_ring_create(size_t capacity)
{
    if (capacity < 2u)
        capacity = 2u;
    capacity = next_pow2(capacity);

    fs_ring *ring = calloc(1u, sizeof *ring);
    if (ring == NULL)
        return NULL;

    ring->slots = calloc(capacity, sizeof *ring->slots);
    if (ring->slots == NULL) {
        free(ring);
        return NULL;
    }
    ring->capacity = capacity;
    ring->mask     = capacity - 1u;
    atomic_store_explicit(&ring->head, 0u, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0u, memory_order_relaxed);
    return ring;
}

void fs_ring_destroy(fs_ring *ring)
{
    if (ring == NULL)
        return;
    free(ring->slots);
    free(ring);
}

int fs_ring_push(fs_ring *ring, void *item)
{
    if (ring == NULL || item == NULL)
        return 0;

    const size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    const size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (head - tail >= ring->mask) /* one slot held back: full */
        return 0;

    atomic_store_explicit(&ring->slots[head & ring->mask], item,
                          memory_order_relaxed);
    /* Release: the slot write above must be visible before the index move. */
    atomic_store_explicit(&ring->head, head + 1u, memory_order_release);
    return 1;
}

void *fs_ring_pop(fs_ring *ring)
{
    if (ring == NULL)
        return NULL;

    const size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    const size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (tail == head)
        return NULL;

    void *item = atomic_load_explicit(&ring->slots[tail & ring->mask],
                                      memory_order_relaxed);
    atomic_store_explicit(&ring->tail, tail + 1u, memory_order_release);
    return item;
}

size_t fs_ring_count(const fs_ring *ring)
{
    if (ring == NULL)
        return 0u;
    const size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    const size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    return head - tail;
}

size_t fs_ring_capacity(const fs_ring *ring)
{
    return ring != NULL ? ring->mask : 0u; /* usable depth */
}
