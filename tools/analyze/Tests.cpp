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
    juce::StringArray checkRender (int genre, int type, double rate, int blockSize, juce::int64 generateSeed = 0, float spectral = -1.0f)
    {
        ZeroEngineProcessor engine;
        loadProfile (engine, genre, type);
        if (generateSeed != 0)
            engine.generate (generateSeed); // a Generate-button variation instead of the designed profile
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
