#include "grunka.h"
#include "engines.h"

#pragma warning(push, 0)
#include <engine/action_binds.h>
#include <engine/engine.h>
#include <engine/input.h>
#include <engine/log.h>

#include <array.h>
#include <hash.h>
#include <memory.h>
#include <murmur_hash.h>

#include <atomic>
#include <inttypes.h>
#include <mutex>
#include <new>
#include <thread>

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>

#include <AK/Plugin/AkAudioInputSourceFactory.h>

#include "Win32/AkFilePackageLowLevelIOBlocking.h"

#if !defined AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include "Wwise_IDs.h"
#pragma warning(pop)

namespace {
const char *bankname_init = "Init.bnk";
const char *bankname_enemies = "Enemies.bnk";
const char *bankname_grunka = "Grunka.bnk";
std::atomic<uint64_t> game_object_count;
grunka::Grunka *grunka_singleton = nullptr;
}

namespace grunka {

/// Murmur hashed actions.
enum class ActionHash : uint64_t {
    NONE = 0x0ULL,
    QUIT = 0x387bbb994ac3551ULL,
};

// Forward declaration of functions.
namespace wwise {
void init(grunka::Grunka *grunka);
}

Grunka::Grunka(foundation::Allocator &allocator, const char *config_path)
: allocator(allocator)
, action_binds(nullptr)
, state(ApplicationState::None)
, lowlevel_io(nullptr)
, grunka_game_object_id(0)
, grunka_playing_id(0)
, engines(nullptr)
, play_sine(false) {
    action_binds = MAKE_NEW(allocator, engine::ActionBinds, allocator, config_path);
    engines = MAKE_NEW(allocator, grunka::engines::Engines, allocator);
    grunka_singleton = this;
}

Grunka::~Grunka() {
    MAKE_DELETE(allocator, ActionBinds, action_binds);
    MAKE_DELETE(allocator, Engines, engines);

#if !defined AK_OPTIMIZED
    AK::Comm::Term();
#endif

    if (AK::SoundEngine::IsInitialized()) {
        AK::SoundEngine::UnregisterAllGameObj();
        AK::SoundEngine::Term();
    }

    if (AK::IAkStreamMgr::Get()) {
        if (lowlevel_io) {
            lowlevel_io->Term();
            MAKE_DELETE(allocator, CAkFilePackageLowLevelIOBlocking, lowlevel_io);
        }

        AK::IAkStreamMgr::Get()->Destroy();
    }

    if (AK::MemoryMgr::IsInitialized()) {
        AK::MemoryMgr::Term();
    }
}

void update(engine::Engine &engine, void *grunka_object, float t, float dt) {
    (void)t;
    (void)dt;

    if (!grunka_object) {
        return;
    }

    Grunka *grunka = static_cast<Grunka *>(grunka_object);

    switch (grunka->state) {
    case ApplicationState::None: {
        transition(engine, grunka_object, ApplicationState::Initializing);
        break;
    }
    case ApplicationState::Running: {
        wwise::update();
        break;
    }
    case ApplicationState::Quitting: {
        if (grunka->engines->state == engines::EnginesState::None || grunka->engines->state == engines::EnginesState::Stopped) {
            transition(engine, grunka_object, ApplicationState::Terminate);
        }
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
    (void)engine;
    (void)grunka_object;
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
    case ApplicationState::Running: {
        if (grunka->grunka_game_object_id) {
            if (grunka->grunka_playing_id) {
                wwise::post_event(AK::EVENTS::STOP_GRUNKA, grunka->grunka_game_object_id);
                grunka->grunka_playing_id = 0;
            }

            wwise::unregister_game_object(grunka->grunka_game_object_id);
            grunka->grunka_game_object_id = 0;
        }

        if (grunka->engines) {
            engines::stop(*grunka->engines);
        }

        break;
    }
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
        wwise::init(grunka);
        transition(engine, grunka_object, ApplicationState::Running);
        break;
    }
    case ApplicationState::Running: {
        grunka->grunka_game_object_id = wwise::register_game_object("Grunka");
        wwise::post_event(AK::EVENTS::PLAY_GRUNKA, grunka->grunka_game_object_id);
        engines::start(*grunka->engines, grunka);
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

namespace wwise {
using namespace foundation::array;

void audio_input_execute_callback(AkPlayingID playing_id, AkAudioBuffer *buffer) {
    (void)playing_id;

    if (!grunka_singleton) {
        return;
    }

    const grunka::Grunka &grunka = *grunka_singleton;
    std::scoped_lock lock(*grunka.engines->mutex);
//    if (size(grunka.engines->data)) {
        AkSampleType *buf = buffer->GetChannel(0);
        for (int i = 0; i < 10; ++i) {
            buf[i] = rand();
        }
        buffer->uValidFrames = 10;
        buffer->eState = AK_DataReady;
    // } else {
    //     buffer->uValidFrames = 0;
    //     buffer->eState = AK_NoDataNeeded;
    // }
}

void audio_input_get_format_callback(AkPlayingID playing_id, AkAudioFormat &audio_format) {
    (void)playing_id;

    audio_format.SetAll(
        48000,
        AkChannelConfig(1, AK_SPEAKER_SETUP_MONO),
        16,
        2,
        AK_INT,
        AK_INTERLEAVED
    );
}

void init(grunka::Grunka *grunka) {
    assert(grunka);

    // MemoryMgr
    {
        AkMemSettings mem_settings;
        AK::MemoryMgr::GetDefaultSettings(mem_settings);
        AKRESULT result = AK::MemoryMgr::Init(&mem_settings);
        if (result != AK_Success) {
            log_fatal("Could not initialize AK::MemoryMgr: %d", result);
        }
    }

    // StreamMgr
    {
        AkStreamMgrSettings stm_settings;
        AK::StreamMgr::GetDefaultSettings(stm_settings);
        AK::IAkStreamMgr *stream_manager = AK::StreamMgr::Create(stm_settings);
        if (!stream_manager) {
            log_fatal("Could not create AK::StreamMgr");
        }

        AkDeviceSettings device_settings;
        AK::StreamMgr::GetDefaultDeviceSettings(device_settings);
        grunka->lowlevel_io = MAKE_NEW(grunka->allocator, CAkFilePackageLowLevelIOBlocking);
        AKRESULT result = grunka->lowlevel_io->Init(device_settings);
        if (result != AK_Success) {
            log_fatal("Could not initialize CAkFilePackageLowLevelIOBlocking: %d", result);
        }

        grunka->lowlevel_io->SetBasePath(AKTEXT("assets/grunka_wwise/GeneratedSoundBanks/Windows/"));
        AK::StreamMgr::SetCurrentLanguage(AKTEXT("English(US)"));
    }

    // SoundEngine
    {
        AkInitSettings init_settings;
        AkPlatformInitSettings platform_init_settings;
        AK::SoundEngine::GetDefaultInitSettings(init_settings);
        AK::SoundEngine::GetDefaultPlatformInitSettings(platform_init_settings);
        AKRESULT result = AK::SoundEngine::Init(&init_settings, &platform_init_settings);
        if (result != AK_Success) {
            log_fatal("Could not initialize AK::SoundEngine: %d", result);
        }
    }

#if !defined AK_OPTIMIZED
    // Communication
    {
        AkCommSettings comm_settings;
        AK::Comm::GetDefaultInitSettings(comm_settings);
        AKPLATFORM::SafeStrCpy(comm_settings.szAppNetworkName, "Grunka", AK_COMM_SETTINGS_MAX_STRING_SIZE);
        AKRESULT result = AK::Comm::Init(comm_settings);
        if (result != AK_Success) {
            log_fatal("Could not initialize AK::Comm: %d", result);
        }
    }
#endif

    // Load banks
    {
        auto load_bank = [](const char *bank_name) {
            AkBankID bank_id;
            AKRESULT result = AK::SoundEngine::LoadBank(bank_name, bank_id);
            if (result != AK_Success) {
                log_fatal("Could not load bank %s: %d", bank_name, result);
            }
        };

        load_bank(bankname_init);
        load_bank(bankname_enemies);
        load_bank(bankname_grunka);
    }

    // Listener
    {
        uint64_t listener_id = register_game_object("Default listener");
        AK::SoundEngine::SetDefaultListeners(&listener_id, 1);
    }

    // Setup AudioInput formats
    {
        SetAudioInputCallbacks(audio_input_execute_callback, audio_input_get_format_callback);
    }

    log_info("Wwise initialized");
}

void update() {
    if (AK::SoundEngine::IsInitialized()) {
        AK::SoundEngine::RenderAudio();
    }
}

uint64_t register_game_object(const char *name) {
    uint64_t id = game_object_count++;
    AK::SoundEngine::RegisterGameObj(id, name);
    return id;
}

void unregister_game_object(uint64_t game_object_id) {
    AK::SoundEngine::UnregisterGameObj(game_object_id);
}

uint32_t post_event(uint32_t event_id, uint64_t game_object_id) {
    AkPlayingID playing_id = AK::SoundEngine::PostEvent(event_id, game_object_id);
    if (playing_id == AK_INVALID_PLAYING_ID) {
        log_error("Could not post event %d for game object %" PRIu64 "", event_id, game_object_id);
    }

    return playing_id;
}

uint32_t post_event(const char *event_name, uint64_t game_object_id) {
    AkPlayingID playing_id = AK::SoundEngine::PostEvent(event_name, game_object_id);
    if (playing_id == AK_INVALID_PLAYING_ID) {
        log_error("Could not post event %s for game object %" PRIu64 "", event_name, game_object_id);
    }

    return playing_id;
}

} // namespace wwise

} // namespace grunka
