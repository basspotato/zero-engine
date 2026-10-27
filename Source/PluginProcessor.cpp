#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "SynthVoice.h"
#include "Wavetable.h"
#include "Profiles.h"

namespace
{
    constexpr int numVoices = 16;
}

ZeroEngineProcessor::ZeroEngineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ZeroEngine", createParameterLayout()),
      macroMap (state),
      synth (state),
      grit (*state.getRawParameterValue (ParamIDs::grit)),
      air (*state.getRawParameterValue (ParamIDs::air)),
      width (*state.getRawParameterValue (ParamIDs::width)),
      output (*state.getRawParameterValue (ParamIDs::output)),
      levelTrim (*state.getRawParameterValue (ParamIDs::levelTrim)),
      subSplit (*state.getRawParameterValue (ParamIDs::subSplit)),
      subOn (*state.getRawParameterValue (ParamIDs::subOn)),
      midHighPass (*state.getRawParameterValue (ParamIDs::midHighPass)),
      ottDepth (*state.getRawParameterValue (ParamIDs::ottDepth)),
      tilt (*state.getRawParameterValue (ParamIDs::tilt)),
      monoFreq (*state.getRawParameterValue (ParamIDs::monoFreq)),
      stereoWidth (*state.getRawParameterValue (ParamIDs::stereoWidth)),
      delayMix (*state.getRawParameterValue (ParamIDs::delayMix)),
      delayTime (*state.getRawParameterValue (ParamIDs::delayTime)),
      delayFeedback (*state.getRawParameterValue (ParamIDs::delayFeedback)),
      reverbMix (*state.getRawParameterValue (ParamIDs::reverbMix)),
      reverbSize (*state.getRawParameterValue (ParamIDs::reverbSize))
{
    WavetableBank::get(); // build tables here, never on the audio thread

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new ZeroVoice (state, macroMap, hostBpm, i));

    synth.addSound (new ZeroSound());

    syncProfile(); // start on the default genre/type's profile
    startTimerHz (10);
}

void ZeroEngineProcessor::timerCallback()
{
    syncProfile();
}

void ZeroEngineProcessor::syncProfile()
{
    const auto genre = (int) state.getRawParameterValue (ParamIDs::genre)->load();
    const auto type = (int) state.getRawParameterValue (ParamIDs::type)->load();

    if (genre == appliedGenre && type == appliedType)
        return;

    loadProfile (genre, type, makeProfile (genre, type));
}

void ZeroEngineProcessor::loadProfile (int genre, int type, const Profile& profile)
{
    auto setChoice = [this] (const char* id, int index)
    {
        auto* param = state.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
    };

    appliedGenre = genre;
    appliedType = type;
    setChoice (ParamIDs::genre, genre);
    setChoice (ParamIDs::type, type);
    applyProfile (state, profile);
    loadMacroMap (macroMap, profile);
}

void ZeroEngineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    synthBuffer.setSize (3, juce::jmax (samplesPerBlock, 4096)); // L, R, clean sub
    finishing.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples (finishing.getLatencySamples());
    keyboardState.reset();
}

bool ZeroEngineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ZeroEngineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    // Host tempo for the delay and synced LFOs; 120 BPM when there is none (standalone).
    auto bpm = 120.0;
    if (auto* host = getPlayHead())
        if (const auto position = host->getPosition())
            if (const auto hostTempo = position->getBpm())
                bpm = juce::jlimit (40.0, 300.0, *hostTempo);
    hostBpm.store ((float) bpm);

    // Sub Split: voices write their sub to a third channel that skips the
    // chain. A block bigger than prepared falls back to the mixed path rather
    // than allocating here.
    const float* sub = nullptr;
    if (subSplit.load() > 0.5f && numSamples <= synthBuffer.getNumSamples())
    {
        juce::AudioBuffer<float> view (synthBuffer.getArrayOfWritePointers(), 3, numSamples);
        view.clear();
        synth.renderNextBlock (view, midi, 0, numSamples);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, view, juce::jmin (ch, 1), 0, numSamples);
        sub = view.getReadPointer (2);
    }
    else
    {
        synth.renderNextBlock (buffer, midi, 0, numSamples);
    }

    FinishingSettings settings;
    settings.grit     = grit.load();
    settings.ottDepth = juce::jlimit (0.0f, 1.0f, ottDepth.load() + macroMap.offset (MacroTarget::ottDepth));
    settings.tiltDb   = juce::jlimit (-6.0f, 6.0f, tilt.load() + macroMap.offset (MacroTarget::tilt));
    settings.air      = air.load();
    settings.monoFreq = monoFreq.load();
    settings.midHighPassHz = juce::jlimit (20.0f, 1000.0f, midHighPass.load() * std::exp2 (macroMap.offset (MacroTarget::midHighPass)));

    // With no sub (leads, chords, pads, plucks) nothing replaces what the
    // high-pass removes, so keep it below the lowest note's fundamental: a low
    // hoover or brass chord keeps its root. Basses keep the fixed split, since
    // their clean sub carries the fundamental.
    if (subOn.load() < 0.5f)
    {
        int lowest = 128;
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (auto* voice = synth.getVoice (i); voice->isVoiceActive())
                lowest = juce::jmin (lowest, voice->getCurrentlyPlayingNote());

        // About 3/4 octave under the fundamental, where the 12 dB slope costs it
        // under 0.5 dB. Glide there over ~10 ms; hold the last value in silence.
        if (lowest < 128)
        {
            const auto target = (float) juce::MidiMessage::getMidiNoteInHertz (lowest) * 0.6f;
            const auto glide = 1.0f - std::exp (-(float) numSamples / (0.01f * (float) getSampleRate()));
            noteHighPassHz *= std::exp2 (glide * std::log2 (target / noteHighPassHz));
        }
        settings.midHighPassHz = juce::jlimit (20.0f, settings.midHighPassHz, noteHighPassHz);
    }
    settings.sideGain = stereoWidth.load() * (0.5f + width.load()); // Width also sets unison spread in the voice
    settings.outputDb = output.load() + levelTrim.load();

    const auto beats = delayTimeBeats[juce::jlimit (0, (int) std::size (delayTimeBeats) - 1, (int) delayTime.load())];
    settings.delaySeconds  = (float) (beats * 60.0 / bpm);
    settings.delayMix      = juce::jlimit (0.0f, 1.0f, delayMix.load() + macroMap.offset (MacroTarget::delayMix));
    settings.delayFeedback = delayFeedback.load();
    settings.reverbMix     = juce::jlimit (0.0f, 1.0f, reverbMix.load() + macroMap.offset (MacroTarget::reverbMix));
    settings.reverbSize    = reverbSize.load();

    finishing.process (buffer, settings, sub);
}

juce::AudioProcessorEditor* ZeroEngineProcessor::createEditor()
{
    return new ZeroEngineEditor (*this);
}

void ZeroEngineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ZeroEngineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            state.replaceState (juce::ValueTree::fromXml (*xml));

            // The saved values already include any tweaks; don't overwrite them with the profile.
            appliedGenre = (int) state.getRawParameterValue (ParamIDs::genre)->load();
            appliedType = (int) state.getRawParameterValue (ParamIDs::type)->load();
            loadMacroMap (macroMap, makeProfile (appliedGenre, appliedType));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZeroEngineProcessor();
}
