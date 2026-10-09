/* ring.h -- lock-free single-producer / single-consumer pointer queue.
 *
 * One capture thread pushes, the mix loop pops. With exactly one writer and
 * one reader the indices need no mutex, only acquire/release ordering, which
 * keeps the frame loop free of any call that can block or priority-invert.
 */
#ifndef FRAME_RING_H
#define FRAME_RING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fs_ring fs_ring;

/* `capacity` is rounded up to a power of two (one slot is reserved to tell
 * "full" apart from "empty", so usable depth is capacity - 1). */
fs_ring *fs_ring_create(size_t capacity);
void     fs_ring_destroy(fs_ring *ring);

/* Producer side only. Returns 1 on success, 0 when full. Never blocks. */
int fs_ring_push(fs_ring *ring, void *item);

/* Consumer side only. Returns NULL when empty. Never blocks. */
void *fs_ring_pop(fs_ring *ring);

size_t fs_ring_count(const fs_ring *ring);
size_t fs_ring_capacity(const fs_ring *ring);

#ifdef __cplusplus
}
#endif
#endif /* FRAME_RING_H */
