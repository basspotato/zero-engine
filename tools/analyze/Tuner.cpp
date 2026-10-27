#include "Tuner.h"
#include "Library.h"
#include "../../Source/Profiles.h"
#include "../../Source/Parameters.h"
#include <map>

namespace
{
    // What the tuner matches: the mean of a group of references.
    struct Target
    {
        double lufs, plr, centroidHz, bassPct, sidePct;
        int note = -1; // render at this MIDI note (-1: the type's default)
    };

    Target targetFrom (const Descriptors& d)
    {
        return { d.lufs, d.plr, d.centroidHz, d.bassPct, d.sidePct };
    }

    double median (std::vector<double> v)
    {
        std::nth_element (v.begin(), v.begin() + (std::ptrdiff_t) (v.size() / 2), v.end());
        return v[v.size() / 2];
    }

    // Median per dimension, so a few odd files (loops, wrong octave, mislabelled)
    // can't drag the target.
    Target targetFrom (const std::vector<const Item*>& members, int type)
    {
        auto column = [&] (double Descriptors::* m)
        {
            std::vector<double> v;
            for (auto* item : members)
                v.push_back (item->d.*m);
            return median (v);
        };

        Target t { column (&Descriptors::lufs), column (&Descriptors::plr), column (&Descriptors::centroidHz),
                   column (&Descriptors::bassPct), column (&Descriptors::sidePct) };
        t.note = groupNote (members, type);
        return t;
    }

    // The values the tuner may move, with search bounds and step sizes.
    // Level is set directly at the end, since it is a pure gain after the clipper.
    struct Knob
    {
        const char* name;
        float ProfileTrim::* member;
        float min, max, step, minStep;
    };

    constexpr Knob knobs[] {
        { "OTT depth",    &ProfileTrim::ottDepth,    0.0f,  1.0f, 0.2f, 0.02f },
        { "tilt",         &ProfileTrim::tilt,       -6.0f,  6.0f, 2.0f, 0.25f },
        { "Weight",       &ProfileTrim::weight,      0.0f,  1.0f, 0.2f, 0.02f },
        { "stereo width", &ProfileTrim::stereoWidth, 0.0f,  2.0f, 0.3f, 0.03f },
    };

    ProfileTrim trimOf (const Profile& p)
    {
        return { true, p.levelTrim, p.ottDepth, p.tilt, p.weight, p.stereoWidth };
    }

    Descriptors measure (int genre, int type, const ProfileTrim& trim, int note)
    {
        auto profile = makeProfile (genre, type);
        applyTrim (profile, trim);
        return analyse (renderProfile (genre, type, &profile, note), renderRate);
    }

    // Squared errors in units of compare's verdict thresholds: 1.0 per
    // dimension means "just at the edge of a match".
    double loss (const Descriptors& d, const Target& t, bool withLoudness)
    {
        auto sq = [] (double x) { return x * x; };
        const auto brightness = std::log2 (juce::jmax (20.0, d.centroidHz) / juce::jmax (20.0, t.centroidHz)) / 0.15;

        auto total = sq ((d.plr - t.plr) / 1.5) + sq (brightness) + sq ((d.bassPct - t.bassPct) / 5.0) + sq ((d.sidePct - t.sidePct) / 4.0);
        if (withLoudness)
            total += sq ((d.lufs - t.lufs) / 1.5);
        return total;
    }

    bool withinTolerance (const Descriptors& d, const Target& t)
    {
        return std::abs (d.lufs - t.lufs) <= 1.5 && std::abs (d.plr - t.plr) <= 1.5
            && std::abs (std::log2 (juce::jmax (20.0, d.centroidHz) / juce::jmax (20.0, t.centroidHz))) <= 0.15
            && std::abs (d.bassPct - t.bassPct) <= 5.0 && std::abs (d.sidePct - t.sidePct) <= 4.0;
    }

    struct Result
    {
        ProfileTrim trim;
        Descriptors before, after;
        int renders = 0;
    };

