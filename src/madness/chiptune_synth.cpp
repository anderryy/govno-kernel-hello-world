#include "madness/chiptune_synth.hpp"

#include <windows.h>
#include <mmsystem.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace govno::madness {

ChiptuneSynth::~ChiptuneSynth() {
    stop();
}

bool ChiptuneSynth::start() {
    if (playing_.load()) return true;
    if (waveOutGetNumDevs() == 0) return false;

    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 22050;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.nAvgBytesPerSec = 44100;

    HWAVEOUT hwo = nullptr;
    if (waveOutOpen(&hwo, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        return false;
    }

    playing_.store(true);
    audio_thread_ = std::thread([this, hwo]() {
        constexpr int SR = 22050;
        constexpr int CHUNK_SAMPLES = 2048;

        const double lead_notes[] = {
            440.0, 523.25, 659.25, 880.0,  659.25, 523.25, 440.0, 523.25,
            392.0, 493.88, 587.33, 783.99, 587.33, 493.88, 392.0, 493.88,
            349.23, 440.0, 523.25, 698.46, 523.25, 440.0, 349.23, 440.0,
            329.63, 415.30, 493.88, 659.25, 493.88, 415.30, 329.63, 659.25
        };
        const double bass_notes[] = {110.0, 110.0, 98.0, 98.0, 87.31, 87.31, 82.41, 82.41};

        std::array<std::vector<int16_t>, 2> bufs;
        bufs[0].resize(CHUNK_SAMPLES);
        bufs[1].resize(CHUNK_SAMPLES);
        WAVEHDR hdrs[2]{};

        uint64_t sample_idx = 0;
        double phase_lead = 0.0, phase_bass = 0.0, phase_modem = 0.0;
        constexpr double TWO_PI = 6.283185307179586;

        auto fill_buf = [&](int b) {
            for (int i = 0; i < CHUNK_SAMPLES; ++i, ++sample_idx) {
                double mix = 0.0;
                // First 1.4 seconds: 56k Dial-Up Modem Handshake screech intro!
                if (sample_idx < SR * 7 / 5) {
                    double f_mod = 1200.0 + ((sample_idx / 900) % 5) * 430.0 + ((sample_idx / 170) % 2) * 320.0;
                    phase_modem += TWO_PI * f_mod / SR;
                    if (phase_modem > TWO_PI) phase_modem -= TWO_PI;
                    double carrier = std::sin(phase_modem);
                    double noise = (sample_idx > SR * 9 / 10)
                        ? (static_cast<double>(((sample_idx * 1103515245u + 12345u) >> 16) & 0xFF) / 128.0 - 1.0)
                        : 0.0;
                    mix = (carrier * 0.55 + noise * 0.35) * 0.11;
                } else {
                    int step = static_cast<int>(sample_idx / (SR / 8)) % 32;
                    int bstep = (step / 4) % 8;
                    phase_lead += TWO_PI * lead_notes[step] / SR;
                    phase_bass += TWO_PI * bass_notes[bstep] / SR;
                    if (phase_lead > TWO_PI) phase_lead -= TWO_PI;
                    if (phase_bass > TWO_PI) phase_bass -= TWO_PI;
                    double sq = (std::sin(phase_lead) >= 0.0) ? 1.0 : -1.0;
                    double bass = std::sin(phase_bass);
                    double env = 1.0 - static_cast<double>(sample_idx % (SR / 8)) / static_cast<double>(SR / 8);
                    mix = (sq * 0.45 * env + bass * 0.55) * 0.14;
                }
                bufs[b][i] = static_cast<int16_t>(mix * 32767.0);
            }
            ZeroMemory(&hdrs[b], sizeof(WAVEHDR));
            hdrs[b].lpData = reinterpret_cast<LPSTR>(bufs[b].data());
            hdrs[b].dwBufferLength = static_cast<DWORD>(bufs[b].size() * sizeof(int16_t));
            waveOutPrepareHeader(hwo, &hdrs[b], sizeof(WAVEHDR));
            waveOutWrite(hwo, &hdrs[b], sizeof(WAVEHDR));
        };

        fill_buf(0);
        fill_buf(1);
        int cur = 0;
        while (playing_.load()) {
            if (hdrs[cur].dwFlags & WHDR_DONE) {
                waveOutUnprepareHeader(hwo, &hdrs[cur], sizeof(WAVEHDR));
                fill_buf(cur);
                cur = 1 - cur;
            } else {
                Sleep(15);
            }
        }

        waveOutReset(hwo);
        for (int b = 0; b < 2; ++b) {
            if (hdrs[b].dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(hwo, &hdrs[b], sizeof(WAVEHDR));
        }
        waveOutClose(hwo);
    });
    return true;
}

void ChiptuneSynth::stop() {
    if (!playing_.exchange(false)) {
        if (audio_thread_.joinable()) audio_thread_.join();
        return;
    }
    if (audio_thread_.joinable()) audio_thread_.join();
}

} // namespace govno::madness
