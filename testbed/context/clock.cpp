#include "context/clock.h"

namespace testbed {

void clock::tick() {
    const auto current_time = static_cast<float>(SDL_GetTicksNS()) / NANOSECONDS_PER_SECOND;
    delta_time = current_time - previous_time;
    previous_time = current_time;
}

float clock::delta() const noexcept {
    return delta_time;
}

} // namespace testbed