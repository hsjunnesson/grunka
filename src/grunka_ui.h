#pragma once

namespace engine {
struct Engine;
} // namespace engine

namespace grunka {

/**
 * @brief Renders the imgui
 *
 * @param engine The engine which calls this function
 * @param grunka_object The Grunka to render
 */
void render_imgui(engine::Engine &engine, void *grunka_object);

} // namespace grunka
