// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Voice signal flow ported from ValleyRackFree (src/Interzone/Interzone.cpp)
// Copyright (C) 2019-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: VCV Rack CV/gate inputs replaced by MIDI
// voice allocation; SSE voice groups replaced by scalar per-voice processing.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsp/DirectOscillator.h"
#include "dsp/Lfo.h"
#include "dsp/LoopingAdsr.h"
#include "dsp/Noise.h"
#include "dsp/OnePoleFilters.h"
#include "dsp/OtaFilter.h"

#include <array>
#include <cstdint>
#include <vector>

namespace tangier
{
/** Raw parameter values, in the same units and ranges as the plugin parameters. */
struct SynthControls
{
    // VCO
    float glide = 0.0f, pitchMod = 0.0f;
    int pitchModSource = 0, pitchModEnvPolarity = 1;
    int octave = 2;
    float coarse = 0.0f;
    int coarseMode = 0;
    float fine = 0.0f;
    float pulseWidth = 0.5f, pwmDepth = 0.0f;
    int pwmSource = 1, pwmEnvPolarity = 0, subOctave = 3;

    // Mixer
    float sawLevel = 0.8f, pulseLevel = 0.0f, subLevel = 0.0f, noiseLevel = 0.0f, extLevel = 0.0f;
    int subWave = 1, noiseType = 0;

    // Filter
    float cutoff = 10.0f, resonance = 0.0f, highPass = 0.0f;
    int poles = 1;
    float filterEnv = 0.0f;
    int filterEnvPolarity = 1;
    float filterLfo = 0.0f, keyTrack = 0.0f, velocityToCutoff = 0.0f, modWheelToCutoff = 0.0f;

    // LFO
    float lfoRate = 0.0f, lfoFine = 0.0f, lfoSlew = 0.0f;
    int lfoWave = 0, lfoKeySync = 0;

    // Envelope
    float attack = 0.0f, decay = 0.0f, sustain = 1.0f, release = 0.0f;
    int envLength = 0, envCycle = 0;

    // VCA
    int vcaSource = 0;
    float aftertouchToVca = 0.0f;

    // Global
    int voices = 8, bendRange = 2;
    float velocitySensitivity = 0.5f, volumeDb = -6.0f;
};

class SynthEngine
{
public:
    static constexpr int maxVoices = 16;

    SynthEngine();

    void prepare (double sampleRate);
    void setControls (const SynthControls& controls);

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void pitchBend (float normalised);       // -1..1
    void modWheel (float normalised);        // 0..1
    void channelPressure (float normalised); // 0..1
    void polyPressure (int note, float normalised);
    void sustainPedal (bool down);
    void allNotesOff();

    /** Renders mono output. ext may be nullptr (no external input). */
    void render (float* out, const float* ext, int numSamples);

    int getNumActiveVoices() const;
    float getLfoValue() const { return lfoValue; }

private:
    /** Values derived from SynthControls, in the units used by the Interzone DSP code. */
    struct Derived
    {
        float pitchOffset = 0.0f;
        float pulseWidth = 0.5f, pwmDepth = 0.0f, pwmEnvPolarity = 1.0f;
        int pwmSource = 1;
        float subWidth = 0.5f;
        bool subIsSaw = false;
        float sawLevel = 0.8f, pulseLevel = 0.0f, subLevel = 0.0f, noiseLevel = 0.0f, extLevel = 0.0f;
        float cutoff = 10.0f, q = 0.0f, keyTrack = 0.0f;
        float filterLfoDepth = 0.0f, filterEnvAmount = 0.0f, filterEnvPolarity = 1.0f;
        float velocityToCutoff = 0.0f, modWheelToCutoff = 0.0f;
        bool pitchModFromEnv = false;
        float pitchModDepth = 0.0f, pitchModEnvPolarity = 1.0f;
        bool vcaUsesGate = false;
        float aftertouchToVca = 0.0f;
        float velocitySensitivity = 0.5f;
        float outputGain = 0.05f;
    };

    struct Voice
    {
        int note = -1;
        float notePitch = 0.0f;
        float velocity = 0.0f;
        float polyPressure = 0.0f;
        bool gate = false;
        bool keyDown = false;
        bool sustained = false;
        bool active = false;
        int triggerSamples = 0;
        uint64_t noteOnOrder = 0;
        uint64_t releaseOrder = 0;

        dsp::DirectOscillator osc;
        dsp::OtaFilter filter;
        dsp::OnePoleHighPass highPass;
        dsp::LoopingAdsr env;
        dsp::OnePoleLowPass glide;
        dsp::OnePoleLowPass gateSlew;
    };

    float processVoice (Voice& v, float noise, float ext);
    int findVoiceForNote (int note) const;
    int allocateVoice();
    void startVoice (Voice& v, int note, float velocity);
    void releaseVoice (Voice& v);
    bool anyKeyHeld() const;

    float sampleRate = 44100.0f;
    SynthControls controls;
    Derived d;

    std::array<Voice, maxVoices> voices;
    int numVoices = 8;
    uint64_t orderCounter = 0;
    std::vector<int> monoNoteStack;

    dsp::Lfo lfo;
    dsp::PinkNoise pink;
    dsp::OnePoleLowPass lfoSlew;
    float lfoSlewParam = 0.0f;
    int lfoWave = 0;
    bool noiseIsPink = false;
    bool lfoKeySync = false;
    float lfoValue = 0.0f;

    float bendVolts = 0.0f;
    float bendRange = 2.0f;
    float bendNormalised = 0.0f;
    float modWheelValue = 0.0f;
    float channelPressureValue = 0.0f;
    bool sustainDown = false;
};
} // namespace tangier
