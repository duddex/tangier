// Tangier - polyphonic virtual analogue synth, based on Valley Interzone
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"

TangierAudioProcessor::TangierAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("External", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "TangierState", tangier::params::createLayout())
{
}

void TangierAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    monoBuffer.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    extBuffer.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    engine.prepare (sampleRate);
    engine.setControls (readControls());
    keyboardState.reset();
}

void TangierAudioProcessor::releaseResources()
{
    engine.allNotesOff();
}

bool TangierAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    const auto& in = layouts.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

tangier::SynthControls TangierAudioProcessor::readControls() const
{
    namespace p = tangier::params;
    auto f = [this] (const char* id) { return state.getRawParameterValue (id)->load(); };
    auto i = [&f] (const char* id) { return juce::roundToInt (f (id)); };

    tangier::SynthControls c;
    c.glide = f (p::glide);
    c.pitchMod = f (p::pitchMod);
    c.pitchModSource = i (p::pitchModSource);
    c.pitchModEnvPolarity = i (p::pitchModEnvPolarity);
    c.octave = i (p::octave);
    c.coarse = f (p::coarse);
    c.coarseMode = i (p::coarseMode);
    c.fine = f (p::fine);
    c.pulseWidth = f (p::pulseWidth);
    c.pwmDepth = f (p::pwmDepth);
    c.pwmSource = i (p::pwmSource);
    c.pwmEnvPolarity = i (p::pwmEnvPolarity);
    c.subOctave = i (p::subOctave);

    c.sawLevel = f (p::sawLevel);
    c.pulseLevel = f (p::pulseLevel);
    c.subLevel = f (p::subLevel);
    c.subWave = i (p::subWave);
    c.noiseLevel = f (p::noiseLevel);
    c.noiseType = i (p::noiseType);
    c.extLevel = f (p::extLevel);

    c.cutoff = f (p::cutoff);
    c.resonance = f (p::resonance);
    c.highPass = f (p::highPass);
    c.poles = i (p::poles);
    c.filterEnv = f (p::filterEnv);
    c.filterEnvPolarity = i (p::filterEnvPolarity);
    c.filterLfo = f (p::filterLfo);
    c.keyTrack = f (p::keyTrack);
    c.velocityToCutoff = f (p::velocityToCutoff);
    c.modWheelToCutoff = f (p::modWheelToCutoff);

    c.lfoRate = f (p::lfoRate);
    c.lfoFine = f (p::lfoFine);
    c.lfoSlew = f (p::lfoSlew);
    c.lfoWave = i (p::lfoWave);
    c.lfoKeySync = i (p::lfoKeySync);

    c.attack = f (p::attack);
    c.decay = f (p::decay);
    c.sustain = f (p::sustain);
    c.release = f (p::release);
    c.envLength = i (p::envLength);
    c.envCycle = i (p::envCycle);

    c.vcaSource = i (p::vcaSource);
    c.aftertouchToVca = f (p::aftertouchToVca);

    c.voices = i (p::voices);
    c.bendRange = i (p::bendRange);
    c.velocitySensitivity = f (p::velocitySensitivity);
    c.volumeDb = f (p::volume);
    return c;
}

void TangierAudioProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        engine.pitchBend (static_cast<float> (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isControllerOfType (1))
        engine.modWheel (static_cast<float> (m.getControllerValue()) / 127.0f);
    else if (m.isSustainPedalOn())
        engine.sustainPedal (true);
    else if (m.isSustainPedalOff())
        engine.sustainPedal (false);
    else if (m.isChannelPressure())
        engine.channelPressure (static_cast<float> (m.getChannelPressureValue()) / 127.0f);
    else if (m.isAftertouch())
        engine.polyPressure (m.getNoteNumber(), static_cast<float> (m.getAfterTouchValue()) / 127.0f);
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff();
}

void TangierAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (static_cast<int> (monoBuffer.size()) < numSamples)
    {
        // Hosts may exceed the announced block size; grow rather than fail.
        monoBuffer.resize (static_cast<size_t> (numSamples));
        extBuffer.resize (static_cast<size_t> (numSamples));
    }

    // Mix the optional external input down to mono before the output overwrites it
    const auto inputBus = getBusBuffer (buffer, true, 0);
    const int numInputChannels = getBus (true, 0) != nullptr && getBus (true, 0)->isEnabled()
                                     ? inputBus.getNumChannels()
                                     : 0;
    const float* ext = nullptr;
    if (numInputChannels > 0)
    {
        const float scale = 1.0f / static_cast<float> (numInputChannels);
        std::fill_n (extBuffer.begin(), numSamples, 0.0f);
        for (int ch = 0; ch < numInputChannels; ++ch)
        {
            const float* in = inputBus.getReadPointer (ch);
            for (int s = 0; s < numSamples; ++s)
                extBuffer[static_cast<size_t> (s)] += in[s] * scale;
        }
        ext = extBuffer.data();
    }

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    engine.setControls (readControls());

    // Render in segments between MIDI events for sample-accurate timing
    int position = 0;
    for (const auto metadata : midi)
    {
        const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition);
        if (eventPos > position)
        {
            engine.render (monoBuffer.data() + position, ext != nullptr ? ext + position : nullptr, eventPos - position);
            position = eventPos;
        }
        handleMidi (metadata.getMessage());
    }

    if (position < numSamples)
        engine.render (monoBuffer.data() + position, ext != nullptr ? ext + position : nullptr, numSamples - position);

    auto output = getBusBuffer (buffer, false, 0);
    for (int ch = 0; ch < output.getNumChannels(); ++ch)
        output.copyFrom (ch, 0, monoBuffer.data(), numSamples);

    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    lfoDisplay.store (engine.getLfoValue());
    activeVoices.store (engine.getNumActiveVoices());
}

juce::AudioProcessorEditor* TangierAudioProcessor::createEditor()
{
    return new TangierAudioProcessorEditor (*this);
}

void TangierAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TangierAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TangierAudioProcessor();
}
