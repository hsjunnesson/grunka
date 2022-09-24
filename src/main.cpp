#include "grunka.h"
#include "grunka_ui.h"

#pragma warning(push, 0)
#define RND_IMPLEMENTATION
#include "rnd.h"

#include <engine/engine.h>
#include <engine/input.h>
#include <engine/log.h>

#include <backward.hpp>
#include <memory.h>

#if defined(LIVE_PP)
#include <Windows.h>

#include "LPP_API.h"
#endif

#if defined(SUPERLUMINAL)
#include <Superluminal/PerformanceAPI.h>
#endif

#pragma warning(pop)

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

#if defined(SUPERLUMINAL)
    PerformanceAPI_SetCurrentThreadName("main");
#endif

    // Validate platform
    {
        unsigned int x = 1;
        char *c = (char *)&x;
        if ((int)*c == 0) {
            log_fatal("Unsupported platform: big endian");
        }

        if constexpr (sizeof(char) != sizeof(uint8_t)) {
            log_fatal("Unsupported platform: invalid char size");
        }

        if constexpr (sizeof(float) != 4) {
            log_fatal("Unsupported platform: invalid float size");
        }
    }

#if defined(LIVE_PP)
    HMODULE livePP = lpp::lppLoadAndRegister(L"LivePP", "AGroupName");
    lpp::lppEnableAllCallingModulesSync(livePP);
#endif

    int status = 0;

    backward::SignalHandling sh;

    foundation::memory_globals::init();

    foundation::Allocator &allocator = foundation::memory_globals::default_allocator();

    {
        const char *config_path = "assets/config.ini";

        engine::Engine engine(allocator, config_path);
        grunka::Grunka grunka(allocator, config_path);

        engine::EngineCallbacks engine_callbacks;
        engine_callbacks.on_input = grunka::on_input;
        engine_callbacks.update = grunka::engine_update;
        engine_callbacks.render = grunka::render;
        engine_callbacks.render_imgui = grunka::render_imgui;
        engine_callbacks.on_shutdown = grunka::on_shutdown;
        engine.engine_callbacks = &engine_callbacks;
        engine.game_object = &grunka;

#if defined(LIVE_PP)
//        game.livePP = livePP;
#endif

        status = engine::run(engine);
    }

    foundation::memory_globals::shutdown();

#if defined(LIVE_PP)
    lpp::lppShutdown(livePP);
    ::FreeLibrary(livePP);
#endif

    return status;
}
