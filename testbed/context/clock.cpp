#include "context/clock.h"

namespace testbed {

void clock::tick() {
    auto current_time = static_cast<float>(SDL_GetTicksNS()) / 1000000000.f;
    delta_time = current_time - previous_time;
    previous_time = current_time;
}

}