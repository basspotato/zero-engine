#include "ReferenceMatch.h"
#include "PluginProcessor.h"
#include "Parameters.h"
#include "Wavetable.h"
#include "Cmaes.h"


namespace
{
    constexpr double rate = 48000.0;
    constexpr int block = 512;

    // The notes a candidate plays (the reference's, transcribed), in samples from
    // the start, shifted together by octave checks; and how much is recorded.
    struct Playing
    {
        // span: from the note's start to the next note's (or, for the last, to the
        // end of the sound); the note is held for gate x span, the gate being
        // searched like a setting, since how long a note was held can't be heard
        // apart from its release.
        struct Event { int start = 0, span = 0, note = 60; };
        std::vector<Event> events;
        int lengthSamples = 0, shift = 0;
        float gate = 0.75f;
        bool chord = false; // some notes come several at once: played poly, whatever the preset says
        int firstNote() const { return (events.empty() ? 60 : events.front().note) + shift; }
        int hold (const Event& e) const { return juce::jmax (block, (int) (gate * (float) e.span)); }
    };

    SoundPrint render (ZeroEngineProcessor& engine, int genre, int type, const Profile& profile, const Playing& playing)
    {
        engine.resetForRender(); // nothing of the last candidate may ring into this one
        auto played = profile;
        if (playing.chord)
            played.playMode = 0.0f;
        engine.loadProfile (genre, type, played);

        // A tenth of a second of silence first, so knob smoothing settles.
        juce::AudioBuffer<float> out (2, playing.lengthSamples), chunk (2, block);
        juce::MidiBuffer midi;
        for (int pos = 0; pos < 5120; pos += block)
        {
            chunk.clear();
            engine.processBlock (chunk, midi);
        }

        // The reference's notes, one at a time as it has them (a chord type
        // sounds its tone on single notes too).
        for (int pos = 0; pos < playing.lengthSamples; pos += block)
        {
            midi.clear();
            for (const auto& e : playing.events)
            {
                const auto note = juce::jlimit (0, 127, e.note + playing.shift);
                if (e.start >= pos && e.start < pos + block)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), e.start - pos);
                if (e.start + playing.hold (e) >= pos && e.start + playing.hold (e) < pos + block)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note), e.start + playing.hold (e) - pos);
            }
            chunk.clear();
            engine.processBlock (chunk, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, chunk, ch, 0, juce::jmin (block, playing.lengthSamples - pos));
        }
        return SoundPrint::of (out, rate, false);
    }


    // The reference's notes as candidates play them (one note, when no pitch
    // can be followed), recorded 0.3 s past its end so a tail that
    // rings on counts (or exactly as far as measured, when it goes on past that).
    Playing playingFor (const SoundPrint& target)
    {
        Playing playing;
        auto toNote = [] (float hz, int fallback)
        {
            return hz > 0.0f ? juce::jlimit (24, 96, juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0))) : fallback;
        };
        auto previous = toNote (target.pitchHz, 60);
        const auto seconds = [] (double s) { return (int) (s * rate); };
        // A sound still going at the window's end is held through it.
        const auto lastSpan = [&] (float start) { return target.cut ? seconds (10.0) : seconds (target.noteSeconds - start); };
        std::vector<float> lengths; // the typical note spacing of a phrase
        for (size_t i = 0; i + 1 < target.notes.size(); ++i)
            lengths.push_back (target.notes[i].length);
        std::sort (lengths.begin(), lengths.end());
        const auto spacing = lengths.empty() ? 0.0f : lengths[lengths.size() / 2];
        if (target.notes.empty()) // no pitch to follow: one note
            playing.events.push_back ({ 0, lastSpan (0.0f), previous });
        for (size_t i = 0; i < target.notes.size(); ++i)
        {
            const auto& n = target.notes[i];
            previous = toNote (n.pitchHz, previous);
            const auto span = i + 1 < target.notes.size() ? seconds (n.length)
                            : target.notes.size() > 1 ? seconds (spacing) // a phrase's last note: held like the others
                                                                    : lastSpan (n.start);
            playing.events.push_back ({ seconds (n.start), span, previous });
            playing.chord = playing.chord || ! n.also.empty();
            for (const auto hz : n.also) // the rest of a chord
                playing.events.push_back ({ seconds (n.start), span, toNote (hz, previous) });
        }
        playing.lengthSamples = juce::jmax (4 * block, seconds (target.cut ? target.noteSeconds : target.noteSeconds + 0.3));
        return playing;
    }
    // The settings the search moves, each mapped from 0..1 (log for times and frequency).
    struct Dimension { float Profile::* member; float min, max; bool log; };
    const Dimension dimensions[] {
        { &Profile::wavePos, 0.0f, 1.0f, false },        { &Profile::tilt, -6.0f, 6.0f, false },
        { &Profile::air, 0.0f, 1.0f, false },            { &Profile::ottDepth, 0.0f, 1.0f, false },
        { &Profile::grit, 0.0f, 1.0f, false },           { &Profile::stereoWidth, 0.0f, 2.0f, false },
        { &Profile::weight, 0.0f, 1.0f, false },         { &Profile::detune, 0.0f, 1.0f, false },
        { &Profile::ampAttack, 0.001f, 2.0f, true },     { &Profile::ampDecay, 0.02f, 4.0f, true },
        { &Profile::ampSustain, 0.0f, 1.0f, false },     { &Profile::ampRelease, 0.01f, 3.0f, true },
        { &Profile::filterCutoff, 60.0f, 20000.0f, true }, { &Profile::filterRes, 0.0f, 0.9f, false },
        { &Profile::filterEnv, -2.0f, 6.0f, false },     { &Profile::modDecay, 0.01f, 2.0f, true },
        { &Profile::reverbMix, 0.0f, 1.0f, false },      { &Profile::delayMix, 0.0f, 1.0f, false },
        { &Profile::lfoRate, 0.1f, 20.0f, true },        { &Profile::lfoToCutoff, 0.0f, 6.0f, false },
        { &Profile::lfoToPos, 0.0f, 1.0f, false },       { &Profile::delayFeedback, 0.0f, 0.9f, false },
    };

    double toUnit (const Dimension& d, float value)
    {
        value = juce::jlimit (d.min, d.max, value);
        return d.log ? std::log (value / d.min) / std::log (d.max / d.min) : (value - d.min) / (d.max - d.min);
    }

    float fromUnit (const Dimension& d, double unit)
    {
        return (float) (d.log ? d.min * std::pow (d.max / d.min, unit) : d.min + unit * (d.max - d.min));
    }

    Profile withUnits (Profile p, const std::vector<double>& units)
    {
        for (size_t i = 0; i < std::size (dimensions); ++i)
            p.*dimensions[i].member = fromUnit (dimensions[i], units[i]);
        return p;
    }

    struct Candidate { int genre = 0, type = 0; Profile profile {}; float distance = 1.0e9f; SoundPrint print; Playing playing {}; };
}

