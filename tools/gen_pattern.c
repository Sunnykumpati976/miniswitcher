/* gen_pattern -- render a synthetic source to raw UYVY.
 *
 *   gen_pattern --pattern bars --width 1280 --height 720 --frames 60 -o bars.yuv
 *   ffplay -f rawvideo -pixel_format uyvy422 -video_size 1280x720 bars.yuv
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frame/frame.h"
#include "frame/pattern.h"

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [--pattern bars|ramp|box|black] [--width N] [--height N]\n"
            "          [--frames N] [-o FILE]\n"
            "\nWidth must be even (4:2:2 carries one chroma pair per two pixels).\n"
            "Use '-' as FILE to write to stdout.\n",
            argv0);
}

int main(int argc, char **argv)
{
    const char *pattern = "bars";
    const char *out_path = "out.yuv";
    long width = 1280, height = 720, frames = 60;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        const int has_value = i + 1 < argc;

        if ((strcmp(arg, "--pattern") == 0 || strcmp(arg, "-p") == 0) && has_value) {
            pattern = argv[++i];
        } else if (strcmp(arg, "--width") == 0 && has_value) {
            width = strtol(argv[++i], NULL, 10);
        } else if (strcmp(arg, "--height") == 0 && has_value) {
            height = strtol(argv[++i], NULL, 10);
        } else if (strcmp(arg, "--frames") == 0 && has_value) {
            frames = strtol(argv[++i], NULL, 10);
        } else if ((strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0) && has_value) {
            out_path = argv[++i];
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "%s: unexpected argument '%s'\n", argv[0], arg);
            usage(argv[0]);
            return 2;
        }
    }

    if (width <= 0 || height <= 0 || width > 65535 || height > 65535 ||
        (width & 1) != 0 || frames <= 0) {
        fprintf(stderr, "%s: bad geometry %ldx%ld, %ld frames\n",
                argv[0], width, height, frames);
        return 2;
    }

    fs_pool *pool = fs_pool_create(1u, (uint16_t)width, (uint16_t)height);
    if (pool == NULL) {
        fprintf(stderr, "%s: cannot allocate frame pool\n", argv[0]);
        return 1;
    }

    fs_frame *frame = fs_pool_acquire(pool);
    FILE *out = strcmp(out_path, "-") == 0 ? stdout : fopen(out_path, "wb");
    if (out == NULL) {
        perror(out_path);
        fs_pool_destroy(pool);
        return 1;
    }

    int status = 0;
    for (long n = 0; n < frames; ++n) {
        frame->pts = (uint64_t)n;

        if (strcmp(pattern, "bars") == 0) {
            fs_pattern_bars(frame);
        } else if (strcmp(pattern, "ramp") == 0) {
            fs_pattern_ramp(frame);
        } else if (strcmp(pattern, "box") == 0) {
            fs_pattern_box(frame, (uint64_t)n * 8u);
        } else if (strcmp(pattern, "black") == 0) {
            fs_fill_black(frame);
        } else {
            fprintf(stderr, "%s: unknown pattern '%s'\n", argv[0], pattern);
            status = 2;
            break;
        }

        if (fs_frame_write(frame, out) != 0) {
            perror("write");
            status = 1;
            break;
        }
    }

    if (out != stdout) {
        fclose(out);
    }
    fs_pool_release(pool, frame);
    fs_pool_destroy(pool);

    if (status == 0) {
        fprintf(stderr, "%s: %ld frames of '%s' at %ldx%ld -> %s\n",
                argv[0], frames, pattern, width, height, out_path);
    }
    return status;
}
