// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
constexpr int kMargin = 10;
constexpr int kHeaderHeight = 44;
constexpr int kRowHeight = 230;
constexpr int kKeyboardHeight = 70;
constexpr int kSectionPadding = 8;
constexpr int kTitleHeight = 22;
constexpr int kSliderWidth = 52;
constexpr int kChoiceWidth = 100;
constexpr int kChoicesPerColumn = 3;
constexpr int kLabelHeight = 16;

const juce::Colour kBackground { 0xff17181c };
const juce::Colour kHeader { 0xff202228 };
const juce::Colour kPanel { 0xff22252b };
const juce::Colour kPanelOutline { 0xff343841 };
const juce::Colour kControlBackground { 0xff191b20 };
const juce::Colour kText { 0xffe6e6e6 };
const juce::Colour kMutedText { 0xffa4a8b1 };
} // namespace

void TangierLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const auto centreY = static_cast<float> (y) + static_cast<float> (height) * 0.47f;
    const auto radius = static_cast<float> (std::min (width, height)) * 0.36f;
    const auto arcRadius = radius + 3.0f;
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const bool isActive = slider.isMouseOverOrDragging();

    g.setColour (juce::Colour (0xff111317).withAlpha (0.55f));
    g.fillEllipse (centreX - radius - 1.0f, centreY - radius - 1.0f,
                   (radius + 1.0f) * 2.0f, (radius + 1.0f) * 2.0f);
    g.setColour (juce::Colour (0xff343841));
    g.drawEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    g.setColour (juce::Colour (0xff292c33));
    g.fillEllipse (centreX - radius + 1.0f, centreY - radius + 1.0f,
                   (radius - 1.0f) * 2.0f, (radius - 1.0f) * 2.0f);

    juce::Path arc;
    arc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                       rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff3a3d45));
    g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    if (sliderPosProportional > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour (accent.withAlpha (isActive ? 1.0f : 0.9f));
        g.strokePath (valueArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    const auto markerRadius = radius * 0.68f;
    const juce::Point<float> markerStart { centreX + std::sin (angle) * radius * 0.24f,
                                           centreY - std::cos (angle) * radius * 0.24f };
    const juce::Point<float> markerEnd { centreX + std::sin (angle) * markerRadius,
                                         centreY - std::cos (angle) * markerRadius };
    g.setColour (accent.brighter (isActive ? 0.35f : 0.15f));
    g.drawLine ({ markerStart, markerEnd }, 2.0f);
    g.fillEllipse (centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f);
}

TangierAudioProcessorEditor::TangierAudioProcessorEditor (TangierAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    namespace id = tangier::params;

    lookAndFeel.setColour (juce::ResizableWindow::backgroundColourId, kBackground);
    lookAndFeel.setColour (juce::Label::textColourId, kText);
    lookAndFeel.setColour (juce::ComboBox::backgroundColourId, kControlBackground);
    lookAndFeel.setColour (juce::ComboBox::textColourId, kText);
    lookAndFeel.setColour (juce::ComboBox::outlineColourId, kPanelOutline);
    lookAndFeel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour (juce::Slider::textBoxTextColourId, kText.withAlpha (0.8f));
    lookAndFeel.setColour (juce::Slider::textBoxBackgroundColourId, kControlBackground);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9d5cc));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff292b31));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, kBackground);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        juce::Colour (0xffe8902c).withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        juce::Colour (0xffe8902c).withAlpha (0.55f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, juce::Colour (0xff202228));
    setLookAndFeel (&lookAndFeel);

    const juce::Colour orange { 0xffe8902c }, green { 0xff5fbf6a }, blue { 0xff4f9be0 },
        yellow { 0xffd9c13f }, red { 0xffe0574f }, purple { 0xffa77be0 }, grey { 0xffa0a4ad };

    auto& vco = addSection ("VCO", orange);
    add (vco, Kind::fader, id::glide, "Glide");
    add (vco, Kind::fader, id::pitchMod, "Mod");
    add (vco, Kind::knob, id::coarse, "Coarse");
    add (vco, Kind::knob, id::fine, "Fine");
    add (vco, Kind::fader, id::pulseWidth, "Width");
    add (vco, Kind::fader, id::pwmDepth, "PWM");
    add (vco, Kind::choice, id::octave, "Octave");
    add (vco, Kind::choice, id::coarseMode, "Coarse mode");
    add (vco, Kind::choice, id::pitchModSource, "Mod source");
    add (vco, Kind::choice, id::pitchModEnvPolarity, "Mod env polarity");
    add (vco, Kind::choice, id::pwmSource, "PWM source");
    add (vco, Kind::choice, id::pwmEnvPolarity, "PWM env polarity");

    auto& mixer = addSection ("Mixer", green);
    add (mixer, Kind::fader, id::sawLevel, "Saw");
    add (mixer, Kind::fader, id::pulseLevel, "Pulse");
    add (mixer, Kind::fader, id::subLevel, "Sub");
    add (mixer, Kind::fader, id::noiseLevel, "Noise");
    add (mixer, Kind::fader, id::extLevel, "Ext");
    add (mixer, Kind::choice, id::subOctave, "Sub octave");
    add (mixer, Kind::choice, id::subWave, "Sub wave");
    add (mixer, Kind::choice, id::noiseType, "Noise type");

    auto& vca = addSection ("VCA", purple);
    add (vca, Kind::knob, id::aftertouchToVca, "Aftertouch");
    add (vca, Kind::choice, id::vcaSource, "Source");

    auto& global = addSection ("Global", grey);
    add (global, Kind::knob, id::voices, "Voices");
    add (global, Kind::knob, id::bendRange, "Bend");
    add (global, Kind::knob, id::velocitySensitivity, "Velocity");
    add (global, Kind::fader, id::volume, "Volume");

    auto& filter = addSection ("Filter", blue);
    add (filter, Kind::fader, id::cutoff, "Freq");
    add (filter, Kind::fader, id::resonance, "Res");
    add (filter, Kind::fader, id::highPass, "HPF");
    add (filter, Kind::fader, id::filterEnv, "Env");
    add (filter, Kind::fader, id::filterLfo, "LFO");
    add (filter, Kind::fader, id::keyTrack, "Key");
    add (filter, Kind::knob, id::velocityToCutoff, "Vel");
    add (filter, Kind::knob, id::modWheelToCutoff, "Wheel");
    add (filter, Kind::choice, id::poles, "Poles");
    add (filter, Kind::choice, id::filterEnvPolarity, "Env polarity");

    auto& lfo = addSection ("LFO", yellow);
    add (lfo, Kind::fader, id::lfoRate, "Rate");
    add (lfo, Kind::knob, id::lfoFine, "Fine");
    add (lfo, Kind::knob, id::lfoSlew, "Slew");
    add (lfo, Kind::choice, id::lfoWave, "Wave");
    add (lfo, Kind::choice, id::lfoKeySync, "Sync");

    auto& env = addSection ("Envelope", red);
    add (env, Kind::fader, id::attack, "A");
    add (env, Kind::fader, id::decay, "D");
    add (env, Kind::fader, id::sustain, "S");
    add (env, Kind::fader, id::release, "R");
    add (env, Kind::choice, id::envLength, "Length");
    add (env, Kind::choice, id::envCycle, "Mode");

    keyboard.setOctaveForMiddleC (4);
    addAndMakeVisible (keyboard);

    // Size the window to fit the widest row of sections
    auto sectionWidth = [] (const Section& s)
    {
        int sliders = 0, choices = 0;
        for (const auto* c : s.controls)
            (c->kind == Kind::choice ? choices : sliders)++;
        const int choiceColumns = (choices + kChoicesPerColumn - 1) / kChoicesPerColumn;
        return 2 * kSectionPadding + sliders * kSliderWidth + choiceColumns * kChoiceWidth;
    };

    int row1 = 0, row2 = 0;
    for (size_t i = 0; i < sections.size(); ++i)
    {
        sections[i]->naturalWidth = sectionWidth (*sections[i]);
        (i < 4 ? row1 : row2) += sections[i]->naturalWidth + kMargin;
    }

    setSize (std::max (row1, row2) + kMargin, kHeaderHeight + 2 * (kRowHeight + kMargin) + kKeyboardHeight + kMargin);
    startTimerHz (30);
}

