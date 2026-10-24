// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/dsp/filters/OnePoleFilters.*, VecOnePoleFilters.*)
// Copyright (C) 2018-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: SSE code rewritten as scalar C++.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DspUtils.h"

namespace tangier::dsp
{
/** One-pole low-pass filter: z = a * x + b * z. */
class OnePoleLowPass
{
public:
    void setSampleRate (float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        maxCutoffFreq = sampleRate * 0.5f - 2.0f;
        setCutoffFreq (cutoffFreq);
    }

    void setCutoffFreq (float freq) noexcept
    {
        cutoffFreq = std::min (freq, maxCutoffFreq);
        b = std::exp (-kTwoPi * cutoffFreq / sampleRate);
        a = 1.0f - b;
    }

    float process (float input) noexcept
    {
        z = a * input + z * b;
        return z;
    }

    void reset (float value = 0.0f) noexcept { z = value; }
    float getState() const noexcept { return z; }

private:
    float sampleRate = 44100.0f;
    float maxCutoffFreq = 22048.0f;
    float cutoffFreq = 1000.0f;
    float a = 1.0f, b = 0.0f, z = 0.0f;
};

/** One-pole high-pass filter, implemented as input minus a one-pole low-pass. */
class OnePoleHighPass
{
public:
    void setSampleRate (float newSampleRate) noexcept { lowPass.setSampleRate (newSampleRate); }

    /** Cutoff is clamped to at least 1 Hz, as in the original vector version. */
    void setCutoffFreq (float freq) noexcept { lowPass.setCutoffFreq (std::max (freq, 1.0f)); }

    float process (float input) noexcept { return input - lowPass.process (input); }
    void reset() noexcept { lowPass.reset(); }

private:
    OnePoleLowPass lowPass;
};
} // namespace tangier::dsp
