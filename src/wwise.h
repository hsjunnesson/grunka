#pragma once

#pragma warning(push, 0)
#include <memory_types.h>
#include <stdint.h>
#pragma warning(pop)

class CAkFilePackageLowLevelIOBlocking;

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
void unregister_game_object(uint64_t game_object_id);

uint32_t post_event(uint32_t event_id, uint64_t game_object_id);
uint32_t post_event(const char *event_name, uint64_t game_object_id);

} // namespace wwise
} // namespace grunka