TangierAudioProcessorEditor::~TangierAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

TangierAudioProcessorEditor::Section& TangierAudioProcessorEditor::addSection (const juce::String& title,
                                                                                 juce::Colour colour)
{
    auto section = std::make_unique<Section>();
    section->title = title;
    section->colour = colour;
    sections.push_back (std::move (section));
    return *sections.back();
}

void TangierAudioProcessorEditor::add (Section& section, Kind kind, const juce::String& paramId,
                                       const juce::String& labelText)
{
    auto control = std::make_unique<Control>();
    control->kind = kind;
    control->paramId = paramId;

    control->label = std::make_unique<juce::Label> (juce::String(), labelText);
    control->label->setJustificationType (juce::Justification::centred);
    control->label->setFont (juce::FontOptions (12.0f));
    control->label->setColour (juce::Label::textColourId, kMutedText);
    addAndMakeVisible (*control->label);

    auto& state = processor.getState();

    if (kind == Kind::choice)
    {
        control->combo = std::make_unique<juce::ComboBox>();
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
            control->combo->addItemList (choice->choices, 1);
        addAndMakeVisible (*control->combo);
        control->comboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            state, paramId, *control->combo);
    }
    else
    {
        const auto style = kind == Kind::fader ? juce::Slider::LinearBarVertical : juce::Slider::RotaryVerticalDrag;
        control->slider = std::make_unique<juce::Slider> (style, juce::Slider::TextBoxBelow);
        control->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, kSliderWidth, 18);
        control->slider->setColour (juce::Slider::trackColourId, section.colour.withAlpha (0.85f));
        control->slider->setColour (juce::Slider::rotarySliderFillColourId, section.colour);
        control->slider->setColour (juce::Slider::thumbColourId, section.colour.brighter (0.3f));
        control->slider->setColour (juce::Slider::backgroundColourId, kControlBackground);
        if (kind == Kind::fader)
            control->slider->setTextBoxIsEditable (true);
        if (auto* param = state.getParameter (paramId))
            control->slider->setTooltip (param->getName (64));
        addAndMakeVisible (*control->slider);
        control->sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, paramId, *control->slider);
    }

    section.controls.push_back (control.get());
    controls.push_back (std::move (control));
}

