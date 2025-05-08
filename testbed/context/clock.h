#pragma once

#include <SDL3/SDL.h>

namespace testbed {

constexpr float NANOSECONDS_PER_SECOND = 1'000'000'000.f;

struct clock {
private:
    float previous_time = 0.f;
    float delta_time = 0.f;
public:
    void tick();
    [[nodiscard]] float delta() const noexcept;
};

}