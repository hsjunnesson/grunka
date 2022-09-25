#include "wwise.h"

#pragma warning(push, 0)
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include <memory.h>

#include "Win32/AkFilePackageLowLevelIOBlocking.h"

#include <engine/log.h>
#pragma warning(pop)

namespace grunka {
namespace wwise {

Wwise::Wwise(foundation::Allocator &allocator)
: allocator(allocator)
, stream_manager(nullptr)
, lowlevel_io(nullptr)
{
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
        stream_manager = AK::StreamMgr::Create(stm_settings);
        if (!stream_manager) {
            log_fatal("Could not create AK::StreamMgr");
        }

        AkDeviceSettings device_settings;
        AK::StreamMgr::GetDefaultDeviceSettings(device_settings);
        lowlevel_io = new CAkFilePackageLowLevelIOBlocking();
        AKRESULT result = lowlevel_io->Init(device_settings);
        if (result != AK_Success) {
            log_fatal("Could not initialize CAkFilePackageLowLevelIOBlocking: %d", result);
        }
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

    log_info("Wwise initialized");
}

Wwise::~Wwise() {
    if (AK::SoundEngine::IsInitialized()) {
        AK::SoundEngine::Term();
    }

    if (AK::IAkStreamMgr::Get()) {
        if (lowlevel_io) {
            lowlevel_io->Term();
            delete lowlevel_io;
        }

        AK::IAkStreamMgr::Get()->Destroy();
    }

    if (AK::MemoryMgr::IsInitialized()) {
        AK::MemoryMgr::Term();
    }
}

} // namespace wwise
} // namespace grunka
