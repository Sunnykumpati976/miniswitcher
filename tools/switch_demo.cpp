// switch_demo -- drive the transition engine over a scripted show and report
// what each frame cost against the frame budget.
//
//   switch_demo                                   # writes demo.yuv at 640x360
//   switch_demo --bench --width 1920 --height 1080 # timing only, no file I/O
//
// The point of --bench is that a switcher is a real-time system: the question
// is never "how fast on average" but "did any frame miss its deadline".

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

#include "core/transition.hpp"
#include "frame/clock.h"
#include "frame/frame.h"
#include "frame/pattern.h"

namespace {

struct Options {
    int width = 640;
    int height = 360;
    double fps = 60.0;
    bool bench = false;
    bool realtime = false;
    std::string out = "demo.yuv";
};

// One scripted step of the show: hold a source, or run a transition over N
// frames. This is the shape a macro recorder will later serialise.
struct Step {
    std::string transition; // "cut" | "mix" | "wipe-right" | ... | "" to hold
    int frames;
    bool swap_after; // program and preview trade places once it completes
};

void usage(const char *argv0)
{
    std::fprintf(stderr,
                 "usage: %s [--width N] [--height N] [--fps F] [--bench]\n"
                 "          [--realtime] [-o FILE]\n",
                 argv0);
}

bool parse(int argc, char **argv, Options &opt)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool has_value = i + 1 < argc;

        if (arg == "--width" && has_value) {
            opt.width = std::atoi(argv[++i]);
        } else if (arg == "--height" && has_value) {
            opt.height = std::atoi(argv[++i]);
        } else if (arg == "--fps" && has_value) {
            opt.fps = std::atof(argv[++i]);
        } else if (arg == "--bench") {
            opt.bench = true;
        } else if (arg == "--realtime") {
            opt.realtime = true;
        } else if ((arg == "-o" || arg == "--out") && has_value) {
            opt.out = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            usage(argv[0]);
            return false;
        } else {
            std::fprintf(stderr, "unexpected argument '%s'\n", arg.c_str());
            usage(argv[0]);
            return false;
        }
    }

    if (opt.width <= 0 || opt.height <= 0 || (opt.width & 1) != 0 || opt.fps <= 0.0) {
        std::fprintf(stderr, "bad geometry %dx%d @ %.3f fps\n", opt.width,
                     opt.height, opt.fps);
        return false;
    }
    return true;
}

void report(std::vector<double> &us, double budget_us, std::uint64_t late)
{
    if (us.empty()) {
        return;
    }
    std::sort(us.begin(), us.end());

    const double sum = std::accumulate(us.begin(), us.end(), 0.0);
    const double avg = sum / static_cast<double>(us.size());
    const double p50 = us[us.size() / 2];
    const std::size_t p99_index =
        std::min(us.size() - 1u,
                 static_cast<std::size_t>(static_cast<double>(us.size()) * 0.99));
    const double p99 = us[p99_index];
    const double max = us.back();

    std::printf("\ncomposite time per frame (%zu frames)\n", us.size());
    std::printf("  budget  %8.1f us\n", budget_us);
    std::printf("  avg     %8.1f us  (%4.1f%% of budget)\n", avg, 100.0 * avg / budget_us);
    std::printf("  p50     %8.1f us\n", p50);
    std::printf("  p99     %8.1f us  (%4.1f%% of budget)\n", p99, 100.0 * p99 / budget_us);
    std::printf("  max     %8.1f us  (%4.1f%% of budget)\n", max, 100.0 * max / budget_us);
    std::printf("  over budget: %zu frame(s)\n",
                static_cast<std::size_t>(
                    std::count_if(us.begin(), us.end(),
                                  [budget_us](double v) { return v > budget_us; })));
    if (late > 0) {
        std::printf("  missed cadence deadlines: %llu\n",
                    static_cast<unsigned long long>(late));
    }
}

} // namespace

int main(int argc, char **argv)
{
    Options opt;
    if (!parse(argc, argv, opt)) {
        return 2;
    }

    const auto w = static_cast<std::uint16_t>(opt.width);
    const auto h = static_cast<std::uint16_t>(opt.height);

    // Four buffers, allocated once: program source, preview source, output,
    // and one spare. Nothing below this line allocates a frame.
    fs_pool *pool = fs_pool_create(4u, w, h);
    if (pool == nullptr) {
        std::fprintf(stderr, "cannot allocate frame pool\n");
        return 1;
    }

    fs_frame *program = fs_pool_acquire(pool);
    fs_frame *preview = fs_pool_acquire(pool);
    fs_frame *out = fs_pool_acquire(pool);

    const std::vector<Step> show = opt.bench
        ? std::vector<Step>{ { "mix", 300, false } }
        : std::vector<Step>{ { "", 30, false },
                             { "mix", 36, true },
                             { "", 18, false },
                             { "wipe-right", 24, true },
                             { "", 12, false } };

    std::FILE *sink = nullptr;
    if (!opt.bench) {
        sink = opt.out == "-" ? stdout : std::fopen(opt.out.c_str(), "wb");
        if (sink == nullptr) {
            std::perror(opt.out.c_str());
            fs_pool_destroy(pool);
            return 1;
        }
    }

    fs_clock clock{};
    fs_clock_start(&clock, opt.fps);

    std::vector<double> timings;
    timings.reserve(512);

    std::uint64_t tick = 0;
    bool program_is_bars = true;
    int status = 0;

    for (const Step &step : show) {
        std::unique_ptr<ms::Transition> transition;
        if (!step.transition.empty()) {
            transition = ms::make_transition(step.transition);
            if (transition == nullptr) {
                std::fprintf(stderr, "unknown transition '%s'\n",
                             step.transition.c_str());
                status = 2;
                break;
            }
        }

        for (int n = 0; n < step.frames && status == 0; ++n, ++tick) {
            // Re-render both buses. Real sources will arrive over the SPSC
            // ring instead; the mix stage below does not change.
            if (program_is_bars) {
                fs_pattern_bars(program);
                fs_pattern_box(preview, tick * 6u);
            } else {
                fs_pattern_box(program, tick * 6u);
                fs_pattern_bars(preview);
            }
            program->pts = tick;
            preview->pts = tick;
            out->pts = tick;

            const float position =
                step.frames > 1 ? static_cast<float>(n) / static_cast<float>(step.frames - 1)
                                : 1.0f;

            const auto t0 = std::chrono::steady_clock::now();
            if (transition) {
                transition->apply(*program, *preview, *out, position);
            } else {
                fs_frame_copy(out, program);
            }
            const auto t1 = std::chrono::steady_clock::now();

            timings.push_back(
                std::chrono::duration<double, std::micro>(t1 - t0).count());

            if (sink != nullptr && fs_frame_write(out, sink) != 0) {
                std::perror("write");
                status = 1;
            }
            if (opt.realtime) {
                fs_clock_wait(&clock);
            }
        }

        if (step.swap_after) {
            program_is_bars = !program_is_bars;
        }
        if (status != 0) {
            break;
        }
    }

    if (sink != nullptr && sink != stdout) {
        std::fclose(sink);
    }

    if (status == 0) {
        std::printf("%s: %llu frames at %ux%u @ %.3f fps\n",
                    opt.bench ? "bench" : "render",
                    static_cast<unsigned long long>(tick), w, h, opt.fps);
        if (!opt.bench) {
            std::printf("wrote %s\n  ffplay -f rawvideo -pixel_format uyvy422"
                        " -video_size %ux%u -framerate %.3f %s\n",
                        opt.out.c_str(), w, h, opt.fps, opt.out.c_str());
        }
        report(timings, 1e6 / opt.fps, clock.late);
    }

    fs_pool_release(pool, out);
    fs_pool_release(pool, preview);
    fs_pool_release(pool, program);
    fs_pool_destroy(pool);
    return status;
}
