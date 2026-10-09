/* clock.h -- monotonic frame cadence.
 *
 * A switcher is genlocked: frames leave on a fixed grid and commands land on
 * frame boundaries, never between them. This is the software stand-in for that
 * reference -- a monotonic clock plus an absolute next-frame deadline, so
 * scheduling jitter does not accumulate into drift.
 */
#ifndef FRAME_CLOCK_H
#define FRAME_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t fs_now_ns(void);

typedef struct fs_clock {
    uint64_t frame_ns; /* nominal frame interval                          */
    uint64_t next_ns;  /* absolute deadline of the next frame             */
    uint64_t frames;   /* frames emitted                                 */
    uint64_t late;     /* frames whose deadline had already passed       */
} fs_clock;

/* 59.94 is 60000.0/1001.0 -- pass it as such, not as 59.94. */
void fs_clock_start(fs_clock *clock, double fps);

/* Sleeps until the next frame deadline. If the deadline is already behind us
 * it returns immediately, bumps `late`, and re-bases so one slow frame does
 * not cause a burst of catch-up frames. */
void fs_clock_wait(fs_clock *clock);

#ifdef __cplusplus
}
#endif
#endif /* FRAME_CLOCK_H */
