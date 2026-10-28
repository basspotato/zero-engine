// ZeroAnalyze test: engine checks that don't need references.
//
// For every profile, at several sample rates and block sizes: output is
// finite, not silent, stays under the clipper's ceiling, and dies away after
// the note (no runaway delay feedback or filter self-oscillation). Also checks
// that processBlock never allocates, and that saved state loads back exactly.

#include <juce_audio_processors/juce_audio_processors.h>
#include "Tests.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"
#include "../../Source/SpectralCompressor.h"
#include "../../Source/Arpeggiator.h"
#include <cstdlib>
#include <new>

//==============================================================================
// Count heap allocations made while the audio thread is inside processBlock.
namespace
{
    std::atomic<int> audioAllocations { 0 };
    thread_local bool insideProcessBlock = false;

    void* allocate (std::size_t size)
    {
        if (insideProcessBlock)
            ++audioAllocations;
        if (auto* p = std::malloc (size != 0 ? size : 1))
            return p;
        throw std::bad_alloc();
    }
}

void* operator new (std::size_t size)                                   { return allocate (size); }
void* operator new[] (std::size_t size)                                 { return allocate (size); }
void* operator new (std::size_t size, const std::nothrow_t&) noexcept   { try { return allocate (size); } catch (...) { return nullptr; } }
void* operator new[] (std::size_t size, const std::nothrow_t&) noexcept { try { return allocate (size); } catch (...) { return nullptr; } }
void operator delete (void* p) noexcept                                 { std::free (p); }
void operator delete[] (void* p) noexcept                               { std::free (p); }
void operator delete (void* p, std::size_t) noexcept                    { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept                  { std::free (p); }

//==============================================================================
namespace
{
    struct Failure
    {
        juce::String what;
    };

    void loadProfile (ZeroEngineProcessor& engine, int genre, int type)
    {
        auto setChoice = [&] (const char* id, int index)
        {
            auto* param = engine.state.getParameter (id);
            param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        };
        setChoice (ParamIDs::genre, genre);
        setChoice (ParamIDs::type, type);
        engine.syncProfile();
    }

    // Plays a chord of three notes for a second, then 8 s of silence.
    // Returns problems found; empty means it passed.
    juce::StringArray checkRender (int genre, int type, double rate, int blockSize, juce::int64 generateSeed = 0, float spectral = -1.0f, bool arp = false)
    {
        ZeroEngineProcessor engine;
        loadProfile (engine, genre, type);
        if (generateSeed != 0)
            engine.generate (generateSeed); // a Generate-button variation instead of the designed profile
        if (arp)
        {
            auto* q = engine.state.getParameter (ParamIDs::arpOn);
            q->setValueNotifyingHost (1.0f);
        }
        if (spectral >= 0.0f) // force the spectral stage to this amount
        {
            auto* q = engine.state.getParameter (ParamIDs::spectral);
            q->setValueNotifyingHost (q->convertTo0to1 (spectral));
        }
        engine.setPlayConfigDetails (0, 2, rate, blockSize);
        engine.prepareToPlay (rate, blockSize);

        const auto noteOff = (int) (1.0 * rate);
        const auto total = (int) (9.0 * rate);
        const auto tailStart = total - (int) (0.5 * rate);

        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        midi.ensureSize (256); // so the harness itself never allocates mid-render

        float peak = 0.0f, tailPeak = 0.0f;
        double energy = 0.0;
        bool finite = true;
        const auto allocationsBefore = audioAllocations.load();

        for (int pos = 0; pos < total; pos += blockSize)
        {
            const auto n = juce::jmin (blockSize, total - pos);
            block.setSize (2, n, false, false, true);
            midi.clear();

            for (auto note : { 48, 55, 60 })
            {
                if (pos == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 0);
                if (noteOff >= pos && noteOff < pos + n)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note), noteOff - pos);
            }

            insideProcessBlock = true;
            engine.processBlock (block, midi);
            insideProcessBlock = false;

            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* data = block.getReadPointer (ch);
                for (int i = 0; i < n; ++i)
                {
                    const auto x = data[i];
                    if (! std::isfinite (x))
                        finite = false;
                    const auto a = std::abs (x);
                    peak = juce::jmax (peak, a);
                    energy += (double) x * x;
                    if (pos + i >= tailStart)
                        tailPeak = juce::jmax (tailPeak, a);
                }
            }
        }

        juce::StringArray problems;
        if (! finite)                                          problems.add ("non-finite output");
        if (energy < 1.0e-6)                                   problems.add ("silent");
        if (peak > 1.001f)                                     problems.add ("peak " + juce::String (juce::Decibels::gainToDecibels (peak), 2) + " dBFS above the clipper ceiling");
        if (tailPeak > juce::Decibels::decibelsToGain (-60.0f)) problems.add ("still " + juce::String (juce::Decibels::gainToDecibels (tailPeak), 1) + " dBFS 8 s after note-off");
        if (const auto a = audioAllocations.load() - allocationsBefore; a > 0)
            problems.add (juce::String (a) + " allocations inside processBlock");
        return problems;
    }

    // Saved state must restore every parameter exactly, without the profile
    // overwriting tweaks on load.
    // A generated sound saved as a favourite must come back exactly, in a new
    // instance, from a scratch folder (never the owner's real favourites).
    juce::StringArray checkFavourites()
    {
        juce::TemporaryFile folderName;
        const auto folder = folderName.getFile();
        folder.createDirectory();

        ZeroEngineProcessor a;
        a.setFavouritesFolder (folder);
        loadProfile (a, genreNames.indexOf ("Trap"), typeNames.indexOf ("Pad"));
        a.generate (12345);
        const auto name = a.saveFavourite();

        ZeroEngineProcessor b;
        b.setFavouritesFolder (folder);
        juce::StringArray problems;
        if (name.isEmpty() || ! b.getFavourites().contains (name) || ! b.loadFavourite (name))
            problems.add ("could not save and reload \"" + name + "\"");
        b.syncProfile(); // a timer tick after loading must not reapply the profile

        for (auto* param : a.getParameters())
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
                if (std::abs (param->getValue() - b.state.getParameter (withId->getParameterID())->getValue()) > 1.0e-5f)
                    problems.add (withId->getParameterID() + " differs after reload");

        // Rename, refuse a name that's taken, then delete.
        const auto second = a.saveFavourite();
        if (b.renameFavourite (name, "My Pad") != "My Pad" || ! b.getFavourites().contains ("My Pad") || b.getFavourites().contains (name))
            problems.add ("rename failed");
        if (b.renameFavourite ("My Pad", second).isNotEmpty())
            problems.add ("rename overwrote another favourite");
        if (! b.deleteFavourite ("My Pad") || b.getFavourites().contains ("My Pad"))
            problems.add ("delete failed");

        folder.deleteRecursively();
        return problems;
    }

    // Back and forward through generated sounds must land on exactly what was
    // heard, including the designed sound from before the first click.
    juce::StringArray checkHistory()
    {
        ZeroEngineProcessor engine;
        loadProfile (engine, genreNames.indexOf ("Techno"), typeNames.indexOf ("Bass"));
        const auto designed = engine.state.copyState();
        engine.generate (1);
        const auto first = engine.state.copyState();
        engine.generate (2);
        const auto second = engine.state.copyState();

        juce::StringArray problems;
        auto expect = [&] (bool ok, const char* what) { if (! ok) problems.add (what); };
        auto now = [&] (const juce::ValueTree& t) { return engine.state.copyState().isEquivalentTo (t); };

        expect (! first.isEquivalentTo (second), "two generates gave the same sound");
        expect (engine.stepHistory (-1) && now (first), "back did not return to the first generated sound");
        expect (engine.stepHistory (-1) && now (designed), "back did not return to the designed sound");
        expect (! engine.stepHistory (-1), "back went past the start");
        expect (engine.stepHistory (1) && engine.stepHistory (1) && now (second), "forward did not return to the newest");
        expect (! engine.stepHistory (1), "forward went past the end");
        return problems;
    }

    // At 0 the spectral stage must pass the signal through untouched, only
    // delayed by its latency; at full it must stay finite and sane in level.
    juce::StringArray checkSpectralStage()
    {
        juce::StringArray problems;
        for (const auto amount : { 0.0f, 1.0f })
        {
            SpectralCompressor stage;
            stage.prepare (48000.0, 2);
            const auto latency = stage.getLatencySamples();
            const auto length = 48000;

            juce::Random random (7);
            juce::AudioBuffer<float> in (2, length), out (2, length);
            for (int i = 0; i < length; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    in.setSample (ch, i, 0.3f * std::sin (0.031f * (float) i * (float) (ch + 1)) + 0.1f * (random.nextFloat() - 0.5f));
            out.makeCopyOf (in);
            for (int start = 0; start < length; start += 333) // odd block sizes on purpose
                {
                    juce::AudioBuffer<float> chunk (out.getArrayOfWritePointers(), 2, start, juce::jmin (333, length - start));
                    stage.process (chunk, chunk.getNumSamples(), amount);
                }

            double error = 0.0, inEnergy = 0.0, outEnergy = 0.0;
            bool finite = true;
            for (int i = latency; i < length; ++i)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto x = in.getSample (ch, i - latency), y = out.getSample (ch, i);
                    finite = finite && std::isfinite (y);
                    error += (x - y) * (x - y);
                    inEnergy += x * x;
                    outEnergy += y * y;
                }

            if (! finite)
                problems.add ("non-finite output at amount " + juce::String (amount));
            else if (amount == 0.0f && error > inEnergy * 1.0e-8) // under -80 dB
                problems.add ("not clean at 0: residual " + juce::String (10.0 * std::log10 (error / inEnergy), 1) + " dB");
            else if (amount > 0.0f && std::abs (10.0 * std::log10 (outEnergy / inEnergy)) > 12.0)
                problems.add ("level moved more than 12 dB at full");
        }
        return problems;
    }

    // The arp plays the held chord in order, on time, and stops when the keys lift.
    juce::StringArray checkArpeggiator()
    {
        Arpeggiator arp;
        arp.prepare (48000.0, 512);
        Arpeggiator::Settings s;
        s.on = true;
        s.mode = Arpeggiator::up;
        s.beatsPerStep = 0.25; // 1/16 at 120 BPM: a step every 6000 samples
        s.bpm = 120.0;

        juce::MidiBuffer keys;
        std::vector<std::pair<int, int>> ons; // (sample, note)
        int offsTotal = 0, onsAfterRelease = 0;
        const auto releaseAt = 48000; // keys lift after 1 s

        for (int block = 0; block < 48000 * 2 / 512; ++block)
        {
            const auto start = block * 512;
            keys.clear();
            if (block == 0)
                for (auto note : { 64, 60, 67 }) // pressed out of order
                    keys.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            if (start <= releaseAt && releaseAt < start + 512)
                for (auto note : { 60, 64, 67 })
                    keys.addEvent (juce::MidiMessage::noteOff (1, note), releaseAt - start);

            for (const auto m : arp.process (keys, 512, s))
            {
                const auto at = start + m.samplePosition;
                const auto msg = m.getMessage();
                if (msg.isNoteOn())
                {
                    ons.push_back ({ at, msg.getNoteNumber() });
                    onsAfterRelease += at > releaseAt ? 1 : 0;
                }
                else if (msg.isNoteOff())
                    ++offsTotal;
            }
        }

        juce::StringArray problems;
        const int expected[] { 60, 64, 67, 60, 64, 67, 60, 64 };
        if (ons.size() != std::size (expected))
            problems.add (juce::String ((int) ons.size()) + " notes in 1 s, expected 8");
        for (size_t i = 0; i < juce::jmin (ons.size(), std::size (expected)); ++i)
        {
            if (ons[i].second != expected[i])
                problems.add ("step " + juce::String ((int) i) + " played " + juce::String (ons[i].second));
            if (std::abs (ons[i].first - (int) i * 6000) > 1)
                problems.add ("step " + juce::String ((int) i) + " at sample " + juce::String (ons[i].first));
        }
        if (onsAfterRelease > 0 || offsTotal != (int) ons.size()) // nothing new after release, nothing left hanging
            problems.add ("did not stop cleanly when the keys lifted");
        return problems;
    }

    // Latch: the pattern keeps playing after the keys lift; a fresh key replaces it.
    juce::StringArray checkLatch()
    {
        Arpeggiator arp;
        arp.prepare (48000.0, 512);
        Arpeggiator::Settings s;
        s.on = true;
        s.latch = true;
        s.beatsPerStep = 0.25;
        s.bpm = 120.0;

        std::vector<std::pair<int, int>> ons;
        juce::MidiBuffer keys;
        for (int block = 0; block < 48000 * 2 / 512; ++block) // 2 s
        {
            const auto start = block * 512;
            keys.clear();
            if (block == 0)
                for (auto note : { 60, 64, 67 })
                    keys.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            if (start <= 24000 && 24000 < start + 512) // let go at 0.5 s
                for (auto note : { 60, 64, 67 })
                    keys.addEvent (juce::MidiMessage::noteOff (1, note), 24000 - start);
            if (start <= 60000 && 60000 < start + 512) // a fresh key at 1.25 s
                keys.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100), 60000 - start);
            for (const auto m : arp.process (keys, 512, s))
                if (m.getMessage().isNoteOn())
                    ons.push_back ({ start + m.samplePosition, m.getMessage().getNoteNumber() });
        }

        juce::StringArray problems;
        bool afterRelease = false, chordAfterNewKey = false, newKeyPlays = false;
        for (const auto& [at, note] : ons)
        {
            afterRelease |= at > 30000 && at < 60000;
            chordAfterNewKey |= at > 60000 && note != 62;
            newKeyPlays |= at >= 60000 && note == 62;
        }
        if (! afterRelease)       problems.add ("latched pattern stopped when the keys lifted");
        if (! newKeyPlays)        problems.add ("the fresh key didn't play");
        if (chordAfterNewKey)     problems.add ("the old chord kept playing after a fresh key");
        return problems;
    }

    // Sequence mode: steps on/off, pitch offsets, accents, length, and chords.
    juce::StringArray checkSequencer()
    {
        auto run = [] (std::initializer_list<int> held)
        {
            Arpeggiator arp;
            arp.prepare (48000.0, 512);
            Arpeggiator::Settings s;
            s.on = true;
            s.mode = Arpeggiator::sequence;
            s.beatsPerStep = 0.25;
            s.bpm = 120.0;
            s.length = 4;
            s.stepOn = { true, true, false, true };
            s.stepPitch = { 0, 7, 0, 0 };
            s.stepAccent = { false, false, false, true };

            std::vector<std::tuple<int, int, int>> ons; // sample, note, velocity
            juce::MidiBuffer keys;
            for (int block = 0; block < 48000 / 512; ++block)
            {
                keys.clear();
                if (block == 0)
                    for (auto note : held)
                        keys.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                for (const auto m : arp.process (keys, 512, s))
                    if (m.getMessage().isNoteOn())
                        ons.push_back ({ block * 512 + m.samplePosition, m.getMessage().getNoteNumber(), m.getMessage().getVelocity() });
            }
            return ons;
        };

        juce::StringArray problems;

        // One key: C, G, rest, C (accented), twice in a second.
        const auto single = run ({ 60 });
        const int notes[] { 60, 67, 60, 60, 67, 60 };
        const int at[] { 0, 6000, 18000, 24000, 30000, 42000 };
        if (single.size() != std::size (notes))
            problems.add ("one key: " + juce::String ((int) single.size()) + " notes, expected 6");
        else
            for (size_t i = 0; i < single.size(); ++i)
            {
                const auto [sample, note, velocity] = single[i];
                if (note != notes[i] || std::abs (sample - at[i]) > 1)
                    problems.add ("one key: note " + juce::String ((int) i) + " was " + juce::String (note) + " at " + juce::String (sample));
                const auto accented = i == 2 || i == 5;
                if (accented != (velocity == 127))
                    problems.add ("one key: note " + juce::String ((int) i) + " accent wrong (velocity " + juce::String (velocity) + ")");
            }

        // Up mode shaped by the pattern: rests don't skip arp notes, pitch and accent apply.
        {
            Arpeggiator arp;
            arp.prepare (48000.0, 512);
            Arpeggiator::Settings s;
            s.on = true;
            s.mode = Arpeggiator::up;
            s.beatsPerStep = 0.25;
            s.bpm = 120.0;
            s.length = 4;
            s.stepOn = { true, false, true, true };
            s.stepPitch = { 0, 0, 12, 0 };
            s.stepAccent = { true, false, false, false };

            std::vector<std::tuple<int, int, int>> ons;
            juce::MidiBuffer keys;
            for (int block = 0; block < 48000 / 512; ++block)
            {
                keys.clear();
                if (block == 0)
                    for (auto note : { 60, 64, 67 })
                        keys.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                for (const auto m : arp.process (keys, 512, s))
                    if (m.getMessage().isNoteOn())
                        ons.push_back ({ block * 512 + m.samplePosition, m.getMessage().getNoteNumber(), m.getMessage().getVelocity() });
            }

            const int upNotes[] { 60, 76, 67, 60, 76, 67 };
            const int upAt[] { 0, 12000, 18000, 24000, 36000, 42000 };
            if (ons.size() != std::size (upNotes))
                problems.add ("up mode: " + juce::String ((int) ons.size()) + " notes, expected 6");
            else
                for (size_t i = 0; i < ons.size(); ++i)
                {
                    const auto [sample, note, velocity] = ons[i];
                    if (note != upNotes[i] || std::abs (sample - upAt[i]) > 1)
                        problems.add ("up mode: note " + juce::String ((int) i) + " was " + juce::String (note) + " at " + juce::String (sample));
                    if ((i % 3 == 0) != (velocity == 127))
                        problems.add ("up mode: note " + juce::String ((int) i) + " accent wrong");
                }
        }

        // A chord: every step that's on plays both keys.
        if (const auto chord = run ({ 60, 64 }); chord.size() != 12)
            problems.add ("chord: " + juce::String ((int) chord.size()) + " notes, expected 12");

        return problems;
    }

    // Sweeping a knob while a note holds must not click: the sharpest sample-to-
    // sample jump (second difference) stays close to the still render's.
    juce::StringArray checkKnobSweeps()
    {
        auto sharpest = [] (const char* id, bool sweep)
        {
            ZeroEngineProcessor engine;
            loadProfile (engine, genreNames.indexOf ("EDM"), typeNames.indexOf ("Pad"));
            engine.setPlayConfigDetails (0, 2, 48000.0, 256);
            engine.prepareToPlay (48000.0, 256);
            auto* param = engine.state.getParameter (id);

            juce::AudioBuffer<float> block (2, 256);
            juce::MidiBuffer midi;
            float worst = 0.0f, prev1 = 0.0f, prev2 = 0.0f;
            const auto blocks = 48000 * 2 / 256;
            for (int b = 0; b < blocks; ++b)
            {
                midi.clear();
                if (b == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                if (sweep && b > blocks / 4) // up and down twice, jumping each block like a fast hand
                    param->setValueNotifyingHost (0.5f + 0.5f * std::sin ((float) b * 0.08f));
                engine.processBlock (block, midi);
                if (b < blocks / 4)
                    continue; // skip the attack
                for (int i = 0; i < 256; ++i)
                {
                    const auto x = block.getSample (0, i);
                    worst = juce::jmax (worst, std::abs (x - 2.0f * prev1 + prev2));
                    prev2 = prev1;
                    prev1 = x;
                }
            }
            return worst;
        };

        juce::StringArray problems;
        // Not Grit: its drive already glides per sample, and more drive makes the
        // waveform itself sharper, which this measure can't tell from a click.
        for (const auto* id : { ParamIDs::air, ParamIDs::weight, ParamIDs::ottDepth, ParamIDs::punch, ParamIDs::width, ParamIDs::motion, ParamIDs::spectral,
                                ParamIDs::wavePos, ParamIDs::tableMix, ParamIDs::detune, ParamIDs::blend, ParamIDs::filterCutoff, ParamIDs::filterRes,
                                ParamIDs::tilt, ParamIDs::stereoWidth, ParamIDs::output, ParamIDs::delayMix, ParamIDs::reverbMix, ParamIDs::lfoToPos })
        {
            const auto still = sharpest (id, false), swept = sharpest (id, true);
            if (swept > still * 2.0f + 1.0e-3f)
                problems.add (juce::String (id) + " sweep clicks: sharpest jump " + juce::String (swept, 4) + " vs " + juce::String (still, 4) + " still");
        }
        return problems;
    }

    // Generate writes a new step pattern only when the arp is on, and the same
    // seed gives the same pattern.
    juce::StringArray checkGeneratePattern()
    {
        auto pattern = [] (bool arpOn, juce::int64 seed)
        {
            ZeroEngineProcessor engine;
            loadProfile (engine, genreNames.indexOf ("Techno"), typeNames.indexOf ("Pluck"));
            engine.state.getParameter (ParamIDs::arpOn)->setValueNotifyingHost (arpOn ? 1.0f : 0.0f);
            engine.generate (seed);
            juce::String steps;
            for (int s = 0; s < Arpeggiator::maxSteps; ++s)
                steps << (engine.state.getRawParameterValue (SeqIDs::on (s))->load() > 0.5f ? "x" : ".")
                      << (int) engine.state.getRawParameterValue (SeqIDs::pitch (s))->load() << " ";
            return steps;
        };

        juce::StringArray problems;
        juce::String defaults;
        for (int s = 0; s < Arpeggiator::maxSteps; ++s)
            defaults << "x0 ";
        if (pattern (false, 11) != defaults)
            problems.add ("Generate changed the pattern with the arp off");
        if (pattern (true, 11) == defaults)
            problems.add ("Generate left the pattern alone with the arp on");
        if (pattern (true, 11) != pattern (true, 11))
            problems.add ("same seed, different pattern");
        return problems;
    }

    // Keyboard lights: holding C4 with the arp on (Up, 2 octaves) lights the arp's
    // other notes too, and everything goes dark once the key is let go.
    juce::StringArray checkKeyLights()
    {
        juce::StringArray problems;
        ZeroEngineProcessor engine;
        loadProfile (engine, genreNames.indexOf ("Trance"), typeNames.indexOf ("Lead"));
        engine.state.getParameter (ParamIDs::arpOn)->setValueNotifyingHost (1.0f);
        auto* octaves = engine.state.getParameter (ParamIDs::arpOctaves);
        octaves->setValueNotifyingHost (octaves->convertTo0to1 (2.0f));
        engine.setPlayConfigDetails (0, 2, 48000.0, 512);
        engine.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        bool heldLit = false, otherLit = false;
        for (int i = 0; i < 400; ++i) // about 4.3 s: held for 2 s, then released
        {
            midi.clear();
            if (i == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            if (i == 188)
                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            engine.processBlock (block, midi);
            for (int note = 0; note < 128 && i < 188; ++note)
                if (engine.isNoteSounding (note))
                    (note == 60 ? heldLit : otherLit) = true;
        }
        if (! heldLit)
            problems.add ("the held key never lit");
        if (! otherLit)
            problems.add ("no arp note besides the held key lit");
        for (int note = 0; note < 128; ++note)
            if (engine.isNoteSounding (note))
                problems.add ("note " + juce::String (note) + " still lit after release");
        return problems;
    }

    // Reference Match: a sound the engine made itself (a profile reshaped by
    // `change`, saved as a WAV) must be matched much closer than the starting
    // sound is, and a wobble in it must come back at the same rate.
    juce::StringArray checkReferenceMatch (int genre, int type, int note, std::function<void (Profile&)> change, bool loop = false)
    {
        juce::StringArray problems;

        // loop: four notes a quarter second apart (up a minor third, a fifth, a fourth), as a pluck loop.
        auto renderTo = [&] (const Profile& profile, bool asLoop = false, int asGenre = -1, int asType = -1)
        {
            ZeroEngineProcessor engine;
            engine.stopProfileSync();
            engine.setPlayConfigDetails (0, 2, 48000.0, 512);
            engine.prepareToPlay (48000.0, 512);
            engine.loadProfile (asGenre < 0 ? genre : asGenre, asType < 0 ? type : asType, profile); // the match may pick another genre/type
            juce::AudioBuffer<float> settle (2, 512);
            juce::MidiBuffer none;
            for (int i = 0; i < 10; ++i) { settle.clear(); engine.processBlock (settle, none); } // as the matcher does
            juce::AudioBuffer<float> out (2, 72000), chunk (2, 512);
            juce::MidiBuffer midi;
            for (int pos = 0; pos + 512 <= out.getNumSamples(); pos += 512)
            {
                midi.clear();
                if (pos == 0 && ! asLoop)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 0);
                constexpr int steps[] { 0, 3, 7, 5 };
                for (int n = 0; asLoop && n < 4; ++n)
                {
                    const auto at = n * 12000, off = at + 9600;
                    if (at >= pos && at < pos + 512)
                        midi.addEvent (juce::MidiMessage::noteOn (1, note + steps[n], (juce::uint8) 110), at - pos);
                    if (off >= pos && off < pos + 512)
                        midi.addEvent (juce::MidiMessage::noteOff (1, note + steps[n]), off - pos);
                }
                chunk.clear();
                engine.processBlock (chunk, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, chunk, ch, 0, 512);
            }
            return out;
        };

        const auto start = makeProfile (genre, type);
        auto changed = start;
        change (changed);

        const auto reference = renderTo (changed, loop);
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_match_test.wav");
        file.deleteFile();
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), 48000.0, 2, 24, {}, 0));
            if (writer == nullptr)
                return { "could not write the test WAV" };
            writer->writeFromAudioSampleBuffer (reference, 0, reference.getNumSamples());
        }

        const auto target = SoundPrint::of (reference, 48000.0);
        const auto wanted = (float) juce::MidiMessage::getMidiNoteInHertz (note);
        if (std::abs (target.pitchHz / wanted - 1.0f) > 0.03f)
            problems.add ("pitch read as " + juce::String (target.pitchHz, 1) + " Hz, wanted " + juce::String (wanted, 1));
        ZeroEngineProcessor scorer;
        scorer.stopProfileSync();
        const auto before = ReferenceMatch::score (scorer, target, genre, type, start, false);

        MatchEngines engines;
        ReferenceMatch match (engines.get());
        const auto began = juce::Time::getMillisecondCounter();
        match.start (file, genre, type, start);
        const auto timeout = juce::Time::getMillisecondCounter() + 60000;
        while (match.getState() == ReferenceMatch::State::running && juce::Time::getMillisecondCounter() < timeout)
            juce::Thread::sleep (20);
        const auto result = match.take();
        file.deleteFile();

        if (! result.ok)
            return { "match failed: " + result.summary };
        // Scored as a match scores (the notes as heard, the best of a few gates).
        const auto after = ReferenceMatch::score (scorer, target, result.genre, result.type, result.profile, false);
        const auto truth = ReferenceMatch::score (scorer, target, genre, type, changed, false);
        std::printf ("(%s; distance %.2f -> %.2f, the true settings %.2f, in %.1f s) ", result.summary.toRawUTF8(), before, after, truth,
                     (juce::Time::getMillisecondCounter() - began) / 1000.0);
        if (after > 0.5f * before && after > truth + 1.0f) // halfway from the preset, or about as close as the truth itself
            problems.add ("match only got from " + juce::String (before, 2) + " to " + juce::String (after, 2) + " (reference: wobble "
                          + juce::String (target.moveRateHz, 2) + " Hz, " + juce::String ((int) target.notes.size()) + " notes heard)");
        return problems;
    }

    // Undo / redo: a knob change, and a Genre change whose new sound comes back as
    // it was (not reloaded from the designed profile).
    juce::StringArray checkUndo()
    {
        ZeroEngineProcessor a;
        loadProfile (a, genreNames.indexOf ("Trap"), typeNames.indexOf ("Lead"));
        auto value = [&] (const char* id) { return a.state.getRawParameterValue (id)->load(); };
        auto set = [&] (const char* id, float v) { auto* q = a.state.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };

        juce::StringArray problems;
        if (a.undo())
            problems.add ("undo did something with nothing changed");

        const auto weightBefore = value (ParamIDs::weight);
        set (ParamIDs::weight, 0.95f);
        if (! a.undo() || std::abs (value (ParamIDs::weight) - weightBefore) > 1.0e-4f)
            problems.add ("undo did not restore Weight");
        if (! a.redo() || std::abs (value (ParamIDs::weight) - 0.95f) > 1.0e-4f)
            problems.add ("redo did not bring Weight back");

        // Tweak, then change genre; undo must give back the tweaked Trap Lead.
        set (ParamIDs::grit, 0.123f);
        a.undo(); a.redo(); // records the tweak as a step
        set (ParamIDs::genre, (float) genreNames.indexOf ("Synthwave"));
        a.syncProfile();
        if (! a.undo())
            problems.add ("could not undo the genre change");
        else if ((int) value (ParamIDs::genre) != genreNames.indexOf ("Trap") || std::abs (value (ParamIDs::grit) - 0.123f) > 1.0e-3f)
            problems.add ("undoing the genre change did not bring back the tweaked sound");
        a.syncProfile(); // a timer tick afterwards must not reload the designed profile
        if (std::abs (value (ParamIDs::grit) - 0.123f) > 1.0e-3f)
            problems.add ("the profile was reloaded over the undone sound");
        return problems;
    }

    // MIDI learn: the next CC maps to the macro, then moves it, and the mapping
    // survives a project save and load.
    juce::StringArray checkMidiLearn()
    {
        ZeroEngineProcessor a;
        a.setPlayConfigDetails (0, 2, 48000.0, 512);
        a.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> block (2, 512);

        auto sendCC = [&] (int cc, int value)
        {
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
            a.processBlock (block, midi);
        };

        juce::StringArray problems;
        a.learnMacro (0); // Weight
        sendCC (74, 64);
        if (a.getMacroCC (0) != 74)
            problems.add ("Weight did not learn CC74");
        sendCC (74, 127);
        if (std::abs (a.state.getRawParameterValue (ParamIDs::weight)->load() - 1.0f) > 0.01f)
            problems.add ("CC74 did not move Weight");

        juce::MemoryBlock saved;
        a.getStateInformation (saved);
        ZeroEngineProcessor b;
        b.setStateInformation (saved.getData(), (int) saved.getSize());
        if (b.getMacroCC (0) != 74 || b.getMacroCC (1) != -1)
            problems.add ("mapping not restored with the project");
        return problems;
    }

    // A note let go mid-decay must finish its release, whatever the sustain
    // (a zero sustain used to hang the voice forever; a tiny one for minutes).
    juce::StringArray checkReleaseMidDecay()
    {
        juce::StringArray problems;
        for (const auto sustain : { 0.0f, 0.004f, 0.3f })
        {
            ZeroEngineProcessor engine;
            loadProfile (engine, genreNames.indexOf ("EDM"), typeNames.indexOf ("Pluck"));
            auto set = [&] (const char* id, float v) { auto* q = engine.state.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
            set (ParamIDs::ampDecay, 2.0f);
            set (ParamIDs::ampSustain, sustain);
            set (ParamIDs::ampRelease, 0.3f);
            engine.setPlayConfigDetails (0, 2, 48000.0, 512);
            engine.prepareToPlay (48000.0, 512);

            juce::AudioBuffer<float> block (2, 512);
            juce::MidiBuffer midi;
            for (int b = 0; b < 48000 * 2 / 512; ++b) // let go at 0.1 s, check at 2 s
            {
                midi.clear();
                if (b == 0)  midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 110), 0);
                if (b == 10) midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                engine.processBlock (block, midi);
            }
            if (engine.getActiveVoiceCount() > 0)
                problems.add ("sustain " + juce::String (sustain, 3) + ": still sounding 1.9 s after a 0.3 s release");
        }
        return problems;
    }

    // The mod wheel must change the sound (it adds to Motion), and not allocate.
    juce::StringArray checkModWheel()
    {
        auto render = [] (int wheel)
        {
            ZeroEngineProcessor engine;
            loadProfile (engine, genreNames.indexOf ("Dubstep"), typeNames.indexOf ("Bass"));
            engine.setPlayConfigDetails (0, 2, 48000.0, 512);
            engine.prepareToPlay (48000.0, 512);

            juce::AudioBuffer<float> block (2, 512), out (1, 512 * 60);
            juce::MidiBuffer midi;
            for (int b = 0; b < 60; ++b)
            {
                midi.clear();
                if (b == 0)
                {
                    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, wheel), 0);
                    midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110), 0);
                }
                block.clear();
                engine.processBlock (block, midi);
                out.copyFrom (0, b * 512, block, 0, 0, 512);
            }
            return out;
        };

        const auto still = render (0), moving = render (127);
        double difference = 0.0, energy = 0.0;
        for (int i = 0; i < still.getNumSamples(); ++i)
        {
            const auto d = still.getSample (0, i) - moving.getSample (0, i);
            difference += d * d;
            energy += still.getSample (0, i) * still.getSample (0, i);
        }

        juce::StringArray problems;
        if (difference < energy * 0.01)
            problems.add ("full mod wheel barely changed the sound");
        return problems;
    }

    juce::StringArray checkState()
    {
        ZeroEngineProcessor a;
        loadProfile (a, genreNames.indexOf ("Dubstep"), typeNames.indexOf ("Bass"));

        // Tweak a few values away from the profile.
        auto setValue = [&] (const char* id, float value)
        {
            auto* param = a.state.getParameter (id);
            param->setValueNotifyingHost (param->convertTo0to1 (value));
        };
        setValue (ParamIDs::grit, 0.123f);
        setValue (ParamIDs::filterCutoff, 1234.0f);
        setValue (ParamIDs::reverbMix, 0.42f);

        juce::MemoryBlock saved;
        a.getStateInformation (saved);

        ZeroEngineProcessor b;
        b.setStateInformation (saved.getData(), (int) saved.getSize());
        b.syncProfile(); // a timer tick after load must not reapply the profile

        juce::StringArray problems;
        for (auto* param : a.getParameters())
        {
            auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param);
            if (withId == nullptr)
                continue;
            const auto expected = param->getValue();
            const auto actual = b.state.getParameter (withId->getParameterID())->getValue();
            if (std::abs (expected - actual) > 1.0e-5f)
                problems.add (withId->getParameterID() + ": saved " + juce::String (expected, 4) + ", loaded " + juce::String (actual, 4));
        }
        return problems;
    }
}

