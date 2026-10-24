// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
//
// Voice signal flow ported from ValleyRackFree (src/Interzone/Interzone.cpp)
// Copyright (C) 2019-2020 Dale Johnson, Valley Audio
// Modifications Copyright (C) 2026 duddex: VCV Rack CV/gate inputs replaced by MIDI
// voice allocation; SSE voice groups replaced by scalar per-voice processing.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SynthEngine.h"

#include <algorithm>
#include <cmath>

namespace tangier
{
namespace
{
constexpr float kFreqC4 = 261.6256f;
constexpr float kGateVolts = 10.0f;
constexpr float kTriggerSeconds = 0.001f;

/** MIDI note to V/oct, with C4 (note 60) at 0 V as in VCV Rack. */
float noteToVolts (int note) { return static_cast<float> (note - 60) / 12.0f; }
} // namespace

SynthEngine::SynthEngine()
{
    prepare (44100.0);
    setControls (controls);
}

void SynthEngine::prepare (double newSampleRate)
{
    sampleRate = static_cast<float> (newSampleRate);

    lfo.setSampleRate (sampleRate);
    pink.setSampleRate (sampleRate);
    lfoSlew.setSampleRate (sampleRate);
    lfoSlew.setCutoffFreq (14000.0f);
    lfoSlew.reset();

    for (auto& v : voices)
    {
        v.osc.setSampleRate (sampleRate);
        v.osc.reset();
        v.filter.setSampleRate (sampleRate);
        v.filter.reset();
        v.highPass.setSampleRate (sampleRate);
        v.highPass.reset();
        v.env.setSampleRate (sampleRate);
        v.env.reset();
        v.gateSlew.setSampleRate (sampleRate);
        v.gateSlew.setCutoffFreq (90.0f);
        v.gateSlew.reset();
        v.glide.setSampleRate (sampleRate);
        v.glide.reset();
        v.gate = v.keyDown = v.sustained = v.active = false;
        v.note = -1;
        v.triggerSamples = 0;
    }

    monoNoteStack.clear();
    setControls (controls);
}

void SynthEngine::setControls (const SynthControls& c)
{
    controls = c;

    // Pitch: octave (-2..2) + coarse (semitones, optionally quantised) + fine (cents)
    const float coarseSemis = c.coarseMode == 1 ? std::round (c.coarse) : c.coarse;
    d.pitchOffset = static_cast<float> (c.octave - 2) + coarseSemis / 12.0f + c.fine / 1200.0f;

    d.pulseWidth = c.pulseWidth;
    d.pwmDepth = c.pwmDepth;
    d.pwmEnvPolarity = static_cast<float> (c.pwmEnvPolarity) * -2.0f + 1.0f;
    d.pwmSource = c.pwmSource;

    d.subWidth = c.subWave < 1 ? 0.75f : 0.5f;
    d.subIsSaw = c.subWave > 1;

    d.sawLevel = c.sawLevel;
    d.pulseLevel = c.pulseLevel;
    d.subLevel = c.subLevel;
    d.noiseLevel = c.noiseLevel;
    d.extLevel = c.extLevel;

    d.cutoff = c.cutoff;
    d.q = c.resonance;
    d.keyTrack = c.keyTrack;
    d.filterLfoDepth = c.filterLfo * c.filterLfo * 5.0f;
    d.filterEnvAmount = c.filterEnv * 10.0f;
    d.filterEnvPolarity = static_cast<float> (c.filterEnvPolarity) * 2.0f - 1.0f;
    d.velocityToCutoff = c.velocityToCutoff;
    d.modWheelToCutoff = c.modWheelToCutoff;

    d.pitchModFromEnv = c.pitchModSource == 1;
    d.pitchModDepth = c.pitchMod * c.pitchMod;
    d.pitchModEnvPolarity = static_cast<float> (c.pitchModEnvPolarity) * 2.0f - 1.0f;

    d.vcaUsesGate = c.vcaSource == 1;
    d.aftertouchToVca = c.aftertouchToVca * 0.1f;
    d.velocitySensitivity = c.velocitySensitivity;
    d.outputGain = 0.1f * std::pow (10.0f, c.volumeDb / 20.0f);

    // Global modulators
    lfo.setFrequency (0.1f * std::exp2 (c.lfoRate + c.lfoFine));
    lfoSlewParam = c.lfoSlew;
    lfoSlew.setCutoffFreq (1760.0f * std::exp2 (c.lfoSlew * 2.0f * -6.0f));
    lfoWave = std::clamp (c.lfoWave, 0, static_cast<int> (dsp::Lfo::numWaves) - 1);
    noiseIsPink = c.noiseType == 1;
    lfoKeySync = c.lfoKeySync == 1;

    bendRange = static_cast<float> (c.bendRange);
    bendVolts = bendNormalised * bendRange / 12.0f;

    // Per-voice settings
    const float glideCutoff = 330.0f * std::exp2 (c.glide * 2.0f * -7.0f);
    const float timeScale = c.envLength == 1 ? 10.0f : 1.0f;
    const int subHarmonic = std::clamp (c.subOctave, 0, static_cast<int> (dsp::DirectOscillator::numSubHarmonics) - 1);
    const float highPassHz = 440.0f * std::exp2 (c.highPass - 5.0f);
    const int filterMode = c.poles == 0 ? dsp::OtaFilter::lowPass2 : dsp::OtaFilter::lowPass4;

    for (auto& v : voices)
    {
        v.glide.setCutoffFreq (glideCutoff);
        v.env.looping = c.envCycle == 1;
        v.env.setTimeScale (timeScale);
        v.env.setAdsr (c.attack, c.decay, c.sustain, c.release);
        v.highPass.setCutoffFreq (highPassHz);
        v.filter.setMode (filterMode);
        v.osc.setSubHarmonic (subHarmonic);
        v.osc.setSubWidth (d.subWidth);
    }

    // Voice count changes: release voices that are no longer allowed
    const int newNumVoices = std::clamp (c.voices, 1, maxVoices);
    if (newNumVoices != numVoices)
    {
        for (int i = newNumVoices; i < maxVoices; ++i)
            if (voices[static_cast<size_t> (i)].gate)
                releaseVoice (voices[static_cast<size_t> (i)]);

        if (newNumVoices == 1 || numVoices == 1)
            monoNoteStack.clear();

        numVoices = newNumVoices;
    }
}

bool SynthEngine::anyKeyHeld() const
{
    return std::any_of (voices.begin(), voices.end(), [] (const Voice& v) { return v.keyDown; });
}

void SynthEngine::startVoice (Voice& v, int note, float velocity)
{
    v.note = note;
    v.notePitch = noteToVolts (note);
    v.velocity = velocity;
    v.polyPressure = 0.0f;
    v.gate = true;
    v.keyDown = true;
    v.sustained = false;
    v.triggerSamples = std::max (1, static_cast<int> (kTriggerSeconds * sampleRate));
    v.noteOnOrder = ++orderCounter;

    if (! v.active)
    {
        // A silent voice starts its glide from the new note, like a freshly patched VCV voice.
        v.glide.reset (v.notePitch + bendVolts);
        v.active = true;
    }
}

void SynthEngine::releaseVoice (Voice& v)
{
    v.gate = false;
    v.keyDown = false;
    v.sustained = false;
    v.releaseOrder = ++orderCounter;
}

int SynthEngine::findVoiceForNote (int note) const
{
    for (int i = 0; i < numVoices; ++i)
        if (voices[static_cast<size_t> (i)].gate && voices[static_cast<size_t> (i)].note == note)
            return i;
    return -1;
}

int SynthEngine::allocateVoice()
{
    int best = -1;

    // 1. A silent voice, 2. the voice that was released longest ago
    for (int i = 0; i < numVoices; ++i)
    {
        const auto& v = voices[static_cast<size_t> (i)];
        if (v.gate)
            continue;
        if (! v.active)
            return i;
        if (best < 0 || v.releaseOrder < voices[static_cast<size_t> (best)].releaseOrder)
            best = i;
    }

    if (best >= 0)
        return best;

    // 3. Steal the oldest playing note
    best = 0;
    for (int i = 1; i < numVoices; ++i)
        if (voices[static_cast<size_t> (i)].noteOnOrder < voices[static_cast<size_t> (best)].noteOnOrder)
            best = i;
    return best;
}

void SynthEngine::noteOn (int note, float velocity)
{
    if (lfoKeySync && ! anyKeyHeld())
        lfo.resetPhase();

    if (numVoices == 1)
    {
        monoNoteStack.erase (std::remove (monoNoteStack.begin(), monoNoteStack.end(), note), monoNoteStack.end());
        monoNoteStack.push_back (note);
        startVoice (voices[0], note, velocity);
        return;
    }

    const int existing = findVoiceForNote (note);
    startVoice (voices[static_cast<size_t> (existing >= 0 ? existing : allocateVoice())], note, velocity);
}

void SynthEngine::noteOff (int note)
{
    if (numVoices == 1)
    {
        monoNoteStack.erase (std::remove (monoNoteStack.begin(), monoNoteStack.end(), note), monoNoteStack.end());

        auto& v = voices[0];
        if (v.note != note || ! v.keyDown)
            return;

        if (! monoNoteStack.empty())
        {
            // Legato: return to the previous held note
            startVoice (v, monoNoteStack.back(), v.velocity);
            return;
        }

        if (sustainDown)
        {
            v.keyDown = false;
            v.sustained = true;
        }
        else
        {
            releaseVoice (v);
        }
        return;
    }

    for (auto& v : voices)
    {
        if (v.note != note || ! v.keyDown)
            continue;

        if (sustainDown)
        {
            v.keyDown = false;
            v.sustained = true;
        }
        else
        {
            releaseVoice (v);
        }
    }
}

void SynthEngine::pitchBend (float normalised)
{
    bendNormalised = std::clamp (normalised, -1.0f, 1.0f);
    bendVolts = bendNormalised * bendRange / 12.0f;
}

void SynthEngine::modWheel (float normalised) { modWheelValue = std::clamp (normalised, 0.0f, 1.0f); }

void SynthEngine::channelPressure (float normalised) { channelPressureValue = std::clamp (normalised, 0.0f, 1.0f); }

void SynthEngine::polyPressure (int note, float normalised)
{
    for (auto& v : voices)
        if (v.gate && v.note == note)
            v.polyPressure = std::clamp (normalised, 0.0f, 1.0f);
}

void SynthEngine::sustainPedal (bool down)
{
    sustainDown = down;

    if (! down)
        for (auto& v : voices)
            if (v.sustained)
                releaseVoice (v);
}

void SynthEngine::allNotesOff()
{
    for (auto& v : voices)
        if (v.gate)
            releaseVoice (v);

    monoNoteStack.clear();
    sustainDown = false;
}

int SynthEngine::getNumActiveVoices() const
{
    return static_cast<int> (std::count_if (voices.begin(), voices.end(), [] (const Voice& v) { return v.active; }));
}

float SynthEngine::processVoice (Voice& v, float noise, float ext)
{
    // Gate, trigger and envelope
    const float gateVolts = v.gate ? kGateVolts : 0.0f;
    const float triggerVolts = v.triggerSamples > 0 ? kGateVolts : 0.0f;
    if (v.triggerSamples > 0)
        --v.triggerSamples;

    const float gateSlewed = v.gateSlew.process (gateVolts);
    const float env = v.env.process (gateVolts, triggerVolts);

    // VCA level
    const float aftertouchVolts = std::max (channelPressureValue, v.polyPressure) * 10.0f;
    float level = d.vcaUsesGate ? gateSlewed : env;
    level = std::clamp (level + aftertouchVolts * d.aftertouchToVca, 0.0f, 1.0f);

    // Pitch with glide
    const float pitch = v.glide.process (v.notePitch + bendVolts);

    // Filter cutoff and resonance
    float cutoff = d.cutoff;
    cutoff += v.velocity * 10.0f * d.velocityToCutoff;
    cutoff += modWheelValue * 10.0f * d.modWheelToCutoff;
    cutoff += pitch * d.keyTrack;
    cutoff += lfoValue * d.filterLfoDepth;
    cutoff += env * d.filterEnvPolarity * d.filterEnvAmount;
    v.filter.setCutoff (cutoff);
    v.filter.setQ (d.q);

    // Oscillator pitch and pulse width
    const float pitchMod = (d.pitchModFromEnv ? env * d.pitchModEnvPolarity : lfoValue) * d.pitchModDepth;
    v.osc.setFrequency (kFreqC4 * std::exp2 (pitch + d.pitchOffset + pitchMod));

    float pwm;
    switch (d.pwmSource)
    {
        case 0:  pwm = modWheelValue * 10.0f * -0.1f; break;
        case 2:  pwm = env * d.pwmEnvPolarity; break;
        default: pwm = lfoValue * -0.5f - 0.5f; break;
    }
    v.osc.setPulseWidth (std::clamp (pwm * d.pwmDepth + d.pulseWidth, 0.0f, 0.5f));

    // Oscillator, mixer, filter, VCA
    v.osc.process();
    const float sub = d.subIsSaw ? v.osc.getSubSaw() : v.osc.getSubPulse();

    float mix = v.osc.getSaw() * d.sawLevel;
    mix += v.osc.getPulse() * d.pulseLevel;
    mix += sub * d.subLevel;
    mix += noise * d.noiseLevel;
    mix += ext * 5.0f * d.extLevel;

    const float filtered = v.filter.process (mix * 2.0f + noise * 8e-5f);
    const float highPassed = v.highPass.process (filtered * 5.0f);
    const float out = std::clamp (highPassed * level, -10.0f, 10.0f);

    if (! v.gate && level <= 1e-6f)
        v.active = false;

    const float velocityGain = 1.0f - d.velocitySensitivity + d.velocitySensitivity * v.velocity;
    return out * velocityGain;
}

void SynthEngine::render (float* out, const float* ext, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        // Shared modulators
        lfo.process();
        const float lfoRaw = lfo.getOutput (lfoWave);
        const float slewed = lfoSlew.process (lfoRaw);
        lfoValue = lfoSlewParam > 0.0001f ? slewed : lfoRaw;

        pink.process();
        const float noise = noiseIsPink ? pink.getValue() : lfo.getOutput (dsp::Lfo::noise);
        const float extSample = ext != nullptr ? ext[i] : 0.0f;

        float sum = 0.0f;
        for (auto& v : voices)
            if (v.active)
                sum += processVoice (v, noise, extSample);

        out[i] = sum * d.outputGain;
    }
}
} // namespace tangier
