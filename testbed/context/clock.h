#pragma once

#include <SDL3/SDL.h>

namespace testbed {

struct clock {
private:
    float previous_time = 0.f;
public:
    float delta_time = 0.f;

    void tick();
};

}