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
    juce::StringArray checkRender (int genre, int type, double rate, int blockSize)
    {
        ZeroEngineProcessor engine;
        loadProfile (engine, genre, type);
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

    std::printf (failures == 0 ? "All tests passed.\n" : "%d failures.\n", failures);
    return failures == 0 ? 0 : 1;
}
