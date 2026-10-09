#include "frame/pattern.h"

#include <string.h>

/* BT.709 luma coefficients, and the chroma denominators 2*(1-Kb), 2*(1-Kr). */
#define KR 0.2126
#define KG 0.7152
#define KB 0.0722
#define CB_DEN 1.8556 /* 2 * (1 - KB) */
#define CR_DEN 1.5748 /* 2 * (1 - KR) */

static uint8_t clamp_u8(double v)
{
    if (v <= 0.0)
        return 0u;
    if (v >= 255.0)
        return 255u;
    return (uint8_t)(v + 0.5);
}

void fs_rgb_to_ycbcr(uint8_t r, uint8_t g, uint8_t b,
                     uint8_t *y_out, uint8_t *cb_out, uint8_t *cr_out)
{
    const double luma = KR * r + KG * g + KB * b; /* 0..255, full range */

    if (y_out != NULL)
        *y_out = clamp_u8(16.0 + luma * (219.0 / 255.0));
    if (cb_out != NULL)
        *cb_out = clamp_u8(128.0 + ((double)b - luma) / CB_DEN * (224.0 / 255.0));
    if (cr_out != NULL)
        *cr_out = clamp_u8(128.0 + ((double)r - luma) / CR_DEN * (224.0 / 255.0));
}

/* Fills a pixel-pair-aligned rectangle. x bounds are snapped to even columns
 * because one chroma sample is shared by two luma samples. */
static void fill_rect(fs_frame *frame, int x0, int y0, int x1, int y1,
                      uint8_t y, uint8_t cb, uint8_t cr)
{
    if (frame == NULL)
        return;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > frame->width)  x1 = frame->width;
    if (y1 > frame->height) y1 = frame->height;

    x0 &= ~1;            /* snap left edge down to a pair boundary  */
    x1 = (x1 + 1) & ~1;  /* and the right edge up                   */
    if (x1 > frame->width)
        x1 = frame->width;
    if (x0 >= x1 || y0 >= y1)
        return;

    for (int row = y0; row < y1; ++row) {
        uint8_t *p = frame->data + (size_t)row * frame->stride
                   + (size_t)x0 * FS_BYTES_PER_PIXEL;
        for (int col = x0; col < x1; col += 2) {
            p[0] = cb; /* U  */
            p[1] = y;  /* Y0 */
            p[2] = cr; /* V  */
            p[3] = y;  /* Y1 */
            p += 4;
        }
    }
}

void fs_fill_flat(fs_frame *frame, uint8_t y, uint8_t cb, uint8_t cr)
{
    if (frame == NULL)
        return;
    fill_rect(frame, 0, 0, frame->width, frame->height, y, cb, cr);
}

void fs_fill_black(fs_frame *frame)
{
    fs_fill_flat(frame, 16u, 128u, 128u); /* video black, not 0 */
}

void fs_pattern_bars(fs_frame *frame)
{
    static const uint8_t rgb[8][3] = {
        { 255, 255, 255 }, /* white   */
        { 255, 255,   0 }, /* yellow  */
        {   0, 255, 255 }, /* cyan    */
        {   0, 255,   0 }, /* green   */
        { 255,   0, 255 }, /* magenta */
        { 255,   0,   0 }, /* red     */
        {   0,   0, 255 }, /* blue    */
        {   0,   0,   0 }, /* black   */
    };

    if (frame == NULL)
        return;

    for (int i = 0; i < 8; ++i) {
        const int x0 = frame->width * i / 8;
        const int x1 = frame->width * (i + 1) / 8;
        uint8_t y, cb, cr;
        fs_rgb_to_ycbcr(rgb[i][0], rgb[i][1], rgb[i][2], &y, &cb, &cr);
        fill_rect(frame, x0, 0, x1, frame->height, y, cb, cr);
    }
}

void fs_pattern_ramp(fs_frame *frame)
{
    if (frame == NULL)
        return;

    const int pairs = frame->width / 2;
    for (uint16_t row = 0u; row < frame->height; ++row) {
        uint8_t *p = frame->data + (size_t)row * frame->stride;
        for (int pair = 0; pair < pairs; ++pair) {
            /* 16..235 left to right: the legal luma range, so a scope shows a
             * clean diagonal with no illegal excursions. */
            const int y0 = 16 + (219 * (pair * 2))     / (frame->width - 1);
            const int y1 = 16 + (219 * (pair * 2 + 1)) / (frame->width - 1);
            p[0] = 128u;
            p[1] = (uint8_t)y0;
            p[2] = 128u;
            p[3] = (uint8_t)y1;
            p += 4;
        }
    }
}

void fs_pattern_box(fs_frame *frame, uint64_t tick)
{
    if (frame == NULL)
        return;

    fs_fill_flat(frame, 40u, 128u, 128u); /* dark neutral background */

    const int size = frame->width / 6;
    const int span = frame->width - size;
    if (span <= 0)
        return;

    /* Triangle wave: run right, then back, so direction errors are obvious. */
    const int phase = (int)(tick % (uint64_t)(span * 2));
    const int x     = phase < span ? phase : span * 2 - phase;
    const int y     = (frame->height - size) / 2;

    uint8_t luma, cb, cr;
    fs_rgb_to_ycbcr(255u, 140u, 0u, &luma, &cb, &cr); /* orange */
    fill_rect(frame, x, y, x + size, y + size, luma, cb, cr);
}