    // Coordinate search: try each knob a step up and down, keep any move that
    // lowers the loss, halve the steps when nothing helps.
    Result tunePair (int genre, int type, const Target& target)
    {
        Result r;
        r.trim = trimOf (makeProfile (genre, type));
        r.before = measure (genre, type, r.trim, target.note);
        ++r.renders;

        auto best = r.before;
        auto bestLoss = loss (best, target, false);
        float steps[std::size (knobs)];
        for (size_t k = 0; k < std::size (knobs); ++k)
            steps[k] = knobs[k].step;

        for (int round = 0; round < 40 && bestLoss > 0.05; ++round)
        {
            bool improved = false;

            for (size_t k = 0; k < std::size (knobs) && ! improved; ++k)
            {
                for (const auto direction : { 1.0f, -1.0f })
                {
                    auto candidate = r.trim;
                    auto& value = candidate.*knobs[k].member;
                    const auto moved = juce::jlimit (knobs[k].min, knobs[k].max, value + direction * steps[k]);
                    if (moved == value)
                        continue;
                    value = moved;

                    const auto d = measure (genre, type, candidate, target.note);
                    ++r.renders;
                    if (const auto l = loss (d, target, false); l < bestLoss)
                    {
                        r.trim = candidate;
                        best = d;
                        bestLoss = l;
                        improved = true;
                        break;
                    }
                }
            }

            if (! improved)
            {
                bool anyLeft = false;
                for (size_t k = 0; k < std::size (knobs); ++k)
                {
                    steps[k] *= 0.5f;
                    anyLeft = anyLeft || steps[k] >= knobs[k].minStep;
                }
                if (! anyLeft)
                    break;
            }
        }

        r.trim.levelDb = juce::jlimit (-18.0f, 12.0f, r.trim.levelDb + (float) (target.lufs - best.lufs));
        r.after = measure (genre, type, r.trim, target.note);
        ++r.renders;
        return r;
    }

    void report (const Target& target, const Result& r)
    {
        std::printf ("    %-12s %9s %9s %9s\n", "", "target", "before", "after");
        std::printf ("    %-12s %9.1f %9.1f %9.1f\n", "lufs",        target.lufs,       r.before.lufs,       r.after.lufs);
        std::printf ("    %-12s %9.1f %9.1f %9.1f\n", "plr_db",      target.plr,        r.before.plr,        r.after.plr);
        std::printf ("    %-12s %9.0f %9.0f %9.0f\n", "centroid_hz", target.centroidHz, r.before.centroidHz, r.after.centroidHz);
        std::printf ("    %-12s %9.1f %9.1f %9.1f\n", "bass_pct",    target.bassPct,    r.before.bassPct,    r.after.bassPct);
        std::printf ("    %-12s %9.1f %9.1f %9.1f\n", "side_pct",    target.sidePct,    r.before.sidePct,    r.after.sidePct);
        std::printf ("    trim: level %+.1f dB, OTT %.2f, tilt %+.1f dB, Weight %.2f, width %.2f  (%d renders)\n",
                     r.trim.levelDb, r.trim.ottDepth, r.trim.tilt, r.trim.weight, r.trim.stereoWidth, r.renders);
    }

    void writeHeader (const juce::File& file, const ProfileTrim (&trims)[12][6])
    {
        juce::String s;
        s << "#pragma once\n\n"
          << "// Per-profile corrections found by `ZeroAnalyze tune` against reference\n"
          << "// sounds. Written by that tool; edit by hand only to clear an entry.\n"
          << "// Untuned entries leave the hand-designed profile unchanged.\n\n"
          << "struct ProfileTrim\n{\n    bool tuned;\n    float levelDb, ottDepth, tilt, weight, stereoWidth;\n};\n\n"
          << "// [genre][type], in genreNames / typeNames order.\n"
          << "inline constexpr ProfileTrim profileTrims[12][6] {\n";

        auto number = [] (float v) { return juce::String (v, 3) + "f"; };

        for (int g = 0; g < 12; ++g)
        {
            s << "    { ";
            for (int t = 0; t < 6; ++t)
            {
                const auto& trim = trims[g][t];
                if (trim.tuned)
                    s << "{ true, " << number (trim.levelDb) << ", " << number (trim.ottDepth) << ", " << number (trim.tilt)
                      << ", " << number (trim.weight) << ", " << number (trim.stereoWidth) << " }";
                else
                    s << "{}";
                s << (t < 5 ? ", " : " ");
            }
            s << "}, // " << genreNames[g] << "\n";
        }
        s << "};\n";

        file.replaceWithText (s);
        std::printf ("Wrote %s; rebuild the plugin to use it.\n", file.getFullPathName().toRawUTF8());
    }
}

