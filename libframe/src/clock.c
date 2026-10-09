/* nanosleep is POSIX, not ISO C: glibc hides it under a strict -std=c11
 * build unless the feature-test macro is set before any include. */
#define _POSIX_C_SOURCE 200809L

#include "frame/clock.h"

#include <time.h>

#define FS_NS_PER_SEC 1000000000ull

uint64_t fs_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * FS_NS_PER_SEC + (uint64_t)ts.tv_nsec;
}

void fs_clock_start(fs_clock *clock, double fps)
{
    if (clock == NULL)
        return;
    if (fps <= 0.0)
        fps = 60.0;

    clock->frame_ns = (uint64_t)((double)FS_NS_PER_SEC / fps + 0.5);
    clock->next_ns  = fs_now_ns() + clock->frame_ns;
    clock->frames   = 0u;
    clock->late     = 0u;
}

void fs_clock_wait(fs_clock *clock)
{
    if (clock == NULL || clock->frame_ns == 0u)
        return;

    const uint64_t now = fs_now_ns();
    clock->frames++;

    if (now >= clock->next_ns) {
        /* Missed the boundary. Re-base on now instead of emitting a burst of
         * catch-up frames -- a switcher drops a frame, it does not sprint. */
        clock->late++;
        clock->next_ns = now + clock->frame_ns;
        return;
    }

    const uint64_t remaining = clock->next_ns - now;
    struct timespec req = {
        .tv_sec  = (time_t)(remaining / FS_NS_PER_SEC),
        .tv_nsec = (long)(remaining % FS_NS_PER_SEC),
    };
    while (nanosleep(&req, &req) == -1) {
        /* interrupted by a signal: finish the remaining interval */
    }
    clock->next_ns += clock->frame_ns;
}
