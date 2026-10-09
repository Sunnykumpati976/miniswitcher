// Transitions: endpoints must be exact. A dissolve that does not land on a
// clean source at position 1.0 shows up on air as a dirty take.
#include <cassert>
#include <cstdio>
#include <cstring>

#include "core/transition.hpp"
#include "frame/pattern.h"

namespace {

bool identical(const fs_frame &a, const fs_frame &b)
{
    for (std::uint16_t y = 0; y < a.height; ++y) {
        if (std::memcmp(a.data + y * a.stride, b.data + y * b.stride,
                        static_cast<std::size_t>(a.width) * FS_BYTES_PER_PIXEL) != 0) {
            return false;
        }
    }
    return true;
}

} // namespace

int main()
{
    fs_pool *pool = fs_pool_create(3u, 64u, 16u);
    assert(pool != nullptr);

    fs_frame *from = fs_pool_acquire(pool);
    fs_frame *to = fs_pool_acquire(pool);
    fs_frame *out = fs_pool_acquire(pool);
    assert(from && to && out);

    fs_fill_flat(from, 40u, 128u, 128u);
    fs_fill_flat(to, 200u, 128u, 128u);

    // Factory contract: unknown names are rejected, not defaulted.
    assert(ms::make_transition("nope") == nullptr);
    assert(ms::make_transition("mix") != nullptr);
    assert(ms::make_transition("dissolve") != nullptr);
    assert(ms::transition_names().size() == 6u);

    const ms::Cut cut;
    cut.apply(*from, *to, *out, 0.0f);
    assert(identical(*out, *from));
    cut.apply(*from, *to, *out, 0.99f);
    assert(identical(*out, *from)); // a cut only lands at the end
    cut.apply(*from, *to, *out, 1.0f);
    assert(identical(*out, *to));

    const ms::Mix mix;
    mix.apply(*from, *to, *out, 0.0f);
    assert(identical(*out, *from));
    mix.apply(*from, *to, *out, 1.0f);
    assert(identical(*out, *to));
    mix.apply(*from, *to, *out, -5.0f); // out-of-range clamps, never reads wild
    assert(identical(*out, *from));
    mix.apply(*from, *to, *out, 5.0f);
    assert(identical(*out, *to));

    mix.apply(*from, *to, *out, 0.5f);
    const int luma = out->data[1];
    assert(luma > 40 && luma < 200);          // genuinely between the sources
    assert(luma > 110 && luma < 130);         // and near the midpoint
    assert(out->data[0] == 128u);             // neutral chroma stays neutral

    // A dissolve must be monotonic: the T-bar never goes backwards on screen.
    int previous = 0;
    for (int step = 0; step <= 10; ++step) {
        mix.apply(*from, *to, *out, static_cast<float>(step) / 10.0f);
        const int current = out->data[1];
        assert(current >= previous);
        previous = current;
    }

    const ms::Wipe wipe(ms::Wipe::Direction::Right);
    wipe.apply(*from, *to, *out, 0.0f);
    assert(identical(*out, *from));
    wipe.apply(*from, *to, *out, 1.0f);
    assert(identical(*out, *to));

    wipe.apply(*from, *to, *out, 0.5f);
    assert(out->data[1] == 200u);                               // left: new
    assert(out->data[(out->width - 1) * FS_BYTES_PER_PIXEL + 1] == 40u); // right: old

    const ms::Wipe down(ms::Wipe::Direction::Down);
    down.apply(*from, *to, *out, 0.5f);
    assert(out->data[1] == 200u);                          // top row revealed
    assert(out->data[(out->height - 1) * out->stride + 1] == 40u);

    fs_pool_destroy(pool);
    std::printf("test_transition: ok\n");
    return 0;
}
