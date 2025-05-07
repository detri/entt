#pragma once

#include <system/movement_system.h>
#include "component/position_component.h"
#include "component/renderable_component.h"

namespace testbed {

void movement_system(entt::registry& registry) {
    auto* key_down = SDL_GetKeyboardState(nullptr);

    auto display = SDL_GetPrimaryDisplay();
    auto refresh_rate = SDL_GetCurrentDisplayMode(display)->refresh_rate;
    float delta;
    if (refresh_rate == 0.0f) {
        delta = 1.f / 60.f;
    } else {
        delta = 1.f / refresh_rate;
    }

    auto speed = 2.f;
    delta *= speed;

    auto view = registry.view<position_component, renderable_component>();
    for (auto&& [ent, pos] : view.each()) {
        if ( key_down != nullptr) {
            if (key_down[SDL_SCANCODE_W]) {
                pos.y -= delta;
            } else if (key_down[SDL_SCANCODE_S]) {
                pos.y += delta;
            }
            if (key_down[SDL_SCANCODE_A]) {
                pos.x -= delta;
            } else if (key_down[SDL_SCANCODE_D]) {
                pos.x += delta;
            }
        }
    }
}

}