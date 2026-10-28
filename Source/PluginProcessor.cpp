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
      spectral (*state.getRawParameterValue (ParamIDs::spectral)),
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
        synth.addVoice (new ZeroVoice (state, macroMap, hostBpm, livePosition, i));

    synth.addSound (new ZeroSound());

    for (auto [slot, id] : { std::pair { 0, ParamIDs::arpOn }, { 1, ParamIDs::arpMode }, { 2, ParamIDs::arpRate }, { 3, ParamIDs::arpOctaves }, { 4, ParamIDs::arpGate } })
        arpParams[slot] = state.getRawParameterValue (id);
    seqLengthParam = state.getRawParameterValue (ParamIDs::seqLength);
    for (auto [m, id] : { std::pair { 0, ParamIDs::weight }, { 1, ParamIDs::grit }, { 2, ParamIDs::punch }, { 3, ParamIDs::width }, { 4, ParamIDs::motion }, { 5, ParamIDs::air } })
        macroParams[m] = state.getParameter (id);
    for (int step = 0; step < Arpeggiator::maxSteps; ++step)
    {
        seqParams[0][step] = state.getRawParameterValue (SeqIDs::on (step));
        seqParams[1][step] = state.getRawParameterValue (SeqIDs::pitch (step));
        seqParams[2][step] = state.getRawParameterValue (SeqIDs::accent (step));
    }

    syncProfile(); // start on the default genre/type's profile
    startTimerHz (10);
}

void ZeroEngineProcessor::timerCallback()
{
    syncProfile();
    if (! juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
        checkpoint(); // after syncProfile, so a Genre/Type change and its new sound are one step
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

void ZeroEngineProcessor::generate (juce::int64 seed)
{
    syncProfile(); // a Genre/Type change not yet picked up by the timer
    juce::Random random (seed);
    const int genre = appliedGenre, type = appliedType;

    // History, like a browser: going back and generating drops what was ahead;
    // the sound before the first click (or any tweak since) is kept as a step too.
    history.erase (history.begin() + juce::jmin ((int) history.size(), historyPos + 1), history.end());
    if (history.empty() || ! history.back().isEquivalentTo (state.state))
        history.push_back (state.copyState());

    loadProfile (genre, type, pickGeneratedProfile (genre, type, random));

    history.push_back (state.copyState());
    constexpr size_t maxHistory = 50;
    if (history.size() > maxHistory)
        history.erase (history.begin(), history.begin() + (std::ptrdiff_t) (history.size() - maxHistory));
    historyPos = (int) history.size() - 1;
}

bool ZeroEngineProcessor::stepHistory (int direction)
{
    const auto target = historyPos + direction;
    if (target < 0 || target >= (int) history.size())
        return false;

    historyPos = target;
    restoreTree (history[(size_t) historyPos]);
    return true;
}

ZeroEngineProcessor::SoundCheck ZeroEngineProcessor::measureProfile (int genre, int type, const Profile& profile)
{
    // A third of a second of a note (or a triad for chords and pads) through a private
    // engine, then the four measurements that explain most mix-to-mix variation:
    // loudness, brightness, bass and width.
    constexpr double rate = 48000.0;
    constexpr int block = 512, length = 16384; // 0.34 s
    constexpr int fftOrder = 12, fftSize = 1 << fftOrder;

    if (checker == nullptr)
        checker = std::make_unique<ZeroEngineProcessor>();
    checker->setPlayConfigDetails (0, 2, rate, block);
    checker->prepareToPlay (rate, block); // fresh voices and tails for every candidate
    // Measure the tone, not the fade-in: slow attacks (pads, FX) would otherwise
    // be compared part-way up their swell.
    auto heard = profile;
    heard.ampAttack = juce::jmin (heard.ampAttack, 0.01f);
    heard.modAttack = juce::jmin (heard.modAttack, 0.01f);
    checker->loadProfile (genre, type, heard);

    juce::AudioBuffer<float> out (2, length), chunk (2, block);
    juce::MidiBuffer midi;
    const auto root = type == 0 ? 36 : type == 4 || type == 1 ? 72 : 60; // bass C2, lead/pluck C5, else C4
    const auto chord = type == 2 || type == 3;
    constexpr int intervals[] { 0, 4, 7 };

    for (int pos = 0; pos < length; pos += block)
    {
        midi.clear();
        if (pos == 0)
            for (int i = 0; i < (chord ? 3 : 1); ++i)
                midi.addEvent (juce::MidiMessage::noteOn (1, root + intervals[i], (juce::uint8) 110), 0);
        chunk.clear();
        checker->processBlock (chunk, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, chunk, ch, 0, juce::jmin (block, length - pos));
    }

    SoundCheck m;
    double mid2 = 0.0, side2 = 0.0;
    for (int i = 0; i < length; ++i)
    {
        const auto l = out.getSample (0, i), r = out.getSample (1, i);
        mid2 += 0.25 * (l + r) * (l + r);
        side2 += 0.25 * (l - r) * (l - r);
    }
    m.levelDb = (float) juce::Decibels::gainToDecibels (std::sqrt ((mid2 + side2) / length), -100.0);
    m.sidePct = (float) (100.0 * side2 / juce::jmax (1.0e-12, mid2 + side2));

    // Spectrum of the held part (the last 4096 samples, mid channel).
    std::vector<float> fft ((size_t) fftSize * 2, 0.0f);
    for (int i = 0; i < fftSize; ++i)
    {
        const auto s = length - fftSize + i;
        const auto hann = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1));
        fft[(size_t) i] = 0.5f * (out.getSample (0, s) + out.getSample (1, s)) * hann;
    }
    juce::dsp::FFT (fftOrder).performFrequencyOnlyForwardTransform (fft.data());

    double total = 0.0, weighted = 0.0, bass = 0.0;
    for (int bin = 1; bin < fftSize / 2; ++bin)
    {
        const auto hz = bin * rate / fftSize;
        const auto e = (double) fft[(size_t) bin] * fft[(size_t) bin];
        total += e;
        weighted += e * hz;
        if (hz < 150.0)
            bass += e;
    }
    m.centroidHz = (float) (total > 0.0 ? weighted / total : 0.0);
    m.bassPct = (float) (total > 0.0 ? 100.0 * bass / total : 0.0);
    return m;
}

