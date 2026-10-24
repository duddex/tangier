// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Ported from ValleyRackFree (src/dsp/generators/Noise.hpp)
// Copyright (C) 2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: minor clean-up for use outside VCV Rack.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <random>

namespace tangier::dsp
{
class WhiteNoise
{
public:
    WhiteNoise() : rng (std::random_device{}()), uniform (-1.0f, 1.0f) {}

    float process() noexcept
    {
        value = uniform (rng);
        return value;
    }

    float getValue() const noexcept { return value; }

private:
    std::minstd_rand rng;
    std::uniform_real_distribution<float> uniform;
    float value = 0.0f;
};

/** Pink noise from filtered white noise (Paul Kellet's method with sample-rate-dependent poles). */
class PinkNoise
{
public:
    PinkNoise() { setSampleRate (44100.0); }

    float process() noexcept
    {
        const double white = whiteGen.process();
        b[0] += a[0] * ((white * 48.69991228070175) - b[0]);
        b[1] += a[1] * ((white * 11.23890718562874) - b[1]);
        b[2] += a[2] * ((white * 4.96296774193548) - b[2]);
        b[3] += a[3] * ((white * 2.32573483146067) - b[3]);
        b[4] += a[4] * ((white * 1.18433822222222) - b[4]);
        b[5] = -0.7616 * b[5] - white * 0.0168980;
        pink = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + white * 0.5362;
        b[6] = white * 0.115926;
        pink *= 0.15;
        return static_cast<float> (pink);
    }

    void setSampleRate (double sampleRate) noexcept
    {
        b.fill (0.0);
        pink = 0.0;

        constexpr std::array<double, 5> poles { 8.00135734209627, 46.88548507044182, 217.61558695916962,
                                                939.80665948455472, 3276.10128392439381 };
        for (size_t i = 0; i < poles.size(); ++i)
            a[i] = std::sin (2.0 * std::numbers::pi * poles[i] / sampleRate);
    }

    float getValue() const noexcept { return static_cast<float> (pink); }

private:
    WhiteNoise whiteGen;
    std::array<double, 7> b {};
    std::array<double, 5> a {};
    double pink = 0.0;
};
} // namespace tangier::dsp
