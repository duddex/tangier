// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/dsp/generators/VecDirectOsc.hpp)
// Copyright (C) 2018-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: SSE code rewritten as scalar C++.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "OnePoleFilters.h"

namespace tangier::dsp
{
/**
    Band-limited (PolyBLEP) oscillator producing saw, pulse and a sub wave at the same time.

    The sub wave is derived from segments of the main phasor that are rescaled and offset
    to build a new phasor at a different frequency.
*/
class DirectOscillator
{
public:
    enum SubHarmonic
    {
        sub2Octaves = 0,
        sub1Octave,
        subFifth,
        unison,
        fifth,
        octave,
        doubleOctave,
        numSubHarmonics
    };

    DirectOscillator()
    {
        for (auto* f : { &sawHpf, &pulseHpf, &subSawHpf, &subPulseHpf })
            f->setCutoffFreq (20.0f);

        setSampleRate (44100.0f);
        setSubHarmonic (unison);
    }

    void setSampleRate (float sampleRate) noexcept
    {
        invSampleRate = 1.0f / sampleRate;

        for (auto* f : { &sawHpf, &pulseHpf, &subSawHpf, &subPulseHpf })
        {
            f->setSampleRate (sampleRate);
            f->setCutoffFreq (20.0f);
        }

        setFrequency (frequency);
    }

    void setFrequency (float newFrequency) noexcept
    {
        frequency = newFrequency;
        stepSize = frequency * invSampleRate;
        subStepSize = std::abs (stepSize) * subScale;
    }

    /** Pulse width 0..0.5 (0.5 = square). */
    void setPulseWidth (float newPulseWidth) noexcept { pwm = newPulseWidth; }

    /** Pulse width of the sub pulse wave. */
    void setSubWidth (float newSubWidth) noexcept
    {
        subWidth = newSubWidth;
        subWidthOffset = (subWidth - 0.5f) * 2.0f;
    }

    void setSubHarmonic (int harmonic) noexcept
    {
        switch (harmonic)
        {
            case sub2Octaves:  subScale = 0.25f; subLimit = 4.0f; subOffsetDegree = 0.25f; break;
            case sub1Octave:   subScale = 0.5f;  subLimit = 2.0f; subOffsetDegree = 0.5f;  break;
            case subFifth:     subScale = 0.75f; subLimit = 4.0f; subOffsetDegree = 0.75f; break;
            case fifth:        subScale = 1.5f;  subLimit = 2.0f; subOffsetDegree = 0.5f;  break;
            case octave:       subScale = 2.0f;  subLimit = 1.0f; subOffsetDegree = 0.0f;  break;
            case doubleOctave: subScale = 4.0f;  subLimit = 1.0f; subOffsetDegree = 0.0f;  break;
            case unison:
            default:           subScale = 1.0f;  subLimit = 1.0f; subOffsetDegree = 0.0f;  break;
        }

        subStepSize = std::abs (stepSize) * subScale;
    }

    void reset() noexcept
    {
        a = phasor = prevPhasor = offset = 0.0f;
        saw = pulse = subSaw = subPulse = 0.0f;
        for (auto* f : { &sawHpf, &pulseHpf, &subSawHpf, &subPulseHpf })
            f->reset();
    }

    void process() noexcept
    {
        // Wrap the phasor
        if (a >= 1.0f) a -= 1.0f;
        if (a < 0.0f) a += 1.0f;
        phasor = a;

        const bool flyBack = (prevPhasor - phasor) >= 0.5f;
        const bool flyForward = (phasor - prevPhasor) > 0.5f;
        const float absStep = std::abs (stepSize);

        saw = phasor * 2.0f - 1.0f;
        saw -= polyBlep (phasor, absStep);

        // Pulse wave: a second, inverted and phase-shifted saw added to the first one
        float revPhasor = phasor + pwm;
        if (revPhasor >= 1.0f) revPhasor -= 1.0f;
        revPhasor = 1.0f - revPhasor;
        pulse = revPhasor * 2.0f - 1.0f;
        pulse = saw + (pulse - polyBlep (revPhasor, absStep));
        pulse += (pwm - 0.5f) * 2.0f;

        // Sub wave
        if (flyBack) offset += 1.0f;
        if (flyForward) offset -= 1.0f;
        if (offset == subLimit) offset -= subLimit;
        if (offset < 0.0f) offset += subLimit;

        float subPhasor = phasor * subScale + offset * subOffsetDegree;
        subPhasor -= static_cast<float> (static_cast<int> (subPhasor));
        subSaw = subPhasor * 2.0f - 1.0f;
        subSaw -= polyBlep (subPhasor, subStepSize);

        subPhasor += subWidth;
        subPhasor -= static_cast<float> (static_cast<int> (subPhasor));
        subPhasor = 1.0f - subPhasor;
        subPulse = subPhasor * 2.0f - 1.0f;
        subPulse -= polyBlep (subPhasor, subStepSize);
        subPulse += subSaw;
        subPulse += subWidthOffset;

        prevPhasor = phasor;
        a += stepSize;

        saw = sawHpf.process (saw);
        pulse = pulseHpf.process (pulse);
        subSaw = subSawHpf.process (subSaw);
        subPulse = subPulseHpf.process (subPulse);
    }

    float getSaw() const noexcept { return saw; }
    float getPulse() const noexcept { return pulse; }
    float getSubSaw() const noexcept { return subSaw; }
    float getSubPulse() const noexcept { return subPulse; }

private:
    float a = 0.0f, phasor = 0.0f, prevPhasor = 0.0f;
    float frequency = 440.0f, invSampleRate = 1.0f / 44100.0f;
    float stepSize = 0.0f, subStepSize = 0.0f;
    float pwm = 0.5f;
    float saw = 0.0f, pulse = 0.0f, subSaw = 0.0f, subPulse = 0.0f;
    float offset = 0.0f, subLimit = 1.0f, subScale = 1.0f, subOffsetDegree = 0.0f;
    float subWidth = 0.5f, subWidthOffset = 0.0f;
    OnePoleHighPass sawHpf, pulseHpf, subSawHpf, subPulseHpf;
};
} // namespace tangier::dsp
