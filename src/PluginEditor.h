// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "PluginProcessor.h"

class TangierAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TangierAudioProcessorEditor (TangierAudioProcessor&);
    ~TangierAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TangierAudioProcessorEditor)
};
