#include "grunka.h"

#pragma warning(push, 0)
#include <engine/engine.h>

#include "Wwise_IDs.h"
#include <imgui.h>
#pragma warning(pop)

namespace grunka {

void test_window(engine::Engine &engine, Grunka &grunka, bool *show_window) {
    (void)engine;

    if (*show_window == false) {
        return;
    }

    const char *window_name = "Test Window###Testwindow";

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(window_name, show_window, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Controls");

    if (ImGui::Button("PLAY_IMP_IDLE")) {
        wwise::post_event(AK::EVENTS::PLAY_IMP_IDLE, grunka.grunka_game_object_id);
    }

    const char *play_sine_button_text = grunka.play_sine ? "Stop sine" : "Play sine";
    if (ImGui::Button(play_sine_button_text)) {
        grunka.play_sine = !grunka.play_sine;
    }

    ImGui::End();
}

void render_imgui(engine::Engine &engine, void *grunka_object) {
    if (!grunka_object) {
        return;
    }

    Grunka *grunka = static_cast<Grunka *>(grunka_object);

    static bool show_metrics_window = false;
    static bool show_test_window = true;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            if (ImGui::MenuItem("Show Metrics", nullptr, show_metrics_window)) {
                show_metrics_window = !show_metrics_window;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (show_metrics_window) {
        ImGui::ShowMetricsWindow();
    }

    test_window(engine, *grunka, &show_test_window);
}

} // namespace grunka
