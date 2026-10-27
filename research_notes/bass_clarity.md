# Bass clarity: clean sub, dirty mids (2026-10-26)

Research for the dubstep bass, and where the same idea applies elsewhere. Sources are tutorials, mixing guides and community reconstructions of Skrillex's methods; no first-hand Skrillex interview on bass design was found, so treat the Skrillex specifics as reported, not confirmed.

## The principle

Every modern bass-music guide agrees on one structure:

1. **A clean sine sub, mono, alone below ~100–150 Hz.** One layer owns the sub range; nothing else plays there. A reconstruction of Skrillex's setup routes the sub straight to the master, never through the mid-bass processing, and ducks it against the kick.
2. **The movement and dirt live in the mids**, roughly 150 Hz to 3.5 kHz, where phone and laptop speakers can still reproduce them. That layer is high-passed at ~100–150 Hz, then takes the distortion, waveshaping, FM, vowel/formant filtering and multiband processing.
3. **Clarity in the mix:** notch the growl's fixed resonances (4–8 dB, Q 6–10), and keep layers few, because more bass layers mean more masking.
4. **Resampling** (bounce, re-process, repeat) is a big part of the Skrillex sound. It's a studio workflow; in Zero Engine its closest match is the later Variation/generation work.

## The gap in Zero Engine (as of c12d39e)

Each voice adds its sine sub to the oscillator output before the finishing chain, so the sub goes through Grit's saturation and the OTT with everything else. Mono-below-120 Hz keeps it mono, but it isn't clean or separate.

**Built in b6b1794 and rolled out after it** (Sub Split and Mid High-Pass in the Dev panel). On: dubstep, DnB, hardstyle, tech house (120 Hz); EDM, future bass, house, pop (90 Hz); trap 808 (80 Hz). Off for now: synthwave (single analog layer) and techno, whose loudness came from the driven sub: split, it measured about 6 LUFS quieter because a clean sine has none of the harmonics the saturation used to add. The clean sub gets a fixed +8 dB so full Weight peaks around -6 dBFS, under the clipper's knee.

The original proposal: split each voice into two signals.
- **Sub:** sine at the note (or an octave down), skips saturation, space and OTT, and rejoins just before the soft clipper, mono.
- **Mid/top:** the wavetable layers, high-passed at a profile-set frequency (default ~120 Hz for basses), then the whole finishing chain.
- No new main-screen controls: Weight still sets the sub level and Grit still drives the mids. One Dev value per profile: the mid-layer high-pass frequency.

## Where it applies

| Profile | Apply | Notes |
|---|---|---|
| Dubstep bass | Yes, first | Clean sub under the classic wobble; the wobble and grit only in the mids. |
| Drum & Bass bass | Yes | Reese and neuro: the detuned saws smear in the low end; a clean sub under them is standard. |
| Hardstyle bass | Yes | Distorted kick-bass on top, clean tail sub underneath. |
| Techno / Tech House bass | Yes | Rolling filtered mid-bass over a steady sub. |
| EDM / Future Bass / House / Pop bass | Yes, lighter | Less distortion, but the same separation keeps the low end tight. |
| Trap bass (808) | Partly | The 808's distortion is its character; keep it saturated but layer the clean sub underneath so the low end stays solid when Grit is high. |
| Synthwave bass | Optional | Analog-style basses are usually one layer; try it and compare. |
| Leads, chords, pads, plucks | Yes, the other half | High-pass these (~150 Hz for chords and pads, higher for leads and plucks) so nothing fights the bass's sub. Their own sine sub (an octave down) should be off or very low. |
| FX | Case by case | Impacts and drops may want a clean sub boom; risers and sweeps should be high-passed. |

## How to check it

`ZeroAnalyze compare` and `tune` already measure bass share below 150 and 60 Hz. After the split, a bass profile should keep its sub share as Grit goes up, instead of the sub getting distorted and compressed. Compare against the owner's references once they're in.

## Sources

- [Udemy: Producing Like Skrillex](https://www.udemy.com/course/producing-like-skrillex-mastering-edm-techniques/)
- [Ask.Video: Sound Like Skrillex in Ableton Live, Part 2 (resampling)](https://ask.video/article/ableton-live/sound-like-skrillex-ableton-live-part-2)
- [Image-Line forum: Skrillex growl bass](https://forum.image-line.com/viewtopic.php?p=602778)
- [Musicians Institute: Skrillex's "Rumble" bass line](https://www.mi.edu/in-the-know/sound-design-tips-make-skrillexs-rumble-bass-line/)
- [Dubstepforum: How to sound like Skrillex](https://www.dubstepforum.com/forum/viewtopic.php?t=198681&start=340)
- [Dubstepforum: Skrillex reveals details on how he makes his basslines](https://www.dubstepforum.com/forum/viewtopic.php?t=210760&start=120)
- [Gearspace: Skrillex! I want to be Skrillex!!](https://gearspace.com/threads/skrillex-i-want-to-be-skrillex-how-do-i-do-it.539453/)
- [Gearspace: Skrillex's tracks (mastering)](https://gearspace.com/threads/skrillexs-tracks-mastering.1046675/)
- [KAN Samples: Sound design for DnB and dubstep](https://kansamples.com/blogs/learn/sound-design-dnb-dubstep)
- [Unison: How to make dubstep beats](https://unison.audio/how-to-make-dubstep/)
- [Point Blank: How to create a dubstep drop from scratch](https://www.pointblankmusicschool.com/blog/how-to-create-a-dubstep-drop-from-scratch/)
- [Orphiq: What is dubstep](https://orphiq.com/resources/what-is-dubstep)
- [EDM Templates: How to mix a dubstep drop](https://edmtemplates.net/blogs/edm-templates-blog/how-to-mix-a-dubstep-drop)
- [TrackSensei: How to mix dubstep](https://tracksensei.com/blog/how-to-mix-dubstep)
- [BeatmakingTools: Bass design guide](https://beatmakingtools.com/techniques/bass-design-guide)
- [MusicProductionWiki: How to make dubstep](https://musicproductionwiki.com/articles/how-to-make-dubstep)
