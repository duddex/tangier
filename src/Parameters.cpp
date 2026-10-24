// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// Parameter set and ranges ported from ValleyRackFree Interzone, Copyright (C) 2019-2020 Dale Johnson
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Parameters.h"

namespace tangier::params
{
namespace
{
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using StringFromValue = std::function<juce::String (float, int)>;

juce::String percent (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String bipolar (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 2) + " x"; }

juce::String hertz (float hz)
{
    return hz >= 1000.0f ? juce::String (hz / 1000.0f, 2) + " kHz"
                         : juce::String (hz, hz < 10.0f ? 2 : 1) + " Hz";
}

void addFloat (Layout& layout, const char* id, const juce::String& name, float min, float max, float def,
               StringFromValue toString = percent, const juce::String& label = {})
{
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (min, max), def,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toString)).withLabel (label)));
}

void addChoice (Layout& layout, const char* id, const juce::String& name, const juce::StringArray& choices, int def)
{
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, choices, def));
}

const juce::StringArray polarity { "Negative", "Positive" };
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    Layout layout;

    // VCO
    addFloat (layout, glide, "Glide", 0.0f, 1.0f, 0.0f);
    addFloat (layout, pitchMod, "Pitch Mod Depth", 0.0f, 1.0f, 0.0f,
              [] (float v, int) { return juce::String (v * v, 3) + " oct"; });
    addChoice (layout, pitchModSource, "Pitch Mod Source", { "LFO", "Envelope" }, 0);
    addChoice (layout, pitchModEnvPolarity, "Pitch Mod Env Polarity", polarity, 1);
    addChoice (layout, octave, "Octave", { "32'", "16'", "8'", "4'", "2'" }, 2);
    addFloat (layout, coarse, "Coarse Tune", -12.0f, 12.0f, 0.0f,
              [] (float v, int) { return juce::String (v, 2) + " st"; });
    addChoice (layout, coarseMode, "Coarse Tune Quantisation", { "Continuous", "Semitone" }, 0);
    addFloat (layout, fine, "Fine Tune", -100.0f, 100.0f, 0.0f,
              [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; });
    addFloat (layout, pulseWidth, "Pulse Width", 0.0f, 0.5f, 0.5f,
              [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " % duty"; });
    addFloat (layout, pwmDepth, "PWM Depth", 0.0f, 0.5f, 0.0f,
              [] (float v, int) { return juce::String (juce::roundToInt (v * 200.0f)) + " %"; });
    addChoice (layout, pwmSource, "PWM Source", { "Mod Wheel", "LFO", "Envelope" }, 1);
    addChoice (layout, pwmEnvPolarity, "PWM Env Polarity", polarity, 0);
    addChoice (layout, subOctave, "Sub Wave Octave",
               { "-2 Oct", "-1 Oct", "-5th", "Unison", "+5th", "+1 Oct", "+2 Oct" }, 3);

    // Mixer
    addFloat (layout, sawLevel, "Saw Level", 0.0f, 1.0f, 0.8f);
    addFloat (layout, pulseLevel, "Pulse Level", 0.0f, 1.0f, 0.0f);
    addFloat (layout, subLevel, "Sub Level", 0.0f, 1.0f, 0.0f);
    addChoice (layout, subWave, "Sub Wave Type", { "Pulse", "Square", "Saw" }, 1);
    addFloat (layout, noiseLevel, "Noise Level", 0.0f, 1.0f, 0.0f);
    addChoice (layout, noiseType, "Noise Type", { "White", "Pink" }, 0);
    addFloat (layout, extLevel, "External Input Level", 0.0f, 1.0f, 0.0f);

    // Filter
    addFloat (layout, cutoff, "Filter Cutoff", 0.0f, 10.0f, 10.0f,
              [] (float v, int) { return hertz (cutoffToHz (v)); });
    addFloat (layout, resonance, "Filter Resonance", 0.0f, 10.0f, 0.0f,
              [] (float v, int) { return juce::String (juce::roundToInt (v * 10.0f)) + " %"; });
    addFloat (layout, highPass, "High Pass Cutoff", 0.0f, 10.0f, 0.0f,
              [] (float v, int) { return hertz (cutoffToHz (v)); });
    addChoice (layout, poles, "Filter Poles", { "2 Pole", "4 Pole" }, 1);
    addFloat (layout, filterEnv, "Filter Env Depth", 0.0f, 1.0f, 0.0f);
    addChoice (layout, filterEnvPolarity, "Filter Env Polarity", polarity, 1);
    addFloat (layout, filterLfo, "Filter LFO Depth", 0.0f, 1.0f, 0.0f);
    addFloat (layout, keyTrack, "Filter Key Tracking", 0.0f, 1.0f, 0.0f);
    addFloat (layout, velocityToCutoff, "Velocity to Cutoff", -1.0f, 1.0f, 0.0f, bipolar);
    addFloat (layout, modWheelToCutoff, "Mod Wheel to Cutoff", -1.0f, 1.0f, 0.0f, bipolar);

    // LFO
    addFloat (layout, lfoRate, "LFO Rate", 0.0f, 11.0f, 0.0f,
              [] (float v, int) { return hertz (lfoRateToHz (v)); });
    addFloat (layout, lfoFine, "LFO Fine", -0.5f, 0.5f, 0.0f,
              [] (float v, int) { return juce::String (v, 2) + " oct"; });
    addFloat (layout, lfoSlew, "LFO Slew", 0.0f, 1.0f, 0.0f);
    addChoice (layout, lfoWave, "LFO Wave",
               { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Sample + Hold", "Noise" }, 0);
    addChoice (layout, lfoKeySync, "LFO Key Sync", { "Free", "Key Sync" }, 0);

    // Envelope
    addFloat (layout, attack, "Attack", 0.0f, 1.0f, 0.0f);
    addFloat (layout, decay, "Decay", 0.0f, 1.0f, 0.0f);
    addFloat (layout, sustain, "Sustain", 0.0f, 1.0f, 1.0f);
    addFloat (layout, release, "Release", 0.0f, 1.0f, 0.0f);
    addChoice (layout, envLength, "Envelope Time Scale", { "Short", "Long" }, 0);
    addChoice (layout, envCycle, "Envelope Mode", { "Gate", "Cycle" }, 0);

    // VCA
    addChoice (layout, vcaSource, "VCA Source", { "Envelope", "Gate" }, 0);
    addFloat (layout, aftertouchToVca, "Aftertouch to VCA", -1.0f, 1.0f, 0.0f, bipolar);

    // Global
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { voices, 1 }, "Voices", 1, 16, 8));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { bendRange, 1 }, "Pitch Bend Range", 0, 24, 2,
        juce::AudioParameterIntAttributes().withLabel ("st")));
    addFloat (layout, velocitySensitivity, "Velocity Sensitivity", 0.0f, 1.0f, 0.5f);
    addFloat (layout, volume, "Volume", -48.0f, 6.0f, -6.0f,
              [] (float v, int) { return juce::String (v, 1) + " dB"; });

    return layout;
}
} // namespace tangier::params
