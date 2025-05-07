#pragma once

#include <iostream>
#include <system/movement_system.h>
#include "component/movement_component.h"
#include "component/position_component.h"
#include "component/renderable_component.h"
#include "context/clock.h"

namespace testbed {

void movement_system(entt::registry& registry) {
    auto* key_down = SDL_GetKeyboardState(nullptr);
    if(!key_down) {
        return;
    }

    auto view = registry.view<position_component, movement_component>();
    for (auto&& [ent, pos, movement] : view.each()) {
        auto delta = registry.ctx().get<clock>().delta_time * movement.speed;
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