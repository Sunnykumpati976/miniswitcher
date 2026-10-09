/* SPSC ring: wraps forever, reports full instead of overwriting a frame. */
#include <assert.h>
#include <stdio.h>

#include "frame/ring.h"

int main(void)
{
    fs_ring *ring = fs_ring_create(5u); /* rounds to 8, usable depth 7 */
    assert(ring != NULL);
    assert(fs_ring_capacity(ring) == 7u);
    assert(fs_ring_count(ring) == 0u);
    assert(fs_ring_pop(ring) == NULL);

    static int items[16];
    for (int i = 0; i < 16; ++i)
        items[i] = i;

    for (int i = 0; i < 7; ++i)
        assert(fs_ring_push(ring, &items[i]) == 1);

    assert(fs_ring_count(ring) == 7u);
    assert(fs_ring_push(ring, &items[7]) == 0); /* full: caller drops */

    for (int i = 0; i < 7; ++i) {
        int *got = fs_ring_pop(ring);
        assert(got != NULL && *got == i); /* strict FIFO order */
    }
    assert(fs_ring_count(ring) == 0u);
    assert(fs_ring_pop(ring) == NULL);

    /* Push/pop past the wrap point many times over. */
    for (int round = 0; round < 1000; ++round) {
        assert(fs_ring_push(ring, &items[round % 16]) == 1);
        int *got = fs_ring_pop(ring);
        assert(got != NULL && *got == round % 16);
    }

    assert(fs_ring_push(ring, NULL) == 0); /* NULL is the empty sentinel */

    fs_ring_destroy(ring);
    printf("test_ring: ok\n");
    return 0;
}
