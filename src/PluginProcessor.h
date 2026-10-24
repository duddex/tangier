// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "engine/SynthEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

class TangierAudioProcessor : public juce::AudioProcessor
{
public:
    TangierAudioProcessor();
    ~TangierAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() { return state; }

    /** Keyboard state shared with the on-screen keyboard in the editor. */
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    /** For the UI: current shared LFO value (-1..1) and number of sounding voices. */
    float getLfoDisplayValue() const { return lfoDisplay.load(); }
    int getActiveVoiceCount() const { return activeVoices.load(); }

private:
    tangier::SynthControls readControls() const;
    void handleMidi (const juce::MidiMessage& m);

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;
    tangier::SynthEngine engine;

    std::vector<float> monoBuffer;
    std::vector<float> extBuffer;

    std::atomic<float> lfoDisplay { 0.0f };
    std::atomic<int> activeVoices { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TangierAudioProcessor)
};