void TangierAudioProcessorEditor::layoutSection (Section& section)
{
    auto area = section.bounds.reduced (kSectionPadding);
    area.removeFromTop (kTitleHeight);

    std::vector<Control*> choices;
    for (auto* c : section.controls)
    {
        if (c->kind == Kind::choice)
        {
            choices.push_back (c);
            continue;
        }

        auto column = area.removeFromLeft (kSliderWidth);
        c->label->setBounds (column.removeFromTop (kLabelHeight));

        if (c->kind == Kind::knob)
            c->slider->setBounds (column.removeFromTop (kSliderWidth + 22));
        else
            c->slider->setBounds (column.reduced (3, 2));
    }

    for (size_t i = 0; i < choices.size(); ++i)
    {
        if (i % kChoicesPerColumn == 0 && i > 0)
            area.removeFromLeft (kChoiceWidth);

        auto column = area.withWidth (kChoiceWidth).reduced (4, 0);
        const int slot = static_cast<int> (i % kChoicesPerColumn);
        auto cell = column.withHeight (52).withY (column.getY() + slot * 56);
        choices[i]->label->setBounds (cell.removeFromTop (kLabelHeight));
        choices[i]->combo->setBounds (cell.removeFromTop (24));
    }
}

void TangierAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    headerBounds = bounds.removeFromTop (kHeaderHeight);

    auto layoutRow = [this] (juce::Rectangle<int> row, size_t first, size_t last)
    {
        int total = 0;
        for (size_t i = first; i < last; ++i)
            total += sections[i]->naturalWidth;

        // Spread any extra width evenly over the sections in this row
        const int count = static_cast<int> (last - first);
        const int extra = std::max (0, (row.getWidth() - kMargin * (count + 1) - total) / count);

        row.removeFromLeft (kMargin);
        for (size_t i = first; i < last; ++i)
        {
            auto& s = *sections[i];
            s.bounds = row.removeFromLeft (s.naturalWidth + extra);
            row.removeFromLeft (kMargin);
            layoutSection (s);
        }
    };

    layoutRow (bounds.removeFromTop (kRowHeight), 0, 4);
    bounds.removeFromTop (kMargin);
    layoutRow (bounds.removeFromTop (kRowHeight), 4, sections.size());
    bounds.removeFromTop (kMargin);
    keyboard.setBounds (bounds.reduced (kMargin, 0).withTrimmedBottom (kMargin));

    // Show C1..C7 (MIDI 24..96): 6 octaves plus the top C = 43 white keys
    keyboard.setAvailableRange (24, 96);
    keyboard.setKeyWidth (static_cast<float> (keyboard.getWidth()) / 43.0f);
    keyboard.setLowestVisibleKey (24);
}

void TangierAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);

    // Header
    auto header = headerBounds.reduced (kMargin, 6);
    g.setColour (kHeader);
    g.fillRoundedRectangle (headerBounds.reduced (kMargin, 3).toFloat(), 7.0f);

    g.setColour (juce::Colour (0xffe8902c));
    g.fillRoundedRectangle (headerBounds.reduced (kMargin, 9).removeFromLeft (3).toFloat(), 1.5f);

    g.setColour (juce::Colour (0xfff0a34e));
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("TANGIER", header.removeFromLeft (140), juce::Justification::centredLeft);
    g.setColour (kMutedText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("POLYPHONIC SYNTH", header.removeFromLeft (360),
                juce::Justification::centredLeft);

    auto status = header.removeFromRight (220).withSizeKeepingCentre (220, 28);
    g.setColour (kControlBackground);
    g.fillRoundedRectangle (status.toFloat(), 8.0f);
    status = status.reduced (10, 0);

    g.setColour (kMutedText);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("VOICES", status.removeFromLeft (52), juce::Justification::centredLeft);
    g.setColour (kText);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (juce::String (voicesSounding), status.removeFromLeft (28), juce::Justification::centred);
    status.removeFromLeft (8);

    g.setColour (kMutedText);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("LFO", status.removeFromLeft (28), juce::Justification::centredRight);
    auto led = status.withSizeKeepingCentre (9, 9).toFloat();
    const float brightness = juce::jlimit (0.0f, 1.0f, lfoLed * 0.5f + 0.5f);
    g.setColour (juce::Colour (0xffd9c13f).withMultipliedBrightness (0.25f + 0.75f * brightness));
    g.fillEllipse (led);

    // Sections
    for (const auto& s : sections)
    {
        const auto r = s->bounds.toFloat();
        g.setColour (kPanel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (kPanelOutline);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (s->colour);
        g.fillRoundedRectangle (r.withHeight (3.0f).reduced (6.0f, 0.0f), 1.5f);
        g.setColour (s->colour.brighter (0.25f));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (s->title.toUpperCase(), s->bounds.reduced (kSectionPadding, 6).withHeight (kTitleHeight),
                    juce::Justification::centredLeft);
    }
}

void TangierAudioProcessorEditor::timerCallback()
{
    const float newLed = processor.getLfoDisplayValue();
    const int newVoices = processor.getActiveVoiceCount();

    if (std::abs (newLed - lfoLed) > 0.01f || newVoices != voicesSounding)
    {
        lfoLed = newLed;
        voicesSounding = newVoices;
        repaint (headerBounds);
    }
}
