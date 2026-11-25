#pragma once

#include <atomic>
#include <thread>

namespace govno::madness {

class ChiptuneSynth {
    std::atomic<bool> playing_{false};
    std::thread audio_thread_;

public:
    ~ChiptuneSynth();
    bool start();
    void stop();
    bool is_playing() const { return playing_.load(); }
};

} // namespace govno::madness
