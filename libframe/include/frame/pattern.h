/* pattern.h -- synthetic sources.
 *
 * Until there is real capture, these stand in for cameras. Colour bars and a
 * luma ramp are the two signals you can eyeball for correctness; the moving
 * box proves motion and frame ordering are intact.
 */
#ifndef FRAME_PATTERN_H
#define FRAME_PATTERN_H

#include <stdint.h>
#include "frame/frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BT.709 limited-range conversion (Y 16-235, C 16-240). */
void fs_rgb_to_ycbcr(uint8_t r, uint8_t g, uint8_t b,
                     uint8_t *y, uint8_t *cb, uint8_t *cr);

void fs_fill_flat(fs_frame *frame, uint8_t y, uint8_t cb, uint8_t cr);
void fs_fill_black(fs_frame *frame);

void fs_pattern_bars(fs_frame *frame);            /* 100% colour bars      */
void fs_pattern_ramp(fs_frame *frame);            /* horizontal luma ramp  */
void fs_pattern_box(fs_frame *frame, uint64_t tick); /* bouncing box       */

#ifdef __cplusplus
}
#endif
#endif /* FRAME_PATTERN_H */
