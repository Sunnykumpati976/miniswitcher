// transition.hpp -- how one source becomes another on the program bus.
//
// A real switcher exposes a handful of transition types behind one control
// surface: the operator moves the T-bar from 0 to 1 and the engine does not
// care which type is loaded. That is the shape here -- Transition is the
// interface, position is the T-bar, and the mixer never branches on type.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "frame/frame.h"

namespace ms {

class Transition {
public:
    virtual ~Transition() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    // Composites `from` and `to` into `out` at T-bar `position`, clamped to
    // [0, 1]: 0 is fully `from`, 1 is fully `to`. All three frames must share
    // geometry. Must not allocate -- it runs inside the frame budget.
    virtual void apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
                       float position) const = 0;
};

// Hard take. The frame boundary does the work; there is nothing to interpolate.
class Cut final : public Transition {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "cut"; }
    void apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
               float position) const override;
};

// Dissolve: a per-sample linear blend. Blending Cb/Cr about their 128 offset
// is linear too, so the packed UYVY bytes can be mixed uniformly.
class Mix final : public Transition {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "mix"; }
    void apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
               float position) const override;
};

// Hard-edged wipe. The edge is the only thing that moves, which makes it the
// cheapest way to prove the T-bar is driving the engine.
class Wipe final : public Transition {
public:
    enum class Direction { Left, Right, Up, Down };

    explicit Wipe(Direction direction = Direction::Right) : direction_(direction) {}

    [[nodiscard]] std::string_view name() const noexcept override { return "wipe"; }
    void apply(const fs_frame &from, const fs_frame &to, fs_frame &out,
               float position) const override;

private:
    Direction direction_;
};

// Factory for the control protocol: "cut", "mix", "wipe", "wipe-left", ...
// Returns nullptr for an unknown name so the caller can reject the command
// rather than silently picking a default mid-show.
[[nodiscard]] std::unique_ptr<Transition> make_transition(std::string_view name);

[[nodiscard]] std::vector<std::string> transition_names();

} // namespace ms
