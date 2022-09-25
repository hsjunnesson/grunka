#include "grunka.h"
#include "wwise.h"

#pragma warning(push, 0)
#include <engine/action_binds.h>
#include <engine/engine.h>
#include <engine/input.h>
#include <engine/log.h>

#include <array.h>
#include <hash.h>
#include <memory.h>
#include <murmur_hash.h>
#pragma warning(pop)

namespace grunka {

/// Murmur hashed actions.
enum class ActionHash : uint64_t {
    NONE = 0x0ULL,
    QUIT = 0x387bbb994ac3551ULL,
};

Grunka::Grunka(foundation::Allocator &allocator, const char *config_path)
: allocator(allocator)
, action_binds(nullptr)
, state(ApplicationState::None)
, wwise(nullptr) {
    action_binds = MAKE_NEW(allocator, engine::ActionBinds, allocator, config_path);
}

Grunka::~Grunka() {
    MAKE_DELETE(allocator, ActionBinds, action_binds);
    MAKE_DELETE(allocator, Wwise, wwise);
}

void update(engine::Engine &engine, void *grunka_object, float t, float dt) {
    if (!grunka_object) {
        return;
    }

    Grunka *grunka = static_cast<Grunka *>(grunka_object);

    switch (grunka->state) {
    case ApplicationState::None: {
        transition(engine, grunka_object, ApplicationState::Initializing);
        break;
    }
    case ApplicationState::Quitting: {
        transition(engine, grunka_object, ApplicationState::Terminate);
        break;
    }
    default: {
        break;
    }
    }
}

void on_input(engine::Engine &engine, void *grunka_object, engine::InputCommand &input_command) {
    if (!grunka_object) {
        return;
    }

    Grunka *grunka = static_cast<Grunka *>(grunka_object);
    assert(grunka->action_binds != nullptr);

    if (input_command.input_type == engine::InputType::Key) {
        bool pressed = input_command.key_state.trigger_state == engine::TriggerState::Pressed;
        engine::ActionBindsBind bind = engine::bind_for_keycode(input_command.key_state.keycode);
        if (bind == engine::ActionBindsBind::NOT_FOUND) {
            log_error("ActionBind not found for keycode %d", input_command.key_state.keycode);
            return;
        }

        uint64_t bind_key = static_cast<uint64_t>(bind);
        ActionHash action_hash = ActionHash(foundation::hash::get(grunka->action_binds->bind_actions, bind_key, (uint64_t)0));

        switch (action_hash) {
        case ActionHash::QUIT: {
            if (pressed) {
                transition(engine, grunka_object, ApplicationState::Quitting);
            }
            break;
        }
        default:
            break;
        }
    }
}

void render(engine::Engine &engine, void *grunka_object) {
}

void on_shutdown(engine::Engine &engine, void *grunka_object) {
    transition(engine, grunka_object, ApplicationState::Quitting);
}

void transition(engine::Engine &engine, void *grunka_object, ApplicationState application_state) {
    if (!grunka_object) {
        return;
    }

    Grunka *grunka = (Grunka *)grunka_object;

    if (grunka->state == application_state) {
        return;
    }

    // When leaving a application state
    switch (grunka->state) {
    case ApplicationState::Terminate: {
        return;
    }
    default:
        break;
    }

    grunka->state = application_state;

    // When entering a new application state
    switch (grunka->state) {
    case ApplicationState::None: {
        break;
    }
    case ApplicationState::Initializing: {
        log_info("Initializing");
        assert(grunka->wwise == nullptr);
        grunka->wwise = MAKE_NEW(grunka->allocator, wwise::Wwise, grunka->allocator);
        transition(engine, grunka_object, ApplicationState::Running);
        break;
    }
    case ApplicationState::Running: {
        log_info("Playing");
        break;
    }
    case ApplicationState::Quitting: {
        log_info("Quitting");
        break;
    }
    case ApplicationState::Terminate: {
        log_info("Terminating");
        engine::terminate(engine);
        break;
    }
    }
}

} // namespace grunka
