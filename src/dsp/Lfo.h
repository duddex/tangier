// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/dsp/modulation/DLFO.hpp)
// Copyright (C) 2018-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: minor clean-up for use outside VCV Rack.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Noise.h"

#include <array>
#include <numbers>

namespace tangier::dsp
{
/** LFO with seven simultaneous outputs, all in the range -1..1. */
class Lfo
{
public:
    enum Wave
    {
        sine = 0,
        triangle,
        sawUp,
        sawDown,
        square,
        sampleAndHold,
        noise,
        numWaves
    };

    Lfo()
    {
        setSampleRate (44100.0f);
        setFrequency (0.75f);
        out.fill (0.0f);
        out[sampleAndHold] = whiteNoise.getValue();
    }

    void setSampleRate (float sampleRate) noexcept
    {
        invSampleRate = 1.0f / sampleRate;
        stepSize = freq * invSampleRate;
    }

    void setFrequency (float newFreq) noexcept
    {
        freq = newFreq;
        stepSize = freq * invSampleRate;
    }

    /** Restarts the LFO cycle. */
    void resetPhase() noexcept { step = 0.0f; }

    void process() noexcept
    {
        float tri = (step < 0.5f) ? step : (1.0f - step);
        tri = -(tri * 4.0f - 1.0f);
        out[triangle] = tri;

        // Parabolic sine approximation driven by the triangle
        const float x = tri * 0.5f * std::numbers::pi_v<float>;
        const float xx = x * x;
        out[sine] = (x < 0.0f) ? (kA * x + kB * xx) : (kA * x - kB * xx);

        float saw = step - 0.25f;
        if (saw < 0.0f)
            saw += 1.0f;
        saw = saw * 2.0f - 1.0f;
        out[sawUp] = saw;
        out[sawDown] = -saw;

        out[square] = (saw > 0.0f) ? 1.0f : -1.0f;
        out[noise] = whiteNoise.process();

        if (out[square] > 0.5f && ! shTriggered)
        {
            shTriggered = true;
            out[sampleAndHold] = whiteNoise.getValue();
        }
        else if (out[square] < 0.5f)
        {
            shTriggered = false;
        }

        step += stepSize;
        if (step > 1.0f)
            step -= 1.0f;
    }

    float getOutput (int wave) const noexcept { return out[static_cast<size_t> (wave)]; }

private:
    static constexpr float kA = 1.27323954f;
    static constexpr float kB = 0.405284735f;

    float freq = 0.75f, invSampleRate = 1.0f / 44100.0f;
    float step = 0.0f, stepSize = 0.0f;
    bool shTriggered = false;
    WhiteNoise whiteNoise;
    std::array<float, numWaves> out {};
};
} // namespace tangier::dsp
