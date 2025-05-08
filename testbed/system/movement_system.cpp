#pragma once

#include <system/movement_system.h>
#include "component/movement_component.h"
#include "component/position_component.h"
#include "context/clock.h"

namespace testbed {

namespace {
bool is_key_down(const bool *keys, SDL_Scancode code) {
    if(keys == nullptr) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return keys[code];
}
}

void movement_system(entt::registry &registry) {
    const auto *keys = SDL_GetKeyboardState(nullptr);
    if(keys == nullptr) {
        return;
    }

    auto view = registry.view<position_component, movement_component>();
    for(auto &&[ent, pos, movement]: view.each()) {
        const auto delta = registry.ctx().get<clock>().delta() * movement.speed;

        if(is_key_down(keys, SDL_SCANCODE_W)) {
            pos.y -= delta;
        } else if(is_key_down(keys, SDL_SCANCODE_S)) {
            pos.y += delta;
        }
        if(is_key_down(keys, SDL_SCANCODE_A)) {
            pos.x -= delta;
        } else if(is_key_down(keys, SDL_SCANCODE_D)) {
            pos.x += delta;
        }
    }
}

} // namespace testbed