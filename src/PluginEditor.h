// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

class TangierAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit TangierAudioProcessorEditor (TangierAudioProcessor&);
    ~TangierAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Kind
    {
        fader,
        knob,
        choice
    };

    struct Control
    {
        Kind kind;
        juce::String paramId;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
    };

    struct Section
    {
        juce::String title;
        juce::Colour colour;
        std::vector<Control*> controls;
        int naturalWidth = 0;
        juce::Rectangle<int> bounds;
    };

    Section& addSection (const juce::String& title, juce::Colour colour);
    void add (Section& section, Kind kind, const juce::String& paramId, const juce::String& labelText);
    void layoutSection (Section& section);
    void timerCallback() override;

    TangierAudioProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;

    std::vector<std::unique_ptr<Control>> controls;
    std::vector<std::unique_ptr<Section>> sections;

    juce::MidiKeyboardComponent keyboard;
    juce::Rectangle<int> headerBounds;
    float lfoLed = 0.0f;
    int voicesSounding = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TangierAudioProcessorEditor)
};
