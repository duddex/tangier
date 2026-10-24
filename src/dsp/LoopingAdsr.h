// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/dsp/modulation/VecLoopingADSR.*)
// Copyright (C) 2020 Dale Johnson, Valley Audio
// Based on the ADSR module from VCV Fundamental, Copyright (C) 2016-2023 VCV
// Modifications Copyright (C) 2026 duddex: rewritten as scalar C++; in cycle mode the
// envelope only keeps looping while the gate is held, so plugin voices can be released.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>

namespace tangier::dsp
{
/** Exponential ADSR envelope with an optional looping (cycle) mode and a time scale. */
class LoopingAdsr
{
public:
    bool looping = false;

    void setSampleRate (float sampleRate) noexcept { sampleTime = 1.0f / sampleRate; }

    /** 1 = short times (1 ms to 10 s), 10 = long times (10 ms to 100 s). */
    void setTimeScale (float timeScale) noexcept
    {
        timeScale = std::clamp (timeScale, 0.0f, 10.0f);
        minTimeScaled = kMinTime * timeScale;
        maxTimeScaled = kMaxTime * timeScale;
        lambdaBase = maxTimeScaled / minTimeScaled;
    }

    /** All values 0..1. Call setTimeScale() first. */
    void setAdsr (float attack, float decay, float newSustain, float release) noexcept
    {
        sustain = std::clamp (newSustain, 0.0f, 1.0f);
        attackLambda = std::pow (lambdaBase, -std::clamp (attack, 0.0f, 1.0f)) / minTimeScaled;
        decayLambda = std::pow (lambdaBase, -std::clamp (decay, 0.0f, 1.0f)) / minTimeScaled;
        releaseLambda = std::pow (lambdaBase, -std::clamp (release, 0.0f, 1.0f)) / minTimeScaled;
    }

    /** gate and trigger are "voltages": a value >= 1 counts as high. */
    float process (float gate, float trigger) noexcept
    {
        gated = gate >= 1.0f;

        if (looping && sustaining)
            attacking = true;

        // Retrigger
        const bool prevTriggered = triggered;
        triggered = trigger >= 1.0f;
        if (! prevTriggered && triggered)
            latched = true;
        if (gated)
            latched = false;
        if (triggered)
            attacking = true;

        constexpr float attackTarget = 1.2f;
        const float target = gated ? (attacking ? attackTarget : sustain)
                                   : (latched ? attackTarget : 0.0f);
        const float lambda = (gated || latched) ? (attacking ? attackLambda : decayLambda)
                                                : releaseLambda;

        env += (target - env) * lambda * sampleTime;

        if (env >= 1.0f)
        {
            attacking = false;
            latched = false;
        }

        if (! gated)
            attacking = true;

        sustaining = (sustain <= env) && (env < sustain + kEpsilon);
        return env;
    }

    float getValue() const noexcept { return env; }

    void reset() noexcept
    {
        env = 0.0f;
        sustaining = gated = triggered = latched = false;
        attacking = true;
    }

private:
    static constexpr float kMinTime = 1e-3f;
    static constexpr float kMaxTime = 10.0f;
    static constexpr float kEpsilon = 0.01f;

    float sampleTime = 1.0f / 44100.0f;
    float minTimeScaled = kMinTime;
    float maxTimeScaled = kMaxTime;
    float lambdaBase = kMaxTime / kMinTime;

    float sustain = 1.0f;
    float attackLambda = 0.0f, decayLambda = 0.0f, releaseLambda = 0.0f;
    float env = 0.0f;
    bool attacking = true, sustaining = false, gated = false, triggered = false, latched = false;
};
} // namespace tangier::dsp
