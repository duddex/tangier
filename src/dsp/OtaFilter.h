// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/dsp/filters/VecOTAFilter.*)
// Copyright (C) 2018-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: SSE code rewritten as scalar C++;
// the filter coefficient is computed directly instead of using a look-up table.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DspUtils.h"

#include <array>

namespace tangier::dsp
{
/** Zero-delay-feedback (TPT) one-pole stage with soft saturation. */
class TptOnePoleStage
{
public:
    float process (float in) noexcept
    {
        const float v = (driveSignal (in) - z) * G;
        const float out = driveSignal (v + z);
        z = out + v;
        return out;
    }

    float G = 0.0f;
    float z = 0.0f;
};

/** Four-stage OTA ladder low-pass filter, switchable between 2 and 4 poles. */
class OtaFilter
{
public:
    enum Mode
    {
        lowPass2 = 0,
        lowPass4
    };

    OtaFilter() { setMode (lowPass4); }

    void setSampleRate (float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        setCutoff (pitch);
    }

    /** Cutoff as a 0..10 V/oct value: 5 = 440 Hz, 0 = 13.75 Hz, 10 = 14080 Hz. */
    void setCutoff (float newPitch) noexcept
    {
        pitch = std::clamp (newPitch, 0.0f, 10.0f);
        const float freq = std::min (440.0f * std::exp2 (pitch - 5.0f), sampleRate * 0.49f);
        const float g = std::tan (kPi * freq / sampleRate);
        invH = 1.0f / (1.0f + g);
        G = g * invH;
        G2 = G * G;
        G3 = G2 * G;
        gamma = G3 * G;

        for (auto& s : stages)
            s.G = G;
    }

    /** Resonance 0..10. */
    void setQ (float q) noexcept { k = 0.4f * std::clamp (q, 0.0f, 10.0f); }

    void setMode (int newMode) noexcept
    {
        if (mode == newMode)
            return;

        mode = newMode;
        mix = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (mode == lowPass2)
            mix[1] = 1.0f;
        else
            mix[3] = 1.0f;
    }

    float process (float in) noexcept
    {
        float sigma = G3 * stages[0].z + G2 * stages[1].z + G * stages[2].z + stages[3].z;
        sigma *= invH;

        float u = in * 0.5f - k * driveSignal (sigma) * kInvTanhOne;
        u /= 1.0f + k * gamma;

        const float lp1 = stages[0].process (u);
        const float lp2 = stages[1].process (lp1);
        const float lp3 = stages[2].process (lp2);
        const float lp4 = stages[3].process (lp3);
        return lp1 * mix[0] + lp2 * mix[1] + lp3 * mix[2] + lp4 * mix[3];
    }

    void reset() noexcept
    {
        for (auto& s : stages)
            s.z = 0.0f;
    }

private:
    static constexpr float kInvTanhOne = 1.0f / 0.9375f; // 1 / driveSignal (1)

    float sampleRate = 44100.0f;
    float pitch = 10.0f;
    float k = 0.0f;
    float G = 0.0f, G2 = 0.0f, G3 = 0.0f, gamma = 0.0f, invH = 1.0f;
    int mode = -1;
    std::array<float, 4> mix {};
    std::array<TptOnePoleStage, 4> stages {};
};
} // namespace tangier::dsp
