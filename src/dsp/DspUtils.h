// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/simd/SIMDUtilities.hpp, src/dsp/shaping/NonLinear.hpp)
// Copyright (C) 2018-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: SSE code rewritten as scalar C++.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace tangier::dsp
{
inline constexpr float kPi = std::numbers::pi_v<float>;
inline constexpr float kTwoPi = 2.0f * kPi;

/** Polynomial band-limited step residual. t is the phase (0..1), dt the phase increment. */
inline float polyBlep (float t, float dt) noexcept
{
    if (t > 1.0f - dt)
    {
        const float x = (t - 1.0f) / dt;
        return x + x + x * x + 1.0f;
    }

    if (t < dt)
    {
        const float x = t / dt;
        return x + x - x * x - 1.0f;
    }

    return 0.0f;
}

/** Piecewise polynomial approximation of tanh() used by the OTA filter stages. */
inline float driveSignal (float x) noexcept
{
    if (x > 1.25f)
        return 1.0f;
    if (x > 0.75f)
        return x * (2.5f - x) - 0.5625f;
    if (x < -1.25f)
        return -1.0f;
    if (x < -0.75f)
        return x * (2.5f + x) + 0.5625f;
    return x;
}
} // namespace tangier::dsp
