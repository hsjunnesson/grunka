#include "engines.h"
#include "grunka.h"

#include "engine/log.h"

#pragma warning(push, 0)
#include <memory.h>

#include <assert.h>
#include <mutex>

#include <atomic>
#include <chrono>
#include <thread>
#pragma warning(pop)

namespace grunka {
namespace engines {

void thread_function(grunka::engines::Engines *engines, grunka::Grunka *grunka) {
    using namespace std::chrono_literals;

    while (true) {
        if (*engines->stop) {
            break;
        }

        std::this_thread::sleep_for(10ms);
    }

    {
        std::scoped_lock lock(*engines->mutex);
        engines->state = EnginesState::Stopped;
    }
}

Engines::Engines(foundation::Allocator &allocator)
: allocator(allocator)
, state(EnginesState::None)
, mutex(nullptr)
, thread(nullptr)
, stop(nullptr) {
    mutex = MAKE_NEW(allocator, std::mutex);
    stop = MAKE_NEW(allocator, std::atomic<bool>, false);
}

Engines::~Engines() {
    if (thread) {
        std::scoped_lock lock(*mutex);
        MAKE_DELETE(allocator, thread, thread);
    }

    MAKE_DELETE(allocator, mutex, mutex);
    MAKE_DELETE(allocator, atomic, stop);
}

void start(Engines &engines, void *grunka_object) {
    assert(engines.state == EnginesState::None);
    assert(engines.thread == nullptr);
    assert(grunka_object);

    grunka::Grunka *grunka = static_cast<grunka::Grunka *>(grunka_object);

    engines.thread = MAKE_NEW(engines.allocator, std::thread, thread_function, &engines, grunka);
    engines.thread->detach();
    engines.state = EnginesState::Running;
}

void stop(Engines &engines) {
    assert(engines.state == EnginesState::Running);
    std::scoped_lock lock(*engines.mutex);
    *engines.stop = true;
}

} // namespace engines
} // namespace grunka