ReferenceMatch::ReferenceMatch (std::vector<ZeroEngineProcessor*> renderEngines)
    : juce::Thread ("Reference Match"), engines (std::move (renderEngines))
{
}

ReferenceMatch::~ReferenceMatch()
{
    stopThread (10000);
}

void ReferenceMatch::start (const juce::File& reference, int newGenre, int newType, const Profile& startFrom)
{
    stopThread (10000);
    file = reference;
    genre = newGenre;
    type = newType;
    from = startFrom;
    result = {};
    progress = 0.0f;
    state = State::running;
    startThread();
}

ReferenceMatch::Result ReferenceMatch::take()
{
    state = State::idle;
    return result;
}

void ReferenceMatch::run()
{
    auto fail = [this] (const juce::String& why)
    {
        result.summary = why;
        state = State::failed;
    };

    // The reference: up to 10 s, any common format.
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return fail ("Can't read " + file.getFileName());

    const auto length = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));
    juce::AudioBuffer<float> audio (2, juce::jmax (1, length));
    reader->read (&audio, 0, length, 0, true, true);
    const auto target = SoundPrint::of (audio, reader->sampleRate, true);
    result.target = target;
    if (target.levelDb < -70.0f || target.melTone.empty())
        return fail (file.getFileName() + " is silent");

    const auto playing = playingFor (target);

    const auto workers = (int) engines.size();
    for (auto* engine : engines) // once: preparing allocates (delay lines, oversampling); renders only reset
    {
        engine->setPlayConfigDetails (0, 2, rate, block);
        engine->prepareToPlay (rate, block);
        engine->setLightRender (lightSearch);
        if (target.harmonics.has_value()) // the Sample wavetable is the reference's own
            engine->setSampleHarmonics (*target.harmonics);
    }
    const auto searchRenders = searchBudget; // per worker
    const auto numPresets = genreNames.size() * typeNames.size();
    const auto expected = (float) (numPresets + workers * (WavetableBank::numBanks + searchRenders + secondBudget));
    std::atomic<int> renders { 0 };
    auto counted = [&] { progress = juce::jmin (0.99f, (float) ++renders / expected); };

    // Runs job (worker index) on every engine at once, on low-priority threads
    // so a match never competes with the host's audio thread.
    juce::ThreadPool pool (juce::ThreadPoolOptions{}.withThreadName ("Reference Match worker")
                                                    .withNumberOfThreads (workers)
                                                    .withDesiredThreadPriority (juce::Thread::Priority::low));
    auto inParallel = [&] (auto job)
    {
        for (int w = 0; w < workers; ++w)
            pool.addJob ([&, w] { job (w); });
        while (pool.getNumJobs() > 0)
            juce::Thread::sleep (5);
    };

    // 1. Every preset at the reference's pitch.
    const auto fxType = typeNames.indexOf ("FX");
    const auto pitched = ! target.notes.empty() && target.notes.front().pitchHz > 0.0f;
    std::vector<Candidate> presets ((size_t) numPresets);
    std::atomic<int> next { 0 };
    inParallel ([&] (int w)
    {
        for (int i = next++; i < numPresets && ! threadShouldExit(); i = next++)
        {
            auto& c = presets[(size_t) i];
            c.genre = i / typeNames.size();
            c.type = i % typeNames.size();
            c.profile = c.genre == genre && c.type == type ? from : makeProfile (c.genre, c.type);
            if (c.type == fxType && pitched) // FX patches (risers, impacts) only for sounds without a clear pitch:
            {                                  // they otherwise pass for close-ish to anything
                counted();
                continue;
            }
            c.print = render (*engines[(size_t) w], c.genre, c.type, c.profile, playing);
            c.distance = SoundPrint::distance (c.print, target);
            if (target.moveRateHz > 0.0f) // a wobbling reference: each preset with the wobble too
            {
                auto p = c.profile;
                p.lfoSync = 0.0f;
                p.lfoRate = target.moveRateHz;
                p.lfoToCutoff = juce::jmax (p.lfoToCutoff, 2.0f);
                p.filterCutoff = juce::jmin (p.filterCutoff, 800.0f); // a wide-open filter has nothing to wobble
                auto print = render (*engines[(size_t) w], c.genre, c.type, p, playing);
                if (const auto d = SoundPrint::distance (print, target); d < c.distance)
                {
                    c.profile = p;
                    c.print = std::move (print);
                    c.distance = d;
                }
            }
            counted();
        }
    });
    if (threadShouldExit())
        return fail ("Match cancelled");

    // The closest few, at most two of any one type, so the starts differ.
    std::sort (presets.begin(), presets.end(), [] (const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
    if (debugPrint) // tooling: the presets as ranked
        for (size_t i = 0; i < 12; ++i)
            std::printf ("    preset %-12s %-6s %.2f\n", genreNames[presets[i].genre].toRawUTF8(), typeNames[presets[i].type].toRawUTF8(), presets[i].distance);

    std::vector<Candidate> starts;
    for (const auto& c : presets)
    {
        const auto sameType = std::count_if (starts.begin(), starts.end(), [&] (const Candidate& s) { return s.type == c.type; });
        if (sameType < 2)
            starts.push_back (c);
        if ((int) starts.size() == workers)
            break;
    }

    // Refining one start on one engine. The first round settles the octave and
    // the wavetable, seeds any wobble, then searches; a second round starts
    // from a good result nudged at random (seed) and searches with finer steps.
    // Returns its best few, re-scored at full quality at the end.
    auto refine = [&] (int w, Candidate start, bool firstRound, unsigned seed)
    {
        auto& engine = *engines[(size_t) w];
        auto best = start;
        auto mine = firstRound ? playing : start.playing; // the notes this worker plays, octave settled below
        best.playing = mine;
        std::vector<Candidate> top;
        auto consider = [&] (const Profile& p, double gateUnit = -1.0)
        {
            auto play = mine;
            if (gateUnit >= 0.0)
                play.gate = (float) (0.05 + 0.95 * gateUnit); // held from 5 % of the note to all of it
            Candidate c { best.genre, best.type, p, 0.0f, render (engine, best.genre, best.type, p, play), play };
            c.distance = SoundPrint::distance (c.print, target);
            counted();
            const auto d = c.distance;
            if (d < best.distance)
                best = c;
            if (top.size() < 3 || d < top.back().distance)
            {
                top.push_back (std::move (c));
                std::sort (top.begin(), top.end(), [] (const Candidate& x, const Candidate& y) { return x.distance < y.distance; });
                if (top.size() > 3)
                    top.pop_back();
            }
            return (double) d;
        };

        if (firstRound)
        {
            // The octave: a pitch read an octave high (a strong second harmonic)
            // is tried an octave down, when the spectrum has real energy there.
            // Never up: a correct reading always loses to its octave neighbours somewhere.
            const auto below = (int) std::floor (std::log2 (juce::MidiMessage::getMidiNoteInHertz (mine.firstNote() - 12) / 31.25) + 0.5);
            const auto strongest = *std::max_element (target.bandsDb.begin(), target.bandsDb.end());
            if (below >= 0 && below < SoundPrint::numBands && target.bandsDb[(size_t) below] > strongest - 12.0f)
            {
                auto other = mine;
                other.shift = mine.shift - 12;
                auto print = render (engine, best.genre, best.type, best.profile, other);
                if (const auto d = SoundPrint::distance (print, target); d < best.distance)
                {
                    best.distance = d;
                    best.print = std::move (print);
                    mine = other;
                    best.playing = mine;
                }
                counted();
            }

            for (int table = 0; table < WavetableBank::numBanks && ! threadShouldExit(); ++table)
            {
                auto p = best.profile;
                p.waveTable = (float) table;
                consider (p);
            }

            // The delay time, a note division the search cannot slide.
            for (int division = 0; division < delayTimeNames.size(); ++division)
            {
                auto p = best.profile;
                if ((int) p.delayTime == division || p.delayMix <= 0.0f)
                    continue;
                p.delayTime = (float) division;
                if (! threadShouldExit())
                    consider (p);
            }

            // The sub layer, which the preset decides: off, or on an octave down or at the note.
            for (const auto [on, octave] : { std::pair { 0.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 0.0f } })
            {
                auto p = best.profile;
                if (p.subOn == on && (on < 0.5f || p.subOctave == octave))
                    continue;
                p.subOn = on;
                p.subOctave = octave;
                if (! threadShouldExit())
                    consider (p);
            }

            // A wobble in the reference: a free-running LFO at its rate, on the
            // filter or the wavetable position, tried before the search refines it.
            if (target.moveRateHz > 0.0f)
                for (const auto [toCutoff, toPos] : { std::pair { 1.5f, 0.0f }, { 3.0f, 0.0f }, { 0.0f, 0.5f } })
                {
                    auto p = best.profile;
                    p.lfoSync = 0.0f;
                    p.lfoRate = target.moveRateHz;
                    p.lfoToCutoff = toCutoff;
                    p.lfoToPos = toPos;
                    if (toCutoff > 0.0f)
                        p.filterCutoff = juce::jmin (p.filterCutoff, 800.0f);
                    if (! threadShouldExit())
                        consider (p, (best.playing.gate - 0.05) / 0.95);
                }
        }
        const auto beforeSearch = best.distance;

        auto base = best.profile;
        if (target.moveRateHz > 0.0f)
            base.lfoSync = 0.0f; // free-running, so the rate dimension counts
        std::vector<double> units;
        for (const auto& d : dimensions)
            units.push_back (toUnit (d, base.*d.member));
        units.push_back ((double) (best.playing.gate - 0.05f) / 0.95); // and how long each note is held

        const auto budget = firstRound ? searchRenders : secondBudget;
        auto stepSize = 0.15;
        if (! firstRound)
        {
            std::mt19937 rng (seed);
            std::normal_distribution<double> nudge (0.0, 0.05);
            for (auto& u : units)
                u = std::clamp (u + nudge (rng), 0.0, 1.0);
            stepSize = 0.08;
        }

        if (useCmaes)
        {
            // CMA-ES with restarts: when a run stops gaining for five
            // generations, start again around the best with twice the population.
            auto lambda = 0;
            for (int done = 0; done < budget && ! threadShouldExit();)
            {
                Cmaes cmaes (units, stepSize, seed + (unsigned) done, lambda);
                lambda = cmaes.populationSize() * 2;
                auto stale = 0;
                for (auto before = best.distance; done + cmaes.populationSize() <= budget && stale < 5 && ! threadShouldExit();)
                {
                    const auto population = cmaes.ask();
                    std::vector<double> costs;
                    for (const auto& x : population)
                        costs.push_back (consider (withUnits (base, x), x.back()));
                    cmaes.tell (costs);
                    done += (int) population.size();
                    stale = best.distance < before - 1.0e-4f ? 0 : stale + 1;
                    before = juce::jmin (before, best.distance);
                }
                if (done + cmaes.populationSize() > budget)
                    break;
            }
        }
        else
        {
            // Pattern search: each setting a step either way, keeping any gain;
            // when a full pass gains nothing, the steps halve.
            auto cost = firstRound ? (double) best.distance : consider (withUnits (base, units), units.back());
            std::vector<double> steps (units.size(), stepSize);
            for (int done = 0; done < budget && ! threadShouldExit();)
            {
                bool improved = false;
                for (size_t k = 0; k < units.size() && done < budget && ! threadShouldExit(); ++k)
                    for (const auto direction : { 1.0, -1.0 })
                    {
                        auto candidate = units;
                        candidate[k] = std::clamp (units[k] + direction * steps[k], 0.0, 1.0);
                        if (candidate[k] == units[k])
                            continue;
                        ++done;
                        if (const auto c = consider (withUnits (base, candidate), candidate.back()); c < cost)
                        {
                            units = candidate;
                            cost = c;
                            improved = true;
                            break;
                        }
                    }
                if (! improved)
                {
                    for (auto& s : steps)
                        s *= 0.5;
                    if (steps[0] < 0.005)
                        break;
                }
            }
        }

        if (debugPrint)
            std::printf ("    %s worker %d %s %s: start %.2f, before search %.2f, after %.2f\n", firstRound ? "first " : "second", w,
                         genreNames[best.genre].toRawUTF8(), typeNames[best.type].toRawUTF8(), start.distance, beforeSearch, best.distance);
        if (top.empty())
            top.push_back (best);
        return top;
    };

    // 2. Each start refined on its own engine.
    std::vector<std::vector<Candidate>> finals ((size_t) workers); // each worker's best few
    inParallel ([&] (int w)
    {
        if (w < (int) starts.size())
            finals[(size_t) w] = refine (w, starts[(size_t) w], true, (unsigned) w + 1);
    });
    if (threadShouldExit())
        return fail ("Match cancelled");

    // 2b. Successive halving: every engine goes on from one of the two best
    //     results so far, each nudged differently, instead of the weaker starts.
    std::vector<Candidate> leaders;
    for (const auto& f : finals)
        if (! f.empty())
            leaders.push_back (f.front());
    std::sort (leaders.begin(), leaders.end(), [] (const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
    leaders.resize (juce::jmin ((size_t) 2, leaders.size()));
    if (! leaders.empty())
    {
        std::vector<std::vector<Candidate>> more ((size_t) workers);
        inParallel ([&] (int w)
        {
            more[(size_t) w] = refine (w, leaders[(size_t) w % leaders.size()], false, (unsigned) (101 + w));
        });
        for (int w = 0; w < workers; ++w)
            for (auto& c : more[(size_t) w])
                finals[(size_t) w].push_back (std::move (c));
    }
    if (threadShouldExit())
        return fail ("Match cancelled");

    // 3. The finalists again at full quality (spectral stage, oversampling), the
    //    sound the plugin will actually make; the best of those wins.
    inParallel ([&] (int w)
    {
        engines[(size_t) w]->setLightRender (false);
        for (auto& c : finals[(size_t) w])
        {
            c.print = render (*engines[(size_t) w], c.genre, c.type, c.profile, c.playing);
            c.distance = SoundPrint::distance (c.print, target);

            // The spectral stage was off while searching, so its amount was never
            // judged: try the finalist without it too, and keep whichever is closer.
            if (c.profile.spectral > 0.0f)
            {
                auto dry = c.profile;
                dry.spectral = 0.0f;
                auto print = render (*engines[(size_t) w], c.genre, c.type, dry, c.playing);
                if (const auto d = SoundPrint::distance (print, target); d < c.distance)
                {
                    c.profile = dry;
                    c.print = std::move (print);
                    c.distance = d;
                }
            }
        }
    });
    if (debugPrint) // tooling: the finalists at full quality, term by term
        for (const auto& f : finals)
            for (const auto& c : f)
            {
                juce::String terms;
                SoundPrint::distance (c.print, target, &terms);
                std::printf ("    final  %-12s %-6s %.2f  gate %.2f  %s\n", genreNames[c.genre].toRawUTF8(), typeNames[c.type].toRawUTF8(), c.distance,
                             c.playing.gate, terms.toRawUTF8());
            }
    std::vector<Candidate> all;
    for (auto& f : finals)
        for (auto& c : f)
            all.push_back (std::move (c));
    if (all.empty())
        return fail ("Match found nothing");
    const auto& winner = *std::min_element (all.begin(), all.end(), [] (const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
    result.ok = true;
    result.profile = winner.profile;
    if (playing.chord)
        result.profile.playMode = 0.0f;
    result.genre = winner.genre;
    result.type = winner.type;
    result.matched = winner.print;
    result.distance = winner.distance;
    // How close, in words: a percentage of a loss means little to a listener.
    // Calibrated on matchbench (distance against the judge's dB).
    const auto closeness = winner.distance < closeBelow ? "VERY CLOSE" : winner.distance < closeBelow * 1.75f ? "CLOSE"
                         : winner.distance < closeBelow * 2.5f ? "ROUGH" : "LOOSE";
    result.summary = "MATCHED " + file.getFileNameWithoutExtension().toUpperCase() + "  -  " + closeness + "  -  "
                   + genreNames[winner.genre].toUpperCase() + " " + typeNames[winner.type].toUpperCase();
    progress = 1.0f;
    state = State::done;
}

//==============================================================================
MatchEngines::MatchEngines()
{
    for (int i = 0; i < juce::jlimit (1, 6, juce::SystemStats::getNumCpus() - 2); ++i)
    {
        owned.push_back (std::make_unique<ZeroEngineProcessor>());
        owned.back()->stopProfileSync();
    }
}

MatchEngines::~MatchEngines() = default;

std::vector<ZeroEngineProcessor*> MatchEngines::get() const
{
    std::vector<ZeroEngineProcessor*> engines;
    for (const auto& e : owned)
        engines.push_back (e.get());
    return engines;
}

float ReferenceMatch::score (ZeroEngineProcessor& engine, const SoundPrint& target, int genre, int type, const Profile& profile, bool light)
{
    engine.setPlayConfigDetails (0, 2, rate, block);
    engine.prepareToPlay (rate, block);
    engine.setLightRender (light);
    if (target.harmonics.has_value())
        engine.setSampleHarmonics (*target.harmonics);
    // The hold isn't known from the audio, so the best of a few gates.
    auto d = 1.0e9f;
    for (const auto gate : { 0.3f, 0.5f, 0.75f, 0.9f, 1.0f })
    {
        auto playing = playingFor (target);
        playing.gate = gate;
        d = juce::jmin (d, SoundPrint::distance (render (engine, genre, type, profile, playing), target));
    }
    engine.setLightRender (false);
    return d;
}
