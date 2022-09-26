#pragma once

#pragma warning(push, 0)
#include <memory_types.h>
#include <stdint.h>
#pragma warning(pop)

class CAkFilePackageLowLevelIOBlocking;

namespace std {
template <typename T> struct atomic;
}

namespace grunka {
namespace wwise {

struct Wwise {
    Wwise(foundation::Allocator &allocator);
    ~Wwise();

    foundation::Allocator &allocator;
    CAkFilePackageLowLevelIOBlocking *lowlevel_io;
};

void update();
uint64_t register_game_object(const char *name);
uint32_t post_event(const char *event_name, uint64_t game_object_id);

} // namespace wwise
} // namespace grunka
