#include "wwise.h"

#pragma warning(push, 0)
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>

#include <AK/Plugin/AkAudioInputSourceFactory.h>

#if !defined AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include <atomic>
#include <inttypes.h>
#include <memory.h>
#include <new>

#include "Win32/AkFilePackageLowLevelIOBlocking.h"

#include <engine/log.h>
#pragma warning(pop)

namespace {
const char *bankname_init = "Init.bnk";
const char *bankname_enemies = "Enemies.bnk";
const char *bankname_grunka = "Grunka.bnk";
const AkGameObjectID LISTENER_ID = 10000;
std::atomic<uint64_t> game_object_count;
} // namespace

namespace grunka {
namespace wwise {

void audio_input_execute_callback(AkPlayingID playing_id, AkAudioBuffer *buffer) {
    (void)playing_id;
    buffer->uValidFrames = 0;
    buffer->eState = AK_NoDataNeeded;
    // continue at SoundInput.cpp ::Execute
}

void audio_input_get_format_callback(AkPlayingID playing_id, AkAudioFormat &audio_format) {
    (void)playing_id;
    (void)audio_format;
}

Wwise::Wwise(foundation::Allocator &allocator)
: allocator(allocator)
, lowlevel_io(nullptr) {
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
        lowlevel_io = MAKE_NEW(allocator, CAkFilePackageLowLevelIOBlocking);
        AKRESULT result = lowlevel_io->Init(device_settings);
        if (result != AK_Success) {
            log_fatal("Could not initialize CAkFilePackageLowLevelIOBlocking: %d", result);
        }

        lowlevel_io->SetBasePath(AKTEXT("assets/grunka_wwise/GeneratedSoundBanks/Windows/"));
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

Wwise::~Wwise() {
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
