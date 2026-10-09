/* Frame pool: exhaustion must be reported, never papered over with a malloc. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "frame/frame.h"
#include "frame/pattern.h"

int main(void)
{
    assert(fs_pool_create(4u, 15u, 10u) == NULL); /* odd width rejected */
    assert(fs_pool_create(0u, 16u, 10u) == NULL);

    fs_pool *pool = fs_pool_create(3u, 64u, 32u);
    assert(pool != NULL);
    assert(fs_pool_capacity(pool) == 3u);
    assert(fs_pool_available(pool) == 3u);

    fs_frame *a = fs_pool_acquire(pool);
    fs_frame *b = fs_pool_acquire(pool);
    fs_frame *c = fs_pool_acquire(pool);
    assert(a && b && c);
    assert(a != b && b != c && a != c);
    assert(fs_pool_available(pool) == 0u);
    assert(fs_pool_acquire(pool) == NULL); /* exhausted, not expanded */

    assert(a->width == 64u && a->height == 32u);
    assert(a->stride == 64u * FS_BYTES_PER_PIXEL);
    assert(((uintptr_t)a->data % 64u) == 0u); /* 64-byte aligned */

    fs_pool_release(pool, b);
    assert(fs_pool_available(pool) == 1u);
    assert(fs_pool_acquire(pool) == b); /* LIFO: comes back hot */

    /* Copy must move every byte and carry the timestamp. */
    fs_pattern_bars(a);
    a->pts = 42u;
    fs_fill_black(c);
    fs_frame_copy(c, a);
    assert(memcmp(c->data, a->data, a->stride * a->height) == 0);
    assert(c->pts == 42u);

    /* Black is video black (16/128), not all zeroes. */
    fs_fill_black(a);
    assert(a->data[0] == 128u && a->data[1] == 16u);

    fs_pool_destroy(pool);
    printf("test_pool: ok\n");
    return 0;
}
