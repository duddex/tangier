// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

TangierAudioProcessor::TangierAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void TangierAudioProcessor::prepareToPlay (double, int)
{
}

void TangierAudioProcessor::releaseResources()
{
}

bool TangierAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void TangierAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
}

juce::AudioProcessorEditor* TangierAudioProcessor::createEditor()
{
    return new TangierAudioProcessorEditor (*this);
}

void TangierAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void TangierAudioProcessor::setStateInformation (const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TangierAudioProcessor();
}