int tuneProfiles (const juce::File& refFolder, const juce::File& headerToWrite)
{
    const auto refs = analyseFolder (refFolder);

    std::map<std::pair<int, int>, std::vector<const Item*>> groups;
    for (const auto& r : refs)
        if (r.genre >= 0 && r.type >= 0)
            groups[{ r.genre, r.type }].push_back (&r);

    ProfileTrim trims[12][6];
    for (int g = 0; g < 12; ++g)
        for (int t = 0; t < 6; ++t)
            trims[g][t] = profileTrims[g][t]; // keep earlier results for groups not tuned now

    int tuned = 0;
    for (const auto& [key, members] : groups)
    {
        const auto [genre, type] = key;
        if (members.size() < 3)
        {
            std::printf ("\n%s: only %d refs, skipped (need 3+)\n", groupName (genre, type).toRawUTF8(), (int) members.size());
            continue;
        }

        auto target = targetFrom (members, type);
        target.note = matchedRoot (genre, type, target.note);

        std::printf ("\n%s (%d refs, rendering at %s)\n", groupName (genre, type).toRawUTF8(), (int) members.size(),
                     target.note >= 0 ? juce::MidiMessage::getMidiNoteName (target.note, true, true, 3).toRawUTF8()
                                      : "the default note");

        // Files far from the group on two or more dimensions are probably loops,
        // the wrong octave or mislabelled; list them so they can be pruned.
        for (auto* m : members)
        {
            juce::StringArray far;
            if (std::abs (m->d.lufs - target.lufs) > 4.5)        far.add ("loudness");
            if (std::abs (m->d.plr - target.plr) > 4.5)          far.add ("dynamics");
            if (std::abs (std::log2 (juce::jmax (20.0, m->d.centroidHz) / juce::jmax (20.0, target.centroidHz))) > 0.45)
                                                                 far.add ("brightness");
            if (std::abs (m->d.bassPct - target.bassPct) > 15.0) far.add ("bass");
            if (std::abs (m->d.sidePct - target.sidePct) > 12.0) far.add ("width");
            if (far.size() >= 2)
                std::printf ("    suspect: %s (%s)\n", m->name.toRawUTF8(), far.joinIntoString (", ").toRawUTF8());
        }
        const auto result = tunePair (genre, type, target);
        report (target, result);
        trims[genre][type] = result.trim;
        ++tuned;
    }

    std::printf ("\nTuned %d profiles.\n", tuned);
    if (tuned > 0 && headerToWrite != juce::File())
        writeHeader (headerToWrite, trims);
    else if (tuned > 0)
        std::printf ("Dry run: add --write to save these into Source/ProfileTuning.h.\n");
    return 0;
}

int tuneTest()
{
    // Each case hides a known change, then asks the tuner to recover the render.
    struct Case
    {
        const char* genre;
        const char* type;
        float level, ott, tilt, weight, width; // added to the profile's own values
    };

    const Case cases[] {
        { "Trance",  "Lead",  -4.0f,  0.20f,  2.0f,  0.0f,   0.4f },
        { "Dubstep", "Bass",   3.0f, -0.20f, -1.5f, -0.2f,   0.0f },
        { "EDM",     "Pad",   -2.0f,  0.00f, -2.0f,  0.1f,  -0.4f },
        { "Trap",    "Pluck",  5.0f,  0.25f,  1.0f,  0.0f,   0.3f },
    };

    int failures = 0;
    for (const auto& c : cases)
    {
        const auto genre = genreNames.indexOf (c.genre);
        const auto type = typeNames.indexOf (c.type);

        auto hidden = trimOf (makeProfile (genre, type));
        hidden.levelDb += c.level;
        hidden.ottDepth = juce::jlimit (0.0f, 1.0f, hidden.ottDepth + c.ott);
        hidden.tilt = juce::jlimit (-6.0f, 6.0f, hidden.tilt + c.tilt);
        hidden.weight = juce::jlimit (0.0f, 1.0f, hidden.weight + c.weight);
        hidden.stereoWidth = juce::jlimit (0.0f, 2.0f, hidden.stereoWidth + c.width);

        const auto target = targetFrom (measure (genre, type, hidden, -1));

        std::printf ("\n%s (hidden: level %+.1f, OTT %+.2f, tilt %+.1f, Weight %+.2f, width %+.2f)\n",
                     groupName (genre, type).toRawUTF8(), c.level, c.ott, c.tilt, c.weight, c.width);
        const auto r = tunePair (genre, type, target);
        report (target, r);

        // Pass when every dimension lands inside compare's tolerance. Several
        // knob settings can give the same measurements, so the recovered values
        // needn't equal the hidden ones.
        const auto before = loss (r.before, target, true), after = loss (r.after, target, true);
        const auto passed = withinTolerance (r.after, target) && after < before;
        std::printf ("    error %.2f -> %.2f  %s\n", before, after, passed ? "ok" : "FAILED");
        if (! passed)
            ++failures;
    }

    std::printf (failures == 0 ? "\nTuner found its way back on every case.\n" : "\n%d cases failed.\n", failures);
    return failures == 0 ? 0 : 1;
}
