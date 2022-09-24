#pragma once

#pragma warning(push, 0)
#include <memory_types.h>
#include <collection_types.h>
#include <stdint.h>
#pragma warning(pop)

namespace engine {
struct Engine;
struct InputCommand;
struct ActionBinds;
} // namespace engine


namespace grunka {

struct Grunka {
    Grunka(foundation::Allocator &allocator, const char *config_path);
    ~Grunka();

    foundation::Allocator &allocator;
    engine::ActionBinds *action_binds;
};

/**
 * @brief Updates the Grunka
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to update
 * @param t The current time
 * @param dt The delta time since last update
 */
void engine_update(engine::Engine &engine, void *grunk_object, float t, float dt);

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

} // namespace grunka
