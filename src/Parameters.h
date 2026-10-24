// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// Parameter set and ranges ported from ValleyRackFree Interzone, Copyright (C) 2019-2020 Dale Johnson
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace tangier::params
{
// VCO
inline constexpr auto glide = "glide";
inline constexpr auto pitchMod = "pitchMod";
inline constexpr auto pitchModSource = "pitchModSource";
inline constexpr auto pitchModEnvPolarity = "pitchModEnvPolarity";
inline constexpr auto octave = "octave";
inline constexpr auto coarse = "coarse";
inline constexpr auto coarseMode = "coarseMode";
inline constexpr auto fine = "fine";
inline constexpr auto pulseWidth = "pulseWidth";
inline constexpr auto pwmDepth = "pwmDepth";
inline constexpr auto pwmSource = "pwmSource";
inline constexpr auto pwmEnvPolarity = "pwmEnvPolarity";
inline constexpr auto subOctave = "subOctave";

// Mixer
inline constexpr auto sawLevel = "sawLevel";
inline constexpr auto pulseLevel = "pulseLevel";
inline constexpr auto subLevel = "subLevel";
inline constexpr auto subWave = "subWave";
inline constexpr auto noiseLevel = "noiseLevel";
inline constexpr auto noiseType = "noiseType";
inline constexpr auto extLevel = "extLevel";

// Filter
inline constexpr auto cutoff = "cutoff";
inline constexpr auto resonance = "resonance";
inline constexpr auto highPass = "highPass";
inline constexpr auto poles = "poles";
inline constexpr auto filterEnv = "filterEnv";
inline constexpr auto filterEnvPolarity = "filterEnvPolarity";
inline constexpr auto filterLfo = "filterLfo";
inline constexpr auto keyTrack = "keyTrack";
inline constexpr auto velocityToCutoff = "velocityToCutoff";
inline constexpr auto modWheelToCutoff = "modWheelToCutoff";

// LFO
inline constexpr auto lfoRate = "lfoRate";
inline constexpr auto lfoFine = "lfoFine";
inline constexpr auto lfoSlew = "lfoSlew";
inline constexpr auto lfoWave = "lfoWave";
inline constexpr auto lfoKeySync = "lfoKeySync";

// Envelope
inline constexpr auto attack = "attack";
inline constexpr auto decay = "decay";
inline constexpr auto sustain = "sustain";
inline constexpr auto release = "release";
inline constexpr auto envLength = "envLength";
inline constexpr auto envCycle = "envCycle";

// VCA
inline constexpr auto vcaSource = "vcaSource";
inline constexpr auto aftertouchToVca = "aftertouchToVca";

// Global (plugin-specific)
inline constexpr auto voices = "voices";
inline constexpr auto bendRange = "bendRange";
inline constexpr auto velocitySensitivity = "velocitySensitivity";
inline constexpr auto volume = "volume";

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/** Filter cutoff value (0..10, V/oct) to Hz. */
inline float cutoffToHz (float v) { return 440.0f * std::exp2 (v - 5.0f); }

/** LFO rate value (0..11, V/oct) to Hz. */
inline float lfoRateToHz (float v) { return 0.1f * std::exp2 (v); }
} // namespace tangier::params
