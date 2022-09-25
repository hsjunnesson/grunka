#pragma once

#pragma warning(push, 0)
#include <memory_types.h>
#pragma warning(pop)

class CAkFilePackageLowLevelIOBlocking;

namespace AK {
class IAkStreamMgr;
}

namespace grunka {
namespace wwise {

struct Wwise {
    Wwise(foundation::Allocator &allocator);
    ~Wwise();

    foundation::Allocator &allocator;
    AK::IAkStreamMgr *stream_manager;
    CAkFilePackageLowLevelIOBlocking *lowlevel_io;
};

} // namespace wwise
} // namespace grunka
