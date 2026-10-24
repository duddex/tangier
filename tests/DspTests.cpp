// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Plain sanity tests for the DSP building blocks. Run via CTest or directly.

#include "dsp/DirectOscillator.h"
#include "dsp/Lfo.h"
#include "dsp/LoopingAdsr.h"
#include "dsp/Noise.h"
#include "dsp/OtaFilter.h"
#include "engine/SynthEngine.h"

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace
{
using namespace tangier::dsp;

constexpr float kSampleRate = 48000.0f;
int failures = 0;

void check (bool condition, const std::string& message)
{
    if (! condition)
    {
        std::printf ("  FAIL: %s\n", message.c_str());
        ++failures;
    }
}

bool isFiniteAndBounded (float x, float bound) { return std::isfinite (x) && std::abs (x) <= bound; }

/** Counts rising zero crossings of the saw output over one second. */
void testOscillatorFrequency()
{
    DirectOscillator osc;
    osc.setSampleRate (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setPulseWidth (0.5f);

    int crossings = 0;
    float prev = 0.0f;
    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
    {
        osc.process();
        const float s = osc.getPulse();
        if (prev < 0.0f && s >= 0.0f)
            ++crossings;
        prev = s;
        check (isFiniteAndBounded (osc.getSaw(), 2.0f), "saw out of range");
        check (isFiniteAndBounded (s, 2.5f), "pulse out of range");
    }

    check (std::abs (crossings - 440) <= 2, "pulse frequency should be 440 Hz, got " + std::to_string (crossings));
}

void testSubOctave()
{
    DirectOscillator osc;
    osc.setSampleRate (kSampleRate);
    osc.setSubHarmonic (DirectOscillator::sub1Octave);
    osc.setSubWidth (0.5f);
    osc.setFrequency (440.0f);

    int crossings = 0;
    float prev = 0.0f;
    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
    {
        osc.process();
        const float s = osc.getSubPulse();
        if (prev < 0.0f && s >= 0.0f)
            ++crossings;
        prev = s;
        check (isFiniteAndBounded (s, 2.5f), "sub pulse out of range");
    }

    check (std::abs (crossings - 220) <= 2, "sub octave should be 220 Hz, got " + std::to_string (crossings));
}

void testFilterStability()
{
    for (int mode : { OtaFilter::lowPass2, OtaFilter::lowPass4 })
    {
        OtaFilter filter;
        filter.setSampleRate (kSampleRate);
        filter.setMode (mode);
        filter.setQ (10.0f);

        WhiteNoise noise;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            filter.setCutoff (10.0f * static_cast<float> (i) / kSampleRate);
            const float y = filter.process (noise.process() * 2.0f);
            check (isFiniteAndBounded (y, 4.0f), "filter output unstable");
        }
    }
}

void testFilterAttenuatesHighFrequencies()
{
    OtaFilter filter;
    filter.setSampleRate (kSampleRate);
    filter.setMode (OtaFilter::lowPass4);
    filter.setQ (0.0f);
    filter.setCutoff (2.0f); // 55 Hz

    float peak = 0.0f;
    for (int i = 0; i < 4800; ++i)
    {
        const float x = std::sin (kTwoPi * 5000.0f * static_cast<float> (i) / kSampleRate);
        const float y = filter.process (x);
        if (i > 2400)
            peak = std::max (peak, std::abs (y));
    }

    check (peak < 0.001f, "4-pole filter at 55 Hz should block 5 kHz, peak " + std::to_string (peak));
}

void testAdsr()
{
    LoopingAdsr env;
    env.setSampleRate (kSampleRate);
    env.setTimeScale (1.0f);
    env.setAdsr (0.2f, 0.3f, 0.5f, 0.3f);

    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        env.process (10.0f, 0.0f);
    check (std::abs (env.getValue() - 0.5f) < 0.02f, "envelope should settle at sustain 0.5, got " + std::to_string (env.getValue()));

    for (int i = 0; i < static_cast<int> (kSampleRate) * 2; ++i)
        env.process (0.0f, 0.0f);
    check (env.getValue() < 0.001f, "envelope should release to 0, got " + std::to_string (env.getValue()));
}

void testLoopingAdsrReleases()
{
    LoopingAdsr env;
    env.looping = true;
    env.setSampleRate (kSampleRate);
    env.setTimeScale (1.0f);
    env.setAdsr (0.1f, 0.1f, 0.3f, 0.1f);

    float minAfterPeak = 1.0f;
    float maxLate = 0.0f;
    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
    {
        const float v = env.process (10.0f, 0.0f);
        if (i > 24000)
        {
            minAfterPeak = std::min (minAfterPeak, v);
            maxLate = std::max (maxLate, v);
        }
    }
    check (maxLate > 0.9f && minAfterPeak < 0.4f, "cycling envelope should keep looping while gated");

    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        env.process (0.0f, 0.0f);
    check (env.getValue() < 0.001f, "cycling envelope should release after gate off");
}

void testLfo()
{
    Lfo lfo;
    lfo.setSampleRate (kSampleRate);
    lfo.setFrequency (10.0f);

    int crossings = 0;
    float prev = 0.0f;
    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
    {
        lfo.process();
        for (int w = 0; w < Lfo::numWaves; ++w)
            check (isFiniteAndBounded (lfo.getOutput (w), 1.0001f), "LFO output out of range");

        const float s = lfo.getOutput (Lfo::sine);
        if (prev < 0.0f && s >= 0.0f)
            ++crossings;
        prev = s;
    }
    check (std::abs (crossings - 10) <= 1, "LFO should run at 10 Hz, got " + std::to_string (crossings));
}

void testPinkNoise()
{
    PinkNoise pink;
    pink.setSampleRate (kSampleRate);
    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        check (isFiniteAndBounded (pink.process(), 4.0f), "pink noise out of range");
}

float renderPeak (tangier::SynthEngine& engine, int numSamples)
{
    std::vector<float> buffer (static_cast<size_t> (numSamples));
    engine.render (buffer.data(), nullptr, numSamples);

    float peak = 0.0f;
    for (float s : buffer)
    {
        check (std::isfinite (s), "engine output not finite");
        peak = std::max (peak, std::abs (s));
    }
    return peak;
}

void testEngineNoteOnOff()
{
    tangier::SynthEngine engine;
    engine.prepare (kSampleRate);
    tangier::SynthControls c;
    c.release = 0.2f;
    engine.setControls (c);

    check (renderPeak (engine, 480) == 0.0f, "engine should be silent without notes");

    engine.noteOn (60, 1.0f);
    engine.noteOn (64, 1.0f);
    engine.noteOn (67, 1.0f);
    const float peak = renderPeak (engine, 4800);
    check (peak > 0.05f && peak < 1.5f, "chord should be audible and not clip, peak " + std::to_string (peak));
    check (engine.getNumActiveVoices() == 3, "three voices should be active");

    engine.noteOff (60);
    engine.noteOff (64);
    engine.noteOff (67);
    renderPeak (engine, static_cast<int> (kSampleRate));
    check (engine.getNumActiveVoices() == 0, "voices should be freed after release");
    check (renderPeak (engine, 480) == 0.0f, "engine should be silent after release");
}

void testEngineVoiceStealing()
{
    tangier::SynthEngine engine;
    engine.prepare (kSampleRate);
    tangier::SynthControls c;
    c.voices = 4;
    engine.setControls (c);

    for (int n = 0; n < 10; ++n)
        engine.noteOn (48 + n, 0.8f);
    renderPeak (engine, 480);
    check (engine.getNumActiveVoices() == 4, "voice count should be limited to 4, got "
                                                 + std::to_string (engine.getNumActiveVoices()));
    engine.allNotesOff();
}

void testEngineMonoLegato()
{
    tangier::SynthEngine engine;
    engine.prepare (kSampleRate);
    tangier::SynthControls c;
    c.voices = 1;
    engine.setControls (c);

    engine.noteOn (60, 1.0f);
    engine.noteOn (67, 1.0f);
    renderPeak (engine, 480);
    engine.noteOff (67); // falls back to the held note 60
    check (renderPeak (engine, 4800) > 0.05f, "mono voice should keep sounding the held note");
    check (engine.getNumActiveVoices() == 1, "mono mode should use a single voice");
}

void testEngineSustainPedal()
{
    tangier::SynthEngine engine;
    engine.prepare (kSampleRate);
    engine.setControls ({});

    engine.sustainPedal (true);
    engine.noteOn (60, 1.0f);
    engine.noteOff (60);
    check (renderPeak (engine, 4800) > 0.05f, "sustain pedal should hold the note");
    engine.sustainPedal (false);
    renderPeak (engine, static_cast<int> (kSampleRate));
    check (engine.getNumActiveVoices() == 0, "releasing the pedal should release the note");
}
} // namespace

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests {
        { "oscillator frequency", testOscillatorFrequency },
        { "sub octave", testSubOctave },
        { "filter stability", testFilterStability },
        { "filter attenuation", testFilterAttenuatesHighFrequencies },
        { "ADSR", testAdsr },
        { "looping ADSR", testLoopingAdsrReleases },
        { "LFO", testLfo },
        { "pink noise", testPinkNoise },
        { "engine note on/off", testEngineNoteOnOff },
        { "engine voice stealing", testEngineVoiceStealing },
        { "engine mono legato", testEngineMonoLegato },
        { "engine sustain pedal", testEngineSustainPedal },
    };

    for (const auto& [name, test] : tests)
    {
        const int before = failures;
        test();
        std::printf ("%s %s\n", failures == before ? "[ OK ]" : "[FAIL]", name);
    }

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
