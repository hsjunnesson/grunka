#pragma once

#pragma warning(push, 0)
#include <collection_types.h>
#include <memory_types.h>

#include <stdint.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#pragma warning(pop)

namespace std {
class thread;
class mutex;
template <typename T>
struct atomic;
} // namespace std

namespace grunka {
namespace engines {

enum class EnginesState {
    // No state
    None,

    // Is running
    Running,

    // Is stopped
    Stopped,
};

struct Engines {
    Engines(foundation::Allocator &allocator);
    ~Engines();

    foundation::Allocator &allocator;
    EnginesState state;

    foundation::Array<AkReal32> data;

    std::mutex *mutex;
    std::thread *thread;
    std::atomic<bool> *stop;
};

/// @brief Starts the engines.
/// @param engines The engines to start.
/// @param grunka_object Pointer to the grunka::Grunka object.
void start(Engines &engines, void *grunka_object);

/// @brief Stops the engines, cleans things up.
/// @param engines The engines to stop.
/// This is a blocking call. There's no restarting it after this.
void stop(Engines &engines);

} // namespace engines
} // namespace grunka
