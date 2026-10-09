#include "core/transition.hpp"

#include <algorithm>
#include <cstring>

namespace ms {
namespace {

float clamp01(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }

std::size_t row_bytes(const fs_frame &f) noexcept
{
    return static_cast<std::size_t>(f.width) * FS_BYTES_PER_PIXEL;
}

bool same_geometry(const fs_frame &a, const fs_frame &b, const fs_frame &c) noexcept
{
    return a.width == b.width && a.width == c.width && a.height == b.height &&
           a.height == c.height;
}

void copy_into(const fs_frame &src, fs_frame &out) noexcept
{
    const std::size_t row = row_bytes(src);
    for (std::uint16_t y = 0; y < src.height; ++y) {
        std::memcpy(out.data + y * out.stride, src.data + y * src.stride, row);
    }
}

} // namespace

void Cut::apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
                float position) const
{
    if (!same_geometry(from, to, out)) {
        return;
    }
    copy_into(clamp01(position) >= 1.0f ? to : from, out);
}

void Mix::apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
                float position) const
{
    if (!same_geometry(from, to, out)) {
        return;
    }

    const float p = clamp01(position);
    if (p <= 0.0f) {
        copy_into(from, out);
        return;
    }
    if (p >= 1.0f) {
        copy_into(to, out);
        return;
    }

    // Fixed point: integer weights out of 256 keep the inner loop in integers
    // and let the compiler vectorise it.
    const unsigned wb = static_cast<unsigned>(p * 256.0f + 0.5f);
    const unsigned wa = 256u - wb;
    const std::size_t row = row_bytes(from);

    for (std::uint16_t y = 0; y < from.height; ++y) {
        const std::uint8_t *a = from.data + y * from.stride;
        const std::uint8_t *b = to.data + y * to.stride;
        std::uint8_t *o = out.data + y * out.stride;
        for (std::size_t i = 0; i < row; ++i) {
            o[i] = static_cast<std::uint8_t>((a[i] * wa + b[i] * wb + 128u) >> 8);
        }
    }
}

void Wipe::apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
                 float position) const
{
    if (!same_geometry(from, to, out)) {
        return;
    }

    const float p = clamp01(position);
    const std::size_t row = row_bytes(from);

    if (direction_ == Direction::Up || direction_ == Direction::Down) {
        const int edge = static_cast<int>(p * from.height + 0.5f);
        for (std::uint16_t y = 0; y < from.height; ++y) {
            const bool revealed = direction_ == Direction::Down
                                      ? y < edge
                                      : y >= from.height - edge;
            const fs_frame &src = revealed ? to : from;
            std::memcpy(out.data + y * out.stride, src.data + y * src.stride, row);
        }
        return;
    }

    // Horizontal: snap the edge to a pixel pair so a chroma sample is never
    // split between two sources.
    int edge = static_cast<int>(p * from.width + 0.5f) & ~1;
    edge = std::clamp(edge, 0, static_cast<int>(from.width));
    const std::size_t lead = static_cast<std::size_t>(edge) * FS_BYTES_PER_PIXEL;

    for (std::uint16_t y = 0; y < from.height; ++y) {
        const std::uint8_t *a = from.data + y * from.stride;
        const std::uint8_t *b = to.data + y * to.stride;
        std::uint8_t *o = out.data + y * out.stride;

        if (direction_ == Direction::Right) {
            std::memcpy(o, b, lead);
            std::memcpy(o + lead, a + lead, row - lead);
        } else {
            std::memcpy(o, a, row - lead);
            std::memcpy(o + (row - lead), b + (row - lead), lead);
        }
    }
}

std::unique_ptr<Transition> make_transition(std::string_view name)
{
    if (name == "cut") {
        return std::make_unique<Cut>();
    }
    if (name == "mix" || name == "dissolve") {
        return std::make_unique<Mix>();
    }
    if (name == "wipe" || name == "wipe-right") {
        return std::make_unique<Wipe>(Wipe::Direction::Right);
    }
    if (name == "wipe-left") {
        return std::make_unique<Wipe>(Wipe::Direction::Left);
    }
    if (name == "wipe-up") {
        return std::make_unique<Wipe>(Wipe::Direction::Up);
    }
    if (name == "wipe-down") {
        return std::make_unique<Wipe>(Wipe::Direction::Down);
    }
    return nullptr;
}

std::vector<std::string> transition_names()
{
    return { "cut", "mix", "wipe-right", "wipe-left", "wipe-up", "wipe-down" };
}

} // namespace ms
