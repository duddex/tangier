// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"

TangierAudioProcessorEditor::TangierAudioProcessorEditor (TangierAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    setSize (600, 400);
}

void TangierAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1d22));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f));
    g.drawFittedText ("Tangier", getLocalBounds(), juce::Justification::centred, 1);
}

void TangierAudioProcessorEditor::resized()
{
}
