#pragma once

#pragma warning(push, 0)
#include <collection_types.h>
#include <memory_types.h>
#include <stdint.h>
#pragma warning(pop)

namespace engine {
struct Engine;
struct InputCommand;
struct ActionBinds;
} // namespace engine

namespace grunka {

namespace wwise {
struct Wwise;
}

/**
 * @brief An enum that describes a specific application state.
 *
 */
enum class ApplicationState {
    // No state.
    None,

    // Creating application, or loading from a save.
    Initializing,

    // Running
    Running,

    // Shutting down, saving and closing the app.
    Quitting,

    // Final state that signals the engine to terminate the application.
    Terminate,
};

struct Grunka {
    Grunka(foundation::Allocator &allocator, const char *config_path);
    ~Grunka();

    foundation::Allocator &allocator;
    engine::ActionBinds *action_binds;
    ApplicationState state;
    wwise::Wwise *wwise;
    uint64_t sound_game_object_id;
};

/**
 * @brief Updates the Grunka
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to update
 * @param t The current time
 * @param dt The delta time since last update
 */
void update(engine::Engine &engine, void *grunka_object, float t, float dt);

/**
 * @brief Callback to the Grunka that an input has ocurred.
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to signal.
 * @param input_command The input command.
 */
void on_input(engine::Engine &engine, void *grunka_object, engine::InputCommand &input_command);

/**
 * @brief Renders the Grunka
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to render
 */
void render(engine::Engine &engine, void *grunka_object);

/**
 * @brief Renders the imgui
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to render
 */
void render_imgui(engine::Engine &engine, void *grunka_object);

/**
 * @brief Asks the Grunka to cleanup.
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to cleanup
 */
void on_shutdown(engine::Engine &engine, void *grunka_object);

/**
 * @brief Transition a Grunka to another application state.
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to transition
 * @param application_state The ApplicationState to transition to.
 */
void transition(engine::Engine &engine, void *grunka_object, ApplicationState application_state);

} // namespace grunka
