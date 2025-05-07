#include <component/input_listener_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/core/hashed_string.hpp>
#include <entt/davey/meta.hpp>
#include <entt/meta/factory.hpp>

#include <meta/meta.h>
#include "component/movement_component.h"

namespace testbed {

void meta_setup() {
    using namespace entt::literals;

    entt::meta_factory<input_listener_component>()
        .custom<entt::davey_data>("input listener")
        .data<&input_listener_component::command>("command"_hs)
        .custom<entt::davey_data>("command");

    entt::meta_factory<position_component>()
        .custom<entt::davey_data>("position")
        .data<&SDL_FPoint::x>("x"_hs)
        .custom<entt::davey_data>("x")
        .data<&SDL_FPoint::y>("y"_hs)
        .custom<entt::davey_data>("y");

    entt::meta_factory<movement_component>()
        .custom<entt::davey_data>("movement")
        .data<&movement_component::speed>("speed"_hs)
        .custom<entt::davey_data>("speed");

    entt::meta_factory<rect_component>()
        .custom<entt::davey_data>("rect")
        .data<&rect_component::x>("x"_hs)
        .custom<entt::davey_data>("x")
        .data<&rect_component::y>("y"_hs)
        .custom<entt::davey_data>("y")
        .data<&rect_component::w>("w"_hs)
        .custom<entt::davey_data>("w")
        .data<&rect_component::h>("h"_hs)
        .custom<entt::davey_data>("h");

    entt::meta_factory<renderable_component>()
        .custom<entt::davey_data>("no data");

    // bind components...
}

} // namespace testbed
