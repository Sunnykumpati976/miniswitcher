/* frame.h -- packed UYVY 4:2:2 frames and a fixed-size frame pool.
 *
 * Broadcast gear moves 4:2:2 chroma-subsampled video, not RGB, so that is what
 * MiniSwitcher carries end to end: two bytes per pixel, packed as
 *
 *     byte 0   byte 1   byte 2   byte 3
 *     U        Y0       V        Y1        <- one pixel *pair*
 *
 * Widths are therefore always even. Every buffer the frame loop touches comes
 * out of a pool allocated once at startup: there is no malloc in the hot path.
 */
#ifndef FRAME_FRAME_H
#define FRAME_FRAME_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_BYTES_PER_PIXEL 2

typedef struct fs_frame {
    uint8_t *data;       /* stride * height bytes, 64-byte aligned          */
    size_t   stride;     /* bytes per row                                   */
    uint16_t width;      /* luma samples per row (always even)              */
    uint16_t height;
    uint64_t pts;        /* presentation time, in frames since start        */
    uint32_t pool_index; /* owning slot; do not touch                       */
} fs_frame;

static inline size_t fs_frame_bytes(uint16_t width, uint16_t height)
{
    return (size_t)width * (size_t)height * FS_BYTES_PER_PIXEL;
}

/* ---- pool -------------------------------------------------------------- */

typedef struct fs_pool fs_pool;

/* Allocates `count` frames of `width`x`height` in one contiguous arena.
 * Returns NULL on bad geometry (odd or zero width) or allocation failure. */
fs_pool *fs_pool_create(size_t count, uint16_t width, uint16_t height);
void     fs_pool_destroy(fs_pool *pool);

/* O(1), no allocation, not thread safe. NULL when the pool is exhausted --
 * in the frame loop that is a dropped frame, not a reason to allocate. */
fs_frame *fs_pool_acquire(fs_pool *pool);
void      fs_pool_release(fs_pool *pool, fs_frame *frame);

size_t fs_pool_available(const fs_pool *pool);
size_t fs_pool_capacity(const fs_pool *pool);

/* ---- frame helpers ----------------------------------------------------- */

/* Row-wise copy; geometry must match. */
void fs_frame_copy(fs_frame *dst, const fs_frame *src);

/* Appends the frame to `out` as raw UYVY. Returns 0 on success, -1 on a short
 * write, so a full disk surfaces as an error instead of a corrupt file. */
int fs_frame_write(const fs_frame *frame, FILE *out);

#ifdef __cplusplus
}
#endif
#endif /* FRAME_FRAME_H */
