#pragma once

#include <array>

// About 100 genre-typical 16-step patterns for Generate's arp roll
// (PluginProcessor::generatePattern). steps: one char per step: 'x' plays,
// 'A' plays accented, '.' rests. pitches: semitones from the held note per
// step (rests' values unused), within -12..+24. genres: which suit it, in
// genreNames order: EDM, Dubstep, Techno, Tech House, Trap, Synthwave,
// Drum & Bass, Pop, House, Trance, Future Bass, Hardstyle.
// Generic genre conventions only; no specific song's melody.
namespace ArpGrooves
{
    struct Groove
    {
        const char* name;
        const char* steps;                 // exactly 16 chars
        std::array<signed char, 16> pitches;
        std::array<bool, 12> genres;
        int rate;                          // suggested rate index: 0 1/4, 1 1/8, 2 1/8T, 3 1/16, 4 1/16T, 5 1/32
    };

    inline const Groove all[] {
        // 16th roll with an octave jump on each beat's third 16th
        { "rolling 16ths", "AxxxAxxxAxxxAxxx", { 0,0,12,0,0,0,12,0,0,0,12,0,0,0,12,0 }, { 1,0,1,0,0,1,1,0,0,1,0,1 }, 3 },
        // big-room offbeat: ghost on the beat, octave up on the 'and'
        { "big room offbeat", "x.A.x.A.x.A.x.A.", { 0,0,12,0,0,0,12,0,0,0,12,0,0,0,12,0 }, { 1,0,0,1,0,0,0,0,1,1,0,1 }, 3 },
        // 3-3-2 pluck with a rising answer
        { "progressive pluck 3-3-2", "A..x..x.A..x..x.", { 0,0,0,7,0,0,12,0,0,0,0,3,0,0,10,0 }, { 1,0,0,0,0,0,0,1,1,1,1,0 }, 3 },
        // electro/complextro root-octave bounce
        { "electro octave bounce", "AxAxAxAxAxAxAxAx", { 0,12,0,12,0,12,0,12,0,12,0,12,0,12,0,12 }, { 1,1,0,0,0,1,1,0,0,0,0,1 }, 3 },
        // syncopated drop riff
        { "drop riff stab", "A.xA.xA.A.xA.x..", { 0,0,0,12,0,10,7,0,0,0,0,12,0,15,0,0 }, { 1,1,0,0,1,0,0,0,0,0,1,0 }, 3 },
        // riser: 8ths climbing the minor scale
        { "build climb 8ths", "A.x.A.x.A.x.A.x.", { 0,0,3,0,5,0,7,0,10,0,12,0,15,0,19,0 }, { 1,0,0,0,0,0,0,1,0,1,1,1 }, 3 },
        // Melbourne bounce: offbeat octaves with a pickup
        { "melbourne bounce", "x.Ax.xA.x.Ax.xA.", { 0,0,12,0,0,0,12,0,0,0,12,0,0,3,15,0 }, { 1,0,0,1,0,0,0,0,1,0,0,0 }, 3 },
        // sidechain-style chord pump
        { "festival chord pump", "A.Ax.A.xA.Ax.A.x", { 0,0,0,12,0,0,0,7,0,0,0,12,0,0,0,10 }, { 1,0,0,0,0,0,0,1,1,0,1,0 }, 3 },
        // big-room drop lead with a fill
        { "big room drop lead", "A...A.A.A...A.xA", { 0,0,0,0,12,0,12,0,0,0,0,0,7,0,10,12 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
        // halftime wobble, octave drop on the last hit
        { "halftime wobble", "A...x.x.A...x...", { 0,0,0,0,0,0,12,0,0,0,0,0,-12,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,1,0 }, 3 },
        // triplet-feel wobble hits
        { "triplet wobble", "AxxAxxAxxAxxAxAx", { 0,0,0,0,0,0,12,0,0,0,0,0,0,0,12,0 }, { 0,1,0,0,1,0,0,0,0,0,1,0 }, 4 },
        // riddim: three stabs and a gap
        { "riddim stutter", "A.A.A...A.A.A...", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,0,0 }, 3 },
        // brostep 'talking' line
        { "brostep talk", "AxA.AxA.x.A.AxAx", { 0,0,12,0,0,12,10,0,7,0,12,0,0,0,-12,0 }, { 1,1,0,0,0,0,1,0,0,0,0,0 }, 3 },
        // drop call and answer
        { "drop call", "A.....x.A..x..x.", { 0,0,0,0,0,0,12,0,0,0,0,10,0,0,7,0 }, { 0,1,0,0,1,0,0,0,0,0,1,0 }, 3 },
        // 3-3-2 growl hits
        { "growl 3-3-2", "A..A..A.A..A..A.", { 0,0,0,0,0,0,12,0,0,0,0,3,0,0,-12,0 }, { 1,1,0,0,1,0,1,0,0,0,0,0 }, 3 },
        // halftime with an octave drop
        { "halftime octave drop", "A.......A...x.x.", { 0,0,0,0,0,0,0,0,-12,0,0,0,0,0,12,0 }, { 0,1,0,0,1,0,1,0,0,0,0,0 }, 3 },
        // wobble that speeds from quarters to 16ths
        { "wobble accelerate", "A...A...AxAxAAAA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,1,0,0,0,0,0,0,0,0,1,1 }, 3 },
        // riddim triplet groups with gaps
        { "riddim triplet gap", "AxA..AxA..AxA..A", { 0,12,0,0,0,0,12,0,0,0,0,12,0,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,0,0 }, 3 },
        // hypnotic one-note 16ths, offbeat accents
        { "one-note pulse", "xxAxxxAxxxAxxxAx", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,1,1,0,0,0,0,0,1,0,0 }, 3 },
        // sparse hypnotic 3-3-4
        { "hypnotic 3-3-2", "A..x..A...x..A..", { 0,0,0,12,0,0,0,0,0,0,7,0,0,0,0,0 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // 303-style acid line, accents stand in for slides
        { "acid 303 line", "AxxAx.Axx.AxxAxA", { 0,12,0,3,0,0,15,0,7,0,0,12,10,0,3,12 }, { 0,0,1,1,0,0,0,0,1,1,0,0 }, 3 },
        // acid squelch with low drops
        { "acid squelch", "Ax.xAxxAx.AxAx.x", { 0,0,0,12,0,3,0,12,-12,0,0,7,0,10,0,0 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // minimal blip
        { "minimal blip", "x...x..x..x...x.", { 0,0,0,0,7,0,0,12,0,0,0,0,0,0,3,0 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // rumble in fives over the bar
        { "rumble fives", "AxxxxAxxxxAxxxxA", { 0,0,-12,0,0,0,0,-12,0,0,0,0,-12,0,0,0 }, { 0,0,1,1,0,0,1,0,0,0,0,1 }, 3 },
        // Detroit-style chord stab
        { "detroit chord stab", "A..A....A..A..A.", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // industrial root / sub-octave
        { "industrial octave", "Ax.AxA.xAx.AxA.x", { 0,-12,0,0,-12,0,0,-12,0,-12,0,0,-12,0,0,-12 }, { 0,0,1,0,0,0,1,0,0,0,0,1 }, 3 },
        // 3-against-4 polymeter
        { "polymeter 3 over 4", "Ax.Ax.Ax.Ax.Ax.A", { 0,12,0,0,12,0,0,12,0,0,12,0,0,12,0,0 }, { 0,0,1,1,0,0,0,0,0,1,0,0 }, 3 },
        // dub-techno chord with echo-like repeats
        { "dub techno chord", "A.....x...A.....", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // tech-house bass bounce
        { "tech house bounce", "x.A.x.Ax..A.x.Ax", { 0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,-12 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // rubbery offbeat bass
        { "rubber offbeat bass", "x.Axx.A.x.Axx.Ax", { 0,0,0,12,0,0,0,0,0,0,0,10,7,0,0,12 }, { 0,0,0,1,0,0,0,0,1,0,0,0 }, 3 },
        // shuffled stab
        { "shuffle stab", "A..x.x..A..x.x..", { 0,0,0,0,0,7,0,0,0,0,0,0,0,3,0,0 }, { 0,0,0,1,0,0,0,1,1,0,0,0 }, 3 },
        // 3-3-2 bass with octave moves
        { "groove 3-3-2 bass", "A..A..A.x..A..x.", { 0,0,0,12,0,0,0,0,-12,0,0,0,0,0,3,0 }, { 1,0,0,1,0,0,0,0,1,0,0,0 }, 3 },
        // chopped vocal-style stab
        { "chopped vocal stab", "A.xx..A..x.x..A.", { 0,0,0,12,0,0,7,0,0,0,0,3,0,0,10,0 }, { 0,0,0,1,0,0,0,1,1,0,1,0 }, 3 },
        // 16th groove with gaps
        { "16th groove", "Axx.xAx.Axx.xAx.", { 0,0,12,0,0,0,12,0,0,0,12,0,0,3,15,0 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // swung rumble
        { "swing rumble", "A.x.xA.xA.x.xA.x", { 0,0,-12,0,0,0,0,-12,0,0,-12,0,0,0,0,-12 }, { 0,0,1,1,0,0,1,0,0,0,0,0 }, 3 },
        // jackin' bass
        { "jackin bass", "A..xA..xA..xA.xx", { 0,0,0,12,0,0,0,12,0,0,0,10,0,0,12,0 }, { 0,0,0,1,0,0,0,0,1,0,0,0 }, 3 },
        // offbeat-accented 16th roll
        { "offbeat 16th roll", "xAxAxAxAxAxAxAxA", { 0,0,0,12,0,0,0,12,0,0,0,12,0,0,0,12 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // 808 halftime with a slide-up
        { "808 halftime", "A......xA..x....", { 0,0,0,0,0,0,0,12,0,0,0,-12,0,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,1,0 }, 3 },
        // triplet roll
        { "triplet roll", "AxxAxxAxxAxxAxxA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12 }, { 0,1,0,0,1,0,0,0,0,0,1,0 }, 4 },
        // hi-hat roll stutter
        { "hat roll stutter", "A.x.A.x.AxxxAxAx", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,1,0 }, 5 },
        // trap bell melody
        { "trap bell melody", "A..x..x.A.x.x...", { 0,0,0,3,0,0,7,0,10,0,7,0,3,0,0,0 }, { 0,0,0,0,1,0,0,1,0,0,1,0 }, 3 },
        // 808 glide notes
        { "808 glide", "A.....A...A.....", { 0,0,0,0,0,0,12,0,0,0,7,0,0,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,0,0 }, 3 },
        // triplet flow arp
        { "triplet flow", "AxxAxxAxxA.xAxx.", { 0,3,7,0,3,7,0,3,7,12,0,10,7,3,0,0 }, { 0,0,0,0,1,0,0,1,0,0,1,0 }, 2 },
        // drill-style bounce
        { "drill bounce", "A..x..A...x.A...", { 0,0,0,0,0,0,12,0,0,0,-12,0,3,0,0,0 }, { 0,1,0,0,1,0,0,0,0,0,0,0 }, 3 },
        // trap pluck with octaves
        { "trap pluck", "A..x..A.x.A.A...", { 0,0,0,12,0,0,0,0,12,0,15,0,10,0,0,0 }, { 0,0,0,0,1,0,0,1,0,0,1,0 }, 3 },
        // phonk cowbell 3-3-2
        { "phonk cowbell", "AxxAxxAxAxxAxxAx", { 0,7,12,0,7,12,0,12,0,7,12,0,7,12,0,12 }, { 0,0,0,0,1,0,0,0,0,0,1,0 }, 3 },
        // 8th-note octave arp through a minor progression
        { "8th octave arp", "AxxxAxxxAxxxAxxx", { 0,12,0,12,0,12,0,12,3,15,3,15,5,17,5,17 }, { 0,0,0,0,0,1,0,1,0,1,0,0 }, 1 },
        // galloping bass
        { "gallop bass", "A.xxA.xxA.xxA.xx", { 0,0,0,0,0,0,0,0,5,0,5,5,3,0,3,3 }, { 0,0,0,0,0,1,1,0,0,1,0,1 }, 3 },
        // minor arp up
        { "minor arp up", "Axxxxxxxxxxxxxxx", { 0,3,7,12,0,3,7,12,0,3,7,12,0,3,7,12 }, { 0,0,0,0,0,1,0,1,0,1,1,0 }, 3 },
        // retro up-down arp, 6-6-4
        { "retro up-down", "AxxxxxAxxxxxAxxx", { 0,3,7,12,7,3,0,3,7,12,7,3,0,7,12,19 }, { 0,0,0,0,0,1,0,1,0,1,0,0 }, 3 },
        // quarter-note synth stabs
        { "synth stab quarters", "A...x...A...x...", { 0,0,0,0,7,0,0,0,3,0,0,0,10,0,0,0 }, { 0,0,0,0,0,1,0,1,1,0,0,0 }, 3 },
        // night-drive 16th bass
        { "night drive 16ths", "AxxAxxxxAxxAxxxx", { 0,0,0,0,0,0,0,0,5,5,5,5,5,5,5,5 }, { 0,0,1,0,0,1,1,0,0,0,0,0 }, 3 },
        // darksynth pulse with sub drops
        { "darksynth pulse", "AAxxAAxxAAxxAAxx", { 0,0,-12,-12,0,0,-12,-12,0,0,-12,-12,0,0,-12,-12 }, { 0,0,1,0,0,1,0,0,0,0,0,1 }, 3 },
        // spread arp with 7ths
        { "arp 7th spread", "AxAxxAxAxxAxAxxA", { 0,7,12,7,0,3,10,15,10,3,0,7,12,19,12,7 }, { 0,0,0,0,0,1,0,1,0,1,0,0 }, 3 },
        // italo octave bass walking
        { "italo octave walk", "AxAxAxAxAxAxAxAx", { 0,12,0,12,0,12,0,12,5,17,5,17,3,15,3,15 }, { 0,0,0,0,0,1,0,1,1,0,0,0 }, 3 },
        // rolling reese
        { "rolling reese", "AxxxxxAxxxAxxxxx", { 0,0,0,-12,0,0,0,0,0,-12,0,0,0,12,0,0 }, { 0,1,0,0,0,0,1,0,0,0,0,0 }, 3 },
        // two-step bass
        { "two-step bass", "A.........A..x..", { 0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0 }, { 0,1,0,0,0,0,1,0,0,0,0,0 }, 3 },
        // liquid arp
        { "liquid arp", "Ax.x.xAx.x.xAx.x", { 0,7,0,12,0,15,3,10,0,15,0,19,0,7,0,12 }, { 0,0,0,0,0,0,1,1,0,1,1,0 }, 3 },
        // neuro stab
        { "neuro stab", "A.A.xA..A.xA.A.x", { 0,0,12,0,0,10,0,0,0,0,-12,7,0,12,0,0 }, { 0,1,0,0,0,0,1,0,0,0,0,0 }, 3 },
        // jungle chop
        { "jungle chop", "AxAAxAxAAxAxAAxA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,1,0,0,0,0,1,0,0,0,0,0 }, 3 },
        // DnB roller
        { "dnb roller", "A.xxA.x.A.xxA.x.", { 0,0,12,0,0,0,12,0,0,0,15,12,0,0,10,0 }, { 0,0,0,0,0,1,1,0,0,0,0,0 }, 3 },
        // halfstep
        { "halfstep", "A.......x.x.A...", { 0,0,0,0,0,0,0,0,0,0,12,0,-12,0,0,0 }, { 0,1,0,0,1,0,1,0,0,0,0,0 }, 3 },
        // rolling sub 3-3-2-2-2
        { "rolling sub", "A..A..A.A.A.A...", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,1,0,0,0,1,0,0,0,0,0 }, 3 },
        // pop pluck 8ths
        { "pop pluck 8ths", "A.x.A.x.x.x.A.x.", { 0,0,7,0,12,0,7,0,3,0,7,0,10,0,7,0 }, { 1,0,0,0,0,0,0,1,1,0,1,0 }, 3 },
        // tropical 3-3-4-3 pluck
        { "tropical pluck", "A..x..x...x..x..", { 0,0,0,7,0,0,12,0,0,0,10,0,0,7,0,0 }, { 1,0,0,0,0,0,0,1,1,0,1,0 }, 3 },
        // broken chords
        { "broken chord", "Axx.Axx.Axx.Axx.", { 0,3,7,0,5,10,12,0,3,7,10,0,0,7,12,0 }, { 0,0,0,0,0,1,0,1,0,1,0,0 }, 3 },
        // offbeat 16th pairs
        { "offbeat pairs", "x.AAx.AAx.AAx.AA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,1,1,0,1,0 }, 3 },
        // pop synth bass 8ths
        { "pop bass 8ths", "A.A.A.A.A.A.A.A.", { 0,0,0,0,0,0,12,0,5,0,5,0,3,0,3,0 }, { 0,0,0,0,0,1,0,1,1,0,0,0 }, 3 },
        // 1/32 sparkle arp
        { "sparkle arp", "AxxxxxxAxxxxxxxx", { 0,3,7,12,15,19,24,19,15,12,7,3,0,7,12,19 }, { 0,0,0,0,0,0,0,1,0,1,1,0 }, 5 },
        // dance-pop stab
        { "dance pop stab", "A..A..A...A.A...", { 0,0,0,0,0,0,7,0,0,0,3,0,5,0,0,0 }, { 1,0,0,0,0,0,0,1,1,0,0,0 }, 3 },
        // pop chord rhythm
        { "pop chord rhythm", "A.xA.xA.x.xA.xA.", { 0,0,0,0,0,0,0,0,5,0,5,5,0,3,7,0 }, { 0,0,0,0,0,0,0,1,0,0,1,0 }, 3 },
        // house organ stab
        { "house organ", "A..x.x.xA..x.x.x", { 0,0,0,0,0,12,0,0,0,0,0,0,0,12,0,7 }, { 1,0,0,1,0,0,0,1,1,0,0,0 }, 3 },
        // house piano offbeats
        { "house piano offbeat", "x.A.x.A.x.A.xAA.", { 0,0,0,0,0,0,0,0,0,0,0,0,0,5,7,0 }, { 0,0,0,0,0,0,0,1,1,0,0,0 }, 3 },
        // deep-house bass
        { "deep house bass", "A..A.x..A..A.xA.", { 0,0,0,0,0,12,0,0,0,0,0,-12,0,10,12,0 }, { 0,0,0,1,0,0,0,0,1,0,0,0 }, 3 },
        // garage skip
        { "garage skip", "A.xA..x.A.xA..x.", { 0,0,0,7,0,0,0,0,0,0,0,10,0,0,12,0 }, { 0,0,0,0,0,0,0,1,1,0,1,0 }, 3 },
        // disco octave bass
        { "disco octave bass", "A.AxA.AxA.AxA.Ax", { 0,0,12,0,0,0,12,0,5,0,17,5,3,0,15,3 }, { 0,0,0,0,0,1,0,1,1,0,0,0 }, 3 },
        // French filter-house chop
        { "filter house chop", "AxxxAx.xAxxxAx.x", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,0,0,1,0,0,0,0,1,0,0,0 }, 3 },
        // Chicago jack
        { "chicago jack", "AxA.xA.xAxA.xA.x", { 0,12,0,0,0,0,0,12,0,12,0,0,0,0,0,12 }, { 0,0,1,1,0,0,0,0,1,0,0,0 }, 3 },
        // trance gate
        { "trance gate", "Ax.xAx.xAx.xAx.x", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
        // trance gate, broken
        { "trance gate broken", "AxAx.xAxAx.xAxA.", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
        // uplifting triad roll
        { "uplifting triad roll", "AxxxxxxxAxxxxxxx", { 0,3,7,12,15,12,7,3,0,3,7,12,15,12,7,3 }, { 1,0,0,0,0,0,0,1,0,1,0,0 }, 3 },
        // rolling bass between kicks
        { "trance rolling bass", "xAAAxAAAxAAAxAAA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,0,1,0,0,0,0,0,0,1,0,1 }, 3 },
        // psy gallop in triplets
        { "psy gallop", "xAAxAAxAAxAAxAAx", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,1,0,0,0,0,0,0,1,0,0 }, 4 },
        // 8th lead over an octave pedal
        { "pedal 8th lead", "A.x.A.x.A.x.A.x.", { 0,0,12,0,7,0,12,0,3,0,12,0,10,0,12,0 }, { 0,0,0,0,0,1,0,0,0,1,0,1 }, 3 },
        // anthem stab
        { "anthem stab", "A..A..A...A..A..", { 0,0,0,0,0,0,0,0,0,0,5,0,0,7,0,0 }, { 1,0,0,0,0,0,0,0,0,1,1,1 }, 3 },
        // 5th arp
        { "trance 5th arp", "AxxxxxxxxxxxAxxx", { 0,7,12,7,0,7,12,15,12,7,12,7,3,10,15,10 }, { 0,0,0,0,0,1,0,0,0,1,0,0 }, 3 },
        // pluck triplets
        { "pluck triplets", "AxxAxxAxxAxx....", { 0,3,7,12,7,3,0,3,7,12,15,19,0,0,0,0 }, { 0,0,0,0,0,0,0,1,0,1,1,0 }, 2 },
        // chord stutter
        { "chord stutter", "AxxxA.AxA..xA.xx", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,0,0,0,0,0,0,1,0,0,1,0 }, 3 },
        // supersaw chop
        { "supersaw chop", "A.AAA.A.A.AAA.A.", { 0,0,0,0,0,0,12,0,0,0,0,0,0,0,7,0 }, { 1,0,0,0,0,0,0,1,0,0,1,0 }, 3 },
        // vocal-chop melody
        { "vocal chop melody", "A..x.xA..x.xA.x.", { 0,0,0,3,0,7,12,0,0,10,0,7,15,0,12,0 }, { 0,0,0,1,0,0,0,1,0,0,1,0 }, 3 },
        // triplet chord stutter
        { "triplet chord stutter", "AxxAxxA.xAxxA.xA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,1,0,0,0,0,0,1,0 }, 4 },
        // 1/32 stutter fill
        { "stutter fill", "A.......AxAxAAAA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12 }, { 1,1,0,0,0,0,0,0,0,0,1,0 }, 5 },
        // kawaii pluck
        { "kawaii pluck", "Ax.xA.x.xA.x.A.x", { 0,12,0,7,15,0,12,0,10,19,0,15,0,12,0,7 }, { 0,0,0,0,0,0,0,1,0,0,1,0 }, 3 },
        // halftime chord hits
        { "halftime chord hits", "A..A......A..A..", { 0,0,0,0,0,0,0,0,0,0,5,0,0,3,0,0 }, { 0,1,0,0,1,0,0,1,0,0,1,0 }, 3 },
        // future bass arp
        { "future bass arp", "AxxAxx.xAxxAxx.x", { 0,7,12,15,12,7,0,3,0,7,12,19,15,12,0,7 }, { 0,0,0,0,0,0,0,1,0,1,1,0 }, 3 },
        // offbeat lead with lifts
        { "hardstyle offbeat lead", "x.A.x.Axx.A.x.AA", { 0,0,12,0,0,0,12,15,0,0,10,0,0,0,12,19 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
        // euphoric lead, 3-3-2 accents on 8ths
        { "euphoric lead", "A.x.x.A.x.x.A.x.", { 0,0,7,0,12,0,15,0,12,0,7,0,10,0,12,0 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
        // rawstyle screech
        { "rawstyle screech", "AAxAAxAxAAxAAxAx", { 0,12,0,0,12,0,12,0,0,12,0,0,12,0,15,0 }, { 0,1,0,0,0,0,0,0,0,0,0,1 }, 3 },
        // kick-tail offbeats
        { "kick tail offbeat", "x.xAx.xAx.xAx.xA", { 0,0,0,12,0,0,0,12,0,0,0,12,0,0,0,12 }, { 0,0,1,0,0,0,0,0,0,1,0,1 }, 3 },
        // hardstyle gate
        { "hardstyle gate", "AxxAAxxAAxxAAxxA", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
        // triplet lead over a pedal
        { "hardstyle triplet lead", "A.xA.xA.xA.xA.xA", { 0,0,12,7,0,12,3,0,12,10,0,12,0,0,12,15 }, { 0,0,0,0,0,0,0,0,0,1,0,1 }, 4 },
        // frenchcore-style stomp
        { "stomp 16ths", "AAAxAAAxAAAxAAAx", { 0,0,12,0,0,0,12,0,0,0,12,0,0,0,12,0 }, { 1,0,1,0,0,0,0,0,0,0,0,1 }, 3 },
        // 3-3-2 anthem
        { "hardstyle anthem 3-3-2", "A..A..A.A.A.A.A.", { 0,0,0,12,0,0,7,0,0,0,12,0,15,0,12,0 }, { 1,0,0,0,0,0,0,0,0,1,0,1 }, 3 },
    };
}
