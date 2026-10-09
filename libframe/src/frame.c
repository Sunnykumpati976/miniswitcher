/* posix_memalign is POSIX, not ISO C: glibc hides it under a strict -std=c11
 * build unless the feature-test macro is set before any include. */
#define _POSIX_C_SOURCE 200809L

#include "frame/frame.h"

#include <stdlib.h>
#include <string.h>

#define FS_ALIGN 64

struct fs_pool {
    uint8_t  *arena;
    fs_frame *frames;
    uint32_t *free_stack;
    size_t    count;
    size_t    top;        /* number of slots currently free */
    size_t    frame_span; /* per-frame arena footprint, aligned */
    uint16_t  width;
    uint16_t  height;
};

static size_t align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1u) & ~(alignment - 1u);
}

fs_pool *fs_pool_create(size_t count, uint16_t width, uint16_t height)
{
    if (count == 0u || width == 0u || height == 0u || (width & 1u) != 0u)
        return NULL;

    fs_pool *pool = calloc(1u, sizeof *pool);
    if (pool == NULL)
        return NULL;

    pool->count      = count;
    pool->width      = width;
    pool->height     = height;
    pool->frame_span = align_up(fs_frame_bytes(width, height), FS_ALIGN);

    pool->frames     = calloc(count, sizeof *pool->frames);
    pool->free_stack = calloc(count, sizeof *pool->free_stack);
    if (pool->frames == NULL || pool->free_stack == NULL) {
        fs_pool_destroy(pool);
        return NULL;
    }

    void *arena = NULL;
    if (posix_memalign(&arena, FS_ALIGN, pool->frame_span * count) != 0) {
        fs_pool_destroy(pool);
        return NULL;
    }
    pool->arena = arena;
    memset(pool->arena, 0, pool->frame_span * count);

    for (size_t i = 0u; i < count; ++i) {
        pool->frames[i].data       = pool->arena + i * pool->frame_span;
        pool->frames[i].stride     = (size_t)width * FS_BYTES_PER_PIXEL;
        pool->frames[i].width      = width;
        pool->frames[i].height     = height;
        pool->frames[i].pts        = 0u;
        pool->frames[i].pool_index = (uint32_t)i;
        /* Free list is a LIFO stack, so a just-released frame comes back hot
         * in cache rather than cycling through every other buffer first. */
        pool->free_stack[i] = (uint32_t)(count - 1u - i);
    }
    pool->top = count;
    return pool;
}

void fs_pool_destroy(fs_pool *pool)
{
    if (pool == NULL)
        return;
    free(pool->arena);
    free(pool->frames);
    free(pool->free_stack);
    free(pool);
}

fs_frame *fs_pool_acquire(fs_pool *pool)
{
    if (pool == NULL || pool->top == 0u)
        return NULL;
    return &pool->frames[pool->free_stack[--pool->top]];
}

void fs_pool_release(fs_pool *pool, fs_frame *frame)
{
    if (pool == NULL || frame == NULL || pool->top >= pool->count)
        return;
    pool->free_stack[pool->top++] = frame->pool_index;
}

size_t fs_pool_available(const fs_pool *pool)
{
    return pool != NULL ? pool->top : 0u;
}

size_t fs_pool_capacity(const fs_pool *pool)
{
    return pool != NULL ? pool->count : 0u;
}

void fs_frame_copy(fs_frame *dst, const fs_frame *src)
{
    if (dst == NULL || src == NULL)
        return;
    if (dst->width != src->width || dst->height != src->height)
        return;

    if (dst->stride == src->stride) {
        memcpy(dst->data, src->data, dst->stride * dst->height);
    } else {
        const size_t row = (size_t)src->width * FS_BYTES_PER_PIXEL;
        for (uint16_t y = 0u; y < src->height; ++y)
            memcpy(dst->data + y * dst->stride, src->data + y * src->stride, row);
    }
    dst->pts = src->pts;
}

int fs_frame_write(const fs_frame *frame, FILE *out)
{
    if (frame == NULL || out == NULL)
        return -1;

    const size_t row = (size_t)frame->width * FS_BYTES_PER_PIXEL;
    for (uint16_t y = 0u; y < frame->height; ++y) {
        if (fwrite(frame->data + y * frame->stride, 1u, row, out) != row)
            return -1;
    }
    return 0;
}