Profile ZeroEngineProcessor::pickGeneratedProfile (int genre, int type, juce::Random& random)
{
    // Render-and-check: several variations, each measured against the designed
    // profile; keep those that still sit in the genre/type's character, then
    // choose among them at random so results stay varied. If none fit, the
    // closest one wins.
    constexpr int candidates = 6;
    // The designed profile never changes while running, so measure it once.
    auto& cached = designedChecks[(size_t) (genre * 6 + type)];
    if (! cached.has_value())
        cached = measureProfile (genre, type, makeProfile (genre, type));
    const auto target = *cached;

    std::vector<std::pair<Profile, float>> fits;
    Profile closest {};
    auto closestDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < candidates; ++i)
    {
        const auto profile = generateProfile (genre, type, random);
        const auto m = measureProfile (genre, type, profile);

        // Each term is 1.0 at the edge of "still the same kind of sound".
        const auto loudness = std::abs (m.levelDb - target.levelDb) / 6.0f;
        const auto brightness = std::abs (std::log2 (juce::jmax (20.0f, m.centroidHz) / juce::jmax (20.0f, target.centroidHz))) / 0.8f;
        const auto bass = std::abs (m.bassPct - target.bassPct) / 20.0f;
        const auto spread = std::abs (m.sidePct - target.sidePct) / 20.0f;
        const auto worst = juce::jmax (juce::jmax (loudness, brightness), juce::jmax (bass, spread));
        const auto silent = m.levelDb < target.levelDb - 20.0f;

        if (! silent && worst <= 1.0f)
            fits.push_back ({ profile, worst });
        if (! silent && worst < closestDistance)
        {
            closest = profile;
            closestDistance = worst;
        }
    }

    lastGenerateFits = (int) fits.size();
    if (! fits.empty())
        return fits[(size_t) random.nextInt ((int) fits.size())].first;
    return closestDistance < std::numeric_limits<float>::max() ? closest : makeProfile (genre, type);
}

void ZeroEngineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    synthBuffer.setSize (3, juce::jmax (samplesPerBlock, 4096)); // L, R, clean sub
    finishing.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    arp.prepare (sampleRate, samplesPerBlock);
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

    // Controllers: the mod wheel (CC 1) rides on top of Motion; MIDI learn takes
    // the next CC for the macro waiting to learn; learned CCs move their macros.
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (! message.isController())
            continue;

        const auto cc = message.getControllerNumber();
        const auto value = (float) message.getControllerValue() / 127.0f;
        if (cc == 1)
            macroMap.setModWheel (value);

        if (const auto learning = learningMacro.exchange (-1); learning >= 0)
            macroCC[(size_t) learning] = cc;

        for (size_t m = 0; m < macroCC.size(); ++m)
            if (macroCC[m].load() == cc)
                macroParams[m]->setValueNotifyingHost (value);
    }

    // Host tempo for the delay and synced LFOs; 120 BPM when there is none (standalone).
    auto bpm = 120.0;
    if (auto* host = getPlayHead())
        if (const auto position = host->getPosition())
            if (const auto hostTempo = position->getBpm())
                bpm = juce::jlimit (40.0, 300.0, *hostTempo);
    hostBpm.store ((float) bpm);
    livePosition.store (-1.0f, std::memory_order_relaxed); // voices that play this block overwrite it

    // Arpeggiator: the keys in, the pattern out (the input itself when it's off).
    Arpeggiator::Settings arpSettings;
    arpSettings.on = arpParams[0]->load() > 0.5f;
    arpSettings.mode = (int) arpParams[1]->load();
    arpSettings.beatsPerStep = arpRateBeats[juce::jlimit (0, (int) std::size (arpRateBeats) - 1, (int) arpParams[2]->load())];
    arpSettings.octaves = (int) arpParams[3]->load();
    arpSettings.gate = arpParams[4]->load();
    arpSettings.bpm = bpm;
    arpSettings.length = (int) seqLengthParam->load();
    for (int step = 0; step < Arpeggiator::maxSteps; ++step)
    {
        arpSettings.stepOn[(size_t) step] = seqParams[0][step]->load() > 0.5f;
        arpSettings.stepPitch[(size_t) step] = juce::roundToInt (seqParams[1][step]->load());
        arpSettings.stepAccent[(size_t) step] = seqParams[2][step]->load() > 0.5f;
    }
    const auto& notes = arp.process (midi, numSamples, arpSettings);

    // Sub Split: voices write their sub to a third channel that skips the
    // chain. A block bigger than prepared falls back to the mixed path rather
    // than allocating here.
    const float* sub = nullptr;
    if (subSplit.load() > 0.5f && numSamples <= synthBuffer.getNumSamples())
    {
        juce::AudioBuffer<float> view (synthBuffer.getArrayOfWritePointers(), 3, numSamples);
        view.clear();
        synth.renderNextBlock (view, notes, 0, numSamples);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, view, juce::jmin (ch, 1), 0, numSamples);
        sub = view.getReadPointer (2);
    }
    else
    {
        synth.renderNextBlock (buffer, notes, 0, numSamples);
    }

    FinishingSettings settings;
    settings.grit     = grit.load();
    settings.ottDepth = juce::jlimit (0.0f, 1.0f, ottDepth.load() + macroMap.offset (MacroTarget::ottDepth));
    settings.spectral = spectral.load();
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

    // Display tap.
    auto write = scopeWrite.load (std::memory_order_relaxed);
    const auto* left = buffer.getReadPointer (0);
    const auto* right = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
    for (int i = 0; i < numSamples; ++i)
    {
        scope[(size_t) write] = 0.5f * (left[i] + right[i]);
        write = (write + 1) % scopeSize;
    }
    scopeWrite.store (write, std::memory_order_release);
}

void ZeroEngineProcessor::copyScope (float* dest, int numSamples) const noexcept
{
    numSamples = juce::jmin (numSamples, scopeSize);
    const auto end = scopeWrite.load (std::memory_order_acquire);
    for (int i = 0; i < numSamples; ++i)
        dest[i] = scope[(size_t) ((end - numSamples + i + scopeSize) % scopeSize)];
}

juce::AudioProcessorEditor* ZeroEngineProcessor::createEditor()
{
    return new ZeroEngineEditor (*this);
}

void ZeroEngineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
    {
        // MIDI learn belongs to the project (your controller), not to a sound, so it
        // is saved here and not in the state tree that favourites copy.
        juce::StringArray ccs;
        for (const auto& cc : macroCC)
            ccs.add (juce::String (cc.load()));
        xml->setAttribute ("macroCCs", ccs.joinIntoString (","));
        copyXmlToBinary (*xml, destData);
    }
}

void ZeroEngineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        restoreState (*xml);
        const auto ccs = juce::StringArray::fromTokens (xml->getStringAttribute ("macroCCs"), ",", {});
        for (size_t m = 0; m < macroCC.size(); ++m)
            macroCC[m] = m < (size_t) ccs.size() ? ccs[(int) m].getIntValue() : -1;
    }
}

void ZeroEngineProcessor::checkpoint()
{
    auto now = state.copyState();
    if (! lastKnown.isValid())
    {
        lastKnown = now; // the starting sound; nothing to undo yet
        return;
    }
    if (now.isEquivalentTo (lastKnown))
        return;

    constexpr size_t maxSteps = 100;
    undoStack.push_back (lastKnown);
    if (undoStack.size() > maxSteps)
        undoStack.erase (undoStack.begin());
    redoStack.clear();
    lastKnown = now;
}

bool ZeroEngineProcessor::undo()
{
    checkpoint(); // a change not yet recorded is the step to undo
    if (undoStack.empty())
        return false;

    redoStack.push_back (state.copyState());
    restoreTree (undoStack.back());
    undoStack.pop_back();
    lastKnown = state.copyState();
    return true;
}

bool ZeroEngineProcessor::redo()
{
    checkpoint();
    if (redoStack.empty())
        return false;

    undoStack.push_back (state.copyState());
    restoreTree (redoStack.back());
    redoStack.pop_back();
    lastKnown = state.copyState();
    return true;
}

void ZeroEngineProcessor::adoptCurrentGenreAndType()
{
    // An undone Genre/Type change brings back that sound's values with it; take
    // them as they are rather than letting syncProfile reload the designed profile.
    appliedGenre = (int) state.getRawParameterValue (ParamIDs::genre)->load();
    appliedType = (int) state.getRawParameterValue (ParamIDs::type)->load();
    loadMacroMap (macroMap, makeProfile (appliedGenre, appliedType));
}

bool ZeroEngineProcessor::restoreState (const juce::XmlElement& xml)
{
    if (! xml.hasTagName (state.state.getType()))
        return false;

    restoreTree (juce::ValueTree::fromXml (xml));
    return true;
}

void ZeroEngineProcessor::restoreTree (const juce::ValueTree& tree)
{
    state.replaceState (tree.createCopy());

    adoptCurrentGenreAndType(); // the saved values already include any tweaks; keep them
}

juce::String ZeroEngineProcessor::saveFavourite()
{
    favouritesFolder.createDirectory();
    const auto base = genreNames[appliedGenre] + " " + typeNames[appliedType];

    int number = 1;
    while (favouritesFolder.getChildFile (base + " " + juce::String (number) + ".xml").exists())
        ++number;

    const auto name = base + " " + juce::String (number);
    if (auto xml = state.copyState().createXml())
        if (xml->writeTo (favouritesFolder.getChildFile (name + ".xml")))
            return name;
    return {};
}

juce::StringArray ZeroEngineProcessor::getFavourites() const
{
    auto files = favouritesFolder.findChildFiles (juce::File::findFiles, false, "*.xml");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getLastModificationTime() > b.getLastModificationTime(); });

    juce::StringArray names;
    for (const auto& file : files)
        names.add (file.getFileNameWithoutExtension());
    return names;
}

bool ZeroEngineProcessor::deleteFavourite (const juce::String& name)
{
    return favouritesFolder.getChildFile (name + ".xml").deleteFile();
}

juce::String ZeroEngineProcessor::renameFavourite (const juce::String& name, const juce::String& newName)
{
    const auto clean = juce::File::createLegalFileName (newName.trim());
    const auto target = favouritesFolder.getChildFile (clean + ".xml");
    if (clean.isEmpty() || target.exists())
        return {};
    return favouritesFolder.getChildFile (name + ".xml").moveFileTo (target) ? clean : juce::String();
}

bool ZeroEngineProcessor::loadFavourite (const juce::String& name)
{
    if (auto xml = juce::XmlDocument::parse (favouritesFolder.getChildFile (name + ".xml")))
        return restoreState (*xml);
    return false;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZeroEngineProcessor();
}