int runTests()
{
    int failures = 0;

    std::printf ("Favourites save/recall... ");
    if (const auto problems = checkFavourites(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Spectral stage (clean at 0, sane at full)... ");
    if (const auto problems = checkSpectralStage(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    {
        std::printf ("All profiles with Spectral at full... ");
        juce::StringArray report;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
                for (const auto& problem : checkRender (g, t, 48000.0, 512, 0, 1.0f))
                    report.add (genreNames[g] + " / " + typeNames[t] + ": " + problem);
        if (report.isEmpty())
            std::printf ("ok\n");
        else
        {
            std::printf ("%d problems\n", report.size());
            for (const auto& line : report)
                std::printf ("    %s\n", line.toRawUTF8());
            failures += report.size();
        }
    }

    std::printf ("Arpeggiator (order, timing, stop)... ");
    if (const auto problems = checkArpeggiator(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Keyboard lights follow the arp... ");
    if (const auto problems = checkKeyLights(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Reference Match, a plucked lead... ");
    if (const auto problems = checkReferenceMatch (genreNames.indexOf ("Synthwave"), typeNames.indexOf ("Lead"), 64, [] (Profile& p)
        {
            p.waveTable = p.waveTable < 1.0f ? 3.0f : 0.0f;
            p.tilt -= 4.0f;
            p.stereoWidth = 0.2f;
            p.ottDepth = juce::jmin (1.0f, p.ottDepth + 0.4f);
            p.ampDecay = 0.15f; // plucked: short decay, low sustain
            p.ampSustain = 0.15f;
        }); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Reference Match, a 5 Hz filter wobble... ");
    if (const auto problems = checkReferenceMatch (genreNames.indexOf ("Techno"), typeNames.indexOf ("Lead"), 57, [] (Profile& p)
        {
            p.lfoSync = 0.0f;
            p.lfoRate = 5.0f;
            p.lfoShape = 0.0f;
            p.lfoToCutoff = 3.0f;
            p.lfoToPitch = 0.0f; // a filter wobble only: the lead's vibrato would bend the pitch at 5 Hz
            p.modToPitch = 0.0f;
            p.filterCutoff = 600.0f;
            p.ampSustain = 1.0f;
        }); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Reference Match, a dark filtered pad... ");
    if (const auto problems = checkReferenceMatch (genreNames.indexOf ("Synthwave"), typeNames.indexOf ("Pad"), 57, [] (Profile& p)
        {
            p.filterCutoff = 450.0f;
            p.air = 0.0f;
            p.tilt = -4.0f;
        }); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Reference Match, a pluck loop (first note heard)... ");
    if (const auto problems = checkReferenceMatch (genreNames.indexOf ("Techno"), typeNames.indexOf ("Pluck"), 60, [] (Profile& p)
        {
            p.ampDecay = 0.12f;
            p.ampSustain = 0.0f;
            p.tilt -= 2.0f;
            p.waveTable = p.waveTable < 1.0f ? 2.0f : 0.0f;
        }, true); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Arp latch... ");
    if (const auto problems = checkLatch(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Step sequencer (steps, pitch, accent, chords)... ");
    if (const auto problems = checkSequencer(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Knob sweeps don't click (macros, OTT, Spectral)... ");
    if (const auto problems = checkKnobSweeps(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Generate writes a pattern with the arp on... ");
    if (const auto problems = checkGeneratePattern(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Undo / redo (knob, genre change)... ");
    if (const auto problems = checkUndo(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("MIDI learn (map, move, save)... ");
    if (const auto problems = checkMidiLearn(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Arp running through the engine... ");
    {
        juce::StringArray report;
        for (const auto& [g, t] : { std::pair { "EDM", "Pluck" }, { "Trance", "Lead" }, { "Dubstep", "Bass" } })
            for (const auto& problem : checkRender (genreNames.indexOf (g), typeNames.indexOf (t), 48000.0, 512, 0, -1.0f, true))
                report.add (juce::String (g) + " / " + t + ": " + problem);
        std::printf (report.isEmpty() ? "ok\n" : "FAILED\n");
        for (const auto& line : report)
            std::printf ("    %s\n", line.toRawUTF8());
        failures += report.size();
    }

    std::printf ("Release mid-decay... ");
    if (const auto problems = checkReleaseMidDecay(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Mod wheel drives Motion... ");
    if (const auto problems = checkModWheel(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("Generate history... ");
    if (const auto problems = checkHistory(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    std::printf ("State save/load... ");
    if (const auto problems = checkState(); problems.isEmpty())
        std::printf ("ok\n");
    else
    {
        std::printf ("FAILED\n");
        for (const auto& p : problems)
            std::printf ("    %s\n", p.toRawUTF8());
        ++failures;
    }

    const std::pair<double, int> configs[] { { 44100.0, 64 }, { 48000.0, 512 }, { 96000.0, 2048 }, { 44100.0, 333 } };
    for (const auto& [rate, blockSize] : configs)
    {
        std::printf ("All profiles at %.1f kHz, %d-sample blocks... ", rate / 1000.0, blockSize);
        juce::StringArray report;

        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
                for (const auto& problem : checkRender (g, t, rate, blockSize))
                    report.add (genreNames[g] + " / " + typeNames[t] + ": " + problem);

        if (report.isEmpty())
            std::printf ("ok\n");
        else
        {
            std::printf ("%d problems\n", report.size());
            for (const auto& line : report)
                std::printf ("    %s\n", line.toRawUTF8());
            failures += report.size();
        }
    }

    // Generate: random variations must meet the same bar as the designed profiles.
    {
        constexpr int seedsPerProfile = 4;
        std::printf ("Generated sounds, %d per profile... ", seedsPerProfile);
        juce::StringArray report;

        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
                for (int s = 1; s <= seedsPerProfile; ++s)
                {
                    const auto seed = (juce::int64) (1000 * g + 100 * t + s);
                    for (const auto& problem : checkRender (g, t, 48000.0, 512, seed))
                        report.add (genreNames[g] + " / " + typeNames[t] + " seed " + juce::String (seed) + ": " + problem);
                }

        if (report.isEmpty())
            std::printf ("ok\n");
        else
        {
            std::printf ("%d problems\n", report.size());
            for (const auto& line : report)
                std::printf ("    %s\n", line.toRawUTF8());
            failures += report.size();
        }
    }

    std::printf (failures == 0 ? "All tests passed.\n" : "%d failures.\n", failures);
    return failures == 0 ? 0 : 1;
}
