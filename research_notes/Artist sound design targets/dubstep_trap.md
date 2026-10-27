# Dubstep and Trap sound design targets (Bass, Lead, Chord, Pad, Pluck, FX)

Scope: concrete synth/processing parameters for Dubstep and Trap x {Bass, Lead, Chord, Pad, Pluck, FX}, plus the trap 808. Researched 2026-10-27 using web search only (no full-page fetch tool was available, so findings rest on search-result summaries of the cited pages; treat single-snippet claims with some caution).

Overall reliability note: published, artist-confirmed patch numbers are rare. I found **no** primary source (artist interview or session breakdown) giving exact settings for Skrillex, Subtronics, Excision, Virtual Riot, Zomboy, Marauda, Seven Lions, Illenium, Metro Boomin, Southside, Pierre Bourne, Wheezy, RL Grime or Baauer. Most numbers below come from general tutorials (MusicRadar, ADSR, Violet Recording, EDMProd, Soundbridge, LANDR) or are my own arithmetic/estimates and are labelled as such. Items marked **(estimate)** are engineering judgement, not sourced, and belong in tuning experiments rather than as facts.

---

## 1. Oscillator / wavetable source per type

### Takeaway
Supersaw stacks (7 to 16 unison voices, low-to-moderate detune, two oscillators panned/offset) are the documented basis for melodic-dubstep chords/leads and EDM-trap chords; dubstep basses/screech leads come from FM, waveshaped sines and warped/resampled wavetables; the 808 is a sine with a fast pitch envelope plus saturation; trap pads/plucks/bells are mostly preset-based (Omnisphere, Analog Lab, u-he, FM8) and undocumented in patch terms.

### Cited Findings
**Dubstep Chord (priority gap)**
- Melodic dubstep is described as 135-150 BPM half-time with "heavy sub-bass and aggressive mid-range FM growls set against massive detuned supersaw stacks"; Seven Lions, Illenium, Slander, Said The Sky named as builders of the sound — [Futureproof Genre Explorer](https://genreexplorer.futureproofmusicschool.com/genre/melodic-dubstep)
- A (student) Serum melodic-dubstep project: chord synth = saw + square, unison 7 voices, then high-passed, distorted, EQ boost in upper mids; lead = single saw at 9 unison voices with similar processing. Not an artist patch — [Bradford College student assessment](https://digitalspace.bradfordcollege.ac.uk/10683756/?p=478)
- Illenium "Crawl Outta Love" pad remake: "rapidly modulated wall of detuned saws and white noise" — [Syntorial preset recipe](https://www.syntorial.com/preset-recipe/illenium-crawl-outta-love-pad/)
- Serum supersaw recipe: saw on Osc A, unison 16, pan slightly left, lower detune until "wide but not phasey"; same on Osc B panned the other way; optionally resample the unison stack into a wavetable — [ADSR Sounds pro tip](https://www.adsrsounds.com/serum-tutorials/serum-supersaw-wavetables-pro-tip/)
- Chord-stab tutorial: unison detune kept "relatively low", find a sweet spot — [Production Music Live](https://www.productionmusiclive.com/blogs/news/1-2-5-how-to-make-a-chord-stab-in-serum-sound-design-tutorial)
- Forum practice: 7-8 unison voices, random phase, second oscillator a few cents higher, a little noise (anecdotal) — [KVR forum](https://kvraudio.com/forum/viewtopic.php?p=7432823); two saws offset about ±0.1 semitone (≈ ±10 cents) plus chorus — [Audiotool track notes](https://www.audiotool.com/track/dubstep_chords-z0kbtkFBh)

**Dubstep Bass**
- Growl: custom wavetable, warp mode "bend", LFO on warp and wavetable position, second oscillator on a spectral wavetable — [Bass Gorilla Serum growl](https://bassgorilla.com/serum-tutorial-super-growl-bass-in-ableton-live/)
- Riddim (Subtronics-style, third-party reconstruction): resampling growly basses, Serum presets — [EDMProd riddim guide](https://www.edmprod.com/how-to-make-riddim/)
- Virtual Riot: records long hardware sessions, chops usable parts, loads them into samplers, resynthesises or turns them into Serum wavetables — [Antares interview](https://www.antarestech.com/blog/virtual-riot-interview); Arturia piece on his Pigments 7 workflow contrasts "heavy, dissonant sound design" with clean musical cores (vendor content) — [Arturia stories](https://www.arturia.com/fr/stories/virtual-riot)
- Classic wobble: triangle LFO into filter cutoff on a saw/basic bass — [MusicRadar LFO wobble bass](https://musicradar.com/how-to/lfo-wobble-bass)

**Dubstep Lead (screech)**
- Skrillex/Zomboy/Barely Alive-style lead in Massive: two sine-based oscillators, waveshaper for harsh top, ring modulation, tube-style drive — [Soundbridge modern dubstep leads](https://www.soundbridge.io/modern-dubstep-leads)
- Early Skrillex-era metallic tones attributed to FM synthesis (course description only) — [Udemy Skrillex course](https://www.udemy.com/course/producing-like-skrillex-mastering-edm-techniques/)
- Screech movement from pitch envelopes, LFOs and a sample-and-hold insert — [Loopmasters Massive screech](https://www.loopmasters.com/articles/2107-Massive-Dubstep-Screech-Synth-Leads-FX-Part-3-4)

**Trap 808**
- Sine oscillator + very fast pitch envelope from a few semitones above down to the note; this pitch drop is what separates an 808 from a plain sine — [EDMProd 808s](https://www.edmprod.com/808s/), [Violet Recording 808 from scratch](https://violetrecording.com/how-to-design-an-808/)
- Saturation/distortion to generate harmonics so it reads on small speakers — [LANDR](https://blog.landr.com/what-is-an-808/), [Attack Magazine 808 with saturation](https://www.attackmagazine.com/technique/synth-secrets/808-bass-with-saturation/)

**Trap Chord / Pad / Pluck / Lead (priority gaps)**
- Trap sound-design course syllabus lists "distorted horn stabs" and "modulated super saw chords" as trap staples — [Muse Group / Groove3 course](https://www-cf.musehub.com/course/g3-course-trap-sound-design-explained)
- RL Grime: "trademark brass horn synths" with "massive 808s" (VOID / "Scylla"); "spacey lead synth" on "Art Money" — [EDMTunes](https://www.edmtunes.com/?p=86801), [Fux With It](https://fuxwithit.com/?p=36203)
- Metro Boomin palette (listening analysis, not documented): layered bells, choirs, pads, plucks — "polished but slightly ominous"; dark keys, vintage polysynth textures, detuned leads, bell-like plucks — [Internet Tattoo](https://www.internettattoo.com/blog/what-vst-plugins-does-metro-boomin-use)
- Pierre Bourne reported gear: Omnisphere 2, FabFilter Twin 2, FM8, Absynth 5, u-he ACE (ACE reportedly on Carti's "Mileage") — [Equipboard](https://equipboard.com/pierre-bourne)
- Generic pluck: bright oscillator, near-instant attack, short-medium decay, zero/low sustain, short release; filter starts open and closes during decay — [Violet Recording pluck](https://violetrecording.com/how-to-make-a-pluck-sound/), [Future Music via PressReader](https://www.pressreader.com/australia/future-music-9629/20200728/283510798260820)
- Dark-trap Serum pack describes plucks with "unsettling, atmospheric undertone" and "trap bell arp" presets (product copy) — [Equipboard listing](https://equipboard.com/items/producer-loops-dark-trap-for-serum)

### Inferences
- Dubstep Chord target (estimate): 2 saw oscillators (or saw + square) x 7-9 unison voices each, detune ≈ 10-25 cents spread, random phase, stereo spread high, plus 5-10% white noise; played as 3-4 note triads/7ths in C3-C5. Serves both melodic-dubstep supersaw drop chords and short "chord stabs" (same source, shorter amp envelope).
- Trap Chord target (estimate): two flavours. (a) EDM/festival trap (RL Grime, Baauer): detuned supersaw or brass-like saw stack (brass = saw with filter envelope that opens on attack, ~30-80 ms), distorted. (b) Rap trap (Metro, Southside, Wheezy): darker poly-synth/keys/choir layers, minimal unison, lowpassed.
- Trap Pluck (estimate): bell/FM-ish or soft saw/square pluck, sometimes with a slight detune; Pierre Bourne's sound leans on bright, slightly lo-fi synth plucks (his reported FM8/ACE/Twin 2 use suggests FM bells and simple analog-style plucks).
- Trap Pad (estimate): dark choir/string-like pad, low unison, heavy reverb, lowpassed — "ominous" rather than bright.

### Gaps
- No artist-confirmed patch for any named artist. No source gave Seven Lions/Illenium chord voicings or voice counts, nor Metro/Southside/Wheezy pad/pluck settings. Electra X (often associated with trap producers) did not appear in any result.

---

## 2. Filter types/cutoffs, envelopes (ms), pitch envelopes (808 glide/punch)

### Takeaway
Hard ms numbers are scarce; the documented facts are: 808 pitch envelope is very fast and drops a few semitones, glide ≈100 ms as a starting point, decay set by role (short = punch, long = sustained). Plucks: instant attack, short decay, zero sustain, closing lowpass. Pads: long sustain/release, static lowpass.

### Cited Findings
- 808 pitch envelope routed to transpose, amount "15-25%" of the envelope range sounds best (synth-specific, Vital); higher = laser-like — [EDMProd 808s](https://www.edmprod.com/808s/)
- 808 amp: long release = sustained trap 808, short = punchy thud — [EDMProd 808s](https://www.edmprod.com/808s/), [LANDR](https://blog.landr.com/what-is-an-808/) (decay described only qualitatively)
- 808 glide via pitch-bend automation or synth glide; ~100 ms suggested starting glide time — [Amped Studio 808](https://www.ampedstudio.com/blog/secrets-of-808-bass/); Soundtrap 808 Glide knob (shorter to the left) — [Soundtrap support](https://support.soundtrap.com/hc/en-us/articles/8136899014546-How-to-use-808-with-glide)
- Pluck: near-instant attack, short-medium decay, zero/very low sustain, short release; filter envelope closes during decay — [Violet Recording pluck](https://violetrecording.com/how-to-make-a-pluck-sound/)
- Pad: filter on, extended sustain and release, unison and LFO on oscillator; static lowpass + reverb — [Soundbridge atmospheric pad](https://soundbridge.io/design-your-own-atmospheric-pad-sound), [PML Serum course](https://productionmusiclive.teachable.com/courses/119310/lectures/1741573)
- Trap-style bass example through "MG Lowpass 12" plus a little tape saturation — [PML Serum lecture](https://productionmusiclive.teachable.com/courses/serum/lectures/49902047)
- Melodic dubstep chord/lead: high-passed (cutoff not given) — [Bradford College](https://digitalspace.bradfordcollege.ac.uk/10683756/?p=478)

### Inferences (all estimates)
| Type | Amp A / D / S / R | Filter | Pitch env |
|---|---|---|---|
| 808 | A 0-2 ms, D 400-1500 ms (follows note length; often held to next note), S 0 or full with gate, R 50-300 ms | none or gentle LP ≥ 1-2 kHz after distortion | +12 to +24 st drop over 20-60 ms; glide 60-150 ms legato |
| Dubstep bass | A 0-5 ms, S full, R 30-100 ms | LP/comb/formant, cutoff modulated by LFO 300 Hz-5 kHz | optional short pitch drop 1-2 st on attack |
| Dubstep lead (screech) | A 0-5 ms, S full, R 100-200 ms | BP/notch or none; FM index envelope | vibrato/pitch env optional |
| Dubstep chord stab | A 0-5 ms, D 150-400 ms, S 0-30%, R 100-300 ms | HP ~150-250 Hz; LP env opening on attack | none |
| Dubstep supersaw chord (sustained) | A 5-20 ms, S full, R 200-500 ms | HP ~150-250 Hz | none |
| Trap chord (festival) | A 5-30 ms, S full, R 200-400 ms | LP env, brass-like opening 30-80 ms | none |
| Trap pad | A 200-800 ms, S full, R 1-3 s | LP 1-4 kHz static | none |
| Trap pluck/bell | A 0-2 ms, D 200-600 ms, S 0, R 150-400 ms | LP env closing (or FM index decay) | none |
| FX riser | 2-8 bars (3.4-13.7 s at 140 BPM) | HP/BP sweep up | pitch rise +12 to +24 st |

### Gaps
- No primary source for exact ms values per artist. The "15-25%" pitch-env figure is synth-dependent (not semitones).

---

## 3. Modulation: LFO rates and destinations

### Takeaway
Wobble basses use tempo-synced LFOs on filter cutoff (or wavetable position/warp in modern growls) at 1/4, 1/8, 1/8T, 1/16; supersaw pads/chords get light LFO/rapid modulation; risers/downlifters use pitch envelopes and envelope-modulated LFO rate.

### Cited Findings
- Wobble: triangle LFO -> filter cutoff at 1/8 triplet — [MusicRadar](https://musicradar.com/how-to/lfo-wobble-bass)
- Tempo-synced 1/4, 1/8, 1/16; faster = tighter, busier — [Violet Recording wobble](https://violetrecording.com/how-to-make-a-wobble-bass/)
- Square LFO at 1/4 (gate-like) — [Dubspot](https://blog.dubspot.com/video-tutorial-making-a-dubstep-bass-in-ableton-live); rates between 1/16 and 1/8 incl. triplets — [DJTechTools forum](https://forum.djtechtools.com/t/anybody-know-how-to-make-a-repetitive-wobble-bass/69195); some avoid triplets — [Ableton forum](https://forum.ableton.com/viewtopic.php?p=1249097)
- Modern growl: LFO on wavetable position and warp — [Bass Gorilla](https://bassgorilla.com/serum-tutorial-super-growl-bass-in-ableton-live/)
- Downlifter: LFO rate modulated by a reverse envelope, plus phaser/flanger — [ADSR Massive downlifter](https://www.adsrsounds.com/ni-massive-tutorials/fx-with-massive-part-2-downlifter/); risers via pitch modulation envelopes — [Loopmasters Massive riser](https://www.loopmasters.com/articles/2096-Dubstep-Riser-FX-in-Native-Instruments-Massive-Part-4-4); reverb automation for washy risers — [MusicTech Odin 2](https://musictech.net/tutorials/weekend-workshop-make-risers-and-downlifters-with-odin-2/)

### Inferences
Tempo-synced rates (my arithmetic, not sourced):

| Division | 140 BPM | 150 BPM |
|---|---|---|
| 1/4 | 428.6 ms / 2.33 Hz | 400 ms / 2.5 Hz |
| 1/8 | 214.3 ms / 4.67 Hz | 200 ms / 5.0 Hz |
| 1/8T | 142.9 ms / 7.0 Hz | 133.3 ms / 7.5 Hz |
| 1/16 | 107.1 ms / 9.33 Hz | 100 ms / 10 Hz |
| 1/16T | 71.4 ms / 14 Hz | 66.7 ms / 15 Hz |

- Dubstep tempos 140-150 (melodic dubstep 135-150 per Futureproof); trap commonly 130-150 (Audentity pack tempos 130/135/145/150 — [rekkerd](https://rekkerd.org/?p=141405)).
- Suggested defaults (estimate): dubstep bass Motion macro sweeps LFO rate 1/4 -> 1/16T on cutoff/wavetable; chords/pads: slow LFO 0.1-0.5 Hz on detune/filter, depth small; 808: no LFO; plucks: none or slow vibrato.

### Gaps
- No artist-specific LFO settings found (Excision/Skrillex specifics not documented).

---

## 4. Distortion/saturation and OTT/multiband habits

### Takeaway
Distortion is central to dubstep bass/leads (waveshaper, tube drive, ring mod) and to the 808 (saturation for harmonics); OTT is ubiquitous on synths but documented depth ranges are 5-60% for normal use, 100% for creative sound design.

### Cited Findings
- Screech lead: waveshaper for top end, tube-style drive for body, ring mod — [Soundbridge](https://www.soundbridge.io/modern-dubstep-leads)
- Melodic dubstep chord/lead distorted after high-pass, upper-mid EQ boost — [Bradford College](https://digitalspace.bradfordcollege.ac.uk/10683756/?p=478)
- Sine sub layered with pink noise, saturated together, low-passed ~240 Hz (bass thickening) — [Bass/sub tutorial summary via Computer Music](https://www.pressreader.com/australia/computer-music/20211229/283253101218270)
- 808: tape, tube or overdrive flavours; heavier for aggressive trap; separate "clean 808" vs "distorted 808" lessons — [Amped Studio](https://www.ampedstudio.com/blog/secrets-of-808-bass/), [Groove3 course](https://groove3.com/products/Trap-Sound-Design-Explained)
- OTT depth acts as dry/wet; 30% shown as a subtler setting, 100% for creative design — [EDMProd OTT](https://www.edmprod.com/ott-plugin/); community starting points 5-20% subtle, 20-60% by instrument — [Equipboard OTT](https://equipboard.com/items/xfer-records-ott-multiband-compressor-plugin); OTT on synth leads increases presence/brightness — [SampleFocus OTT guide](https://blog.samplefocus.com/blog/ott-compression-guide-what-it-is-and-how-to-use-multiband-compression); overuse flattens dynamics/adds noise — [Mixelite](https://mixelite.com/blog/ott-compression/)
- DNB/bass course lists "OTT & Applying It To Basses" and "Sub Processing" as standard steps (content not visible) — [EDMProd course outline](https://edmprod.teachable.com/courses/1135170/lectures/24360432)

### Inferences (estimates)
- Dubstep bass: heavy (multi-stage) distortion + OTT 50-100% on the mid layer only; sub kept clean (matches the project's existing "clean sub, dirty mids" principle).
- Dubstep lead: heavy; OTT 40-80%.
- Dubstep chords: moderate distortion (soft clip/tube), OTT 20-40%, sidechain pumping is typical in drops (not sourced here).
- Trap 808: moderate-heavy saturation (rap trap varies from clean to clipped); OTT rarely (estimate); festival trap brass/chords: distorted, OTT 30-60%.
- Trap pads/plucks (rap trap): little distortion, maybe tape/lo-fi; OTT low 0-25%.

### Gaps
- No source quantifies distortion drive in dB or OTT depth for named artists.

---

## 5. Stereo, mono crossover, sub handling

### Takeaway
Sources recommend mono below roughly 100 Hz (some 200 Hz) with a separate clean sine sub; width only above ~200 Hz. Supersaws get maximum width via unison spread and opposite-panned oscillators.

### Cited Findings
- Keep below ~100 Hz mono to avoid phase issues on mono summing — [emastered sub-bass guide](https://emastered.com/de/blog/sub-bass)
- Dubstep analysis: low-pass the sub, keep sub mono, widen only above ~200 Hz — [Dubspot Datsik bassline analysis](https://blog.dubspot.com/electronic-music-theory-analyzing-dubstep-basslines-datsiks-3-fist-style)
- Separate dry sub oscillator kept unaffected by modulation/reverb in a sine-based bass design — [Computer Music via PressReader](https://www.pressreader.com/australia/computer-music/20211229/283253101218270)
- Supersaw: two 16-voice stacks panned opposite — [ADSR](https://www.adsrsounds.com/serum-tutorials/serum-supersaw-wavetables-pro-tip/)

### Inferences (estimates)
- Bass and 808: mono below 100-150 Hz; sub sine crossover 80-120 Hz for dubstep (sub layer LP ~100 Hz, mids HP ~100-150 Hz). 808 effectively mono overall (side share < 5%).
- Leads: mono-ish centre with moderate width (side share 10-25%).
- Chords/supersaws: wide (side share 30-50%, correlation 0.2-0.6) above 200 Hz.
- Pads: wide (side 30-50%). Plucks: moderate (15-35%), often widened by reverb/delay.
- FX: wide (risers 30-60% side), impacts centred low end.

### Gaps
- No measured side-share or correlation numbers from references; the project's own `ZeroAnalyze analyze` on references would be the right way to fill this.

---

## 6. Brightness/tilt, loudness, transient character of a finished one-shot

### Takeaway
No sample pack or tutorial found that publishes LUFS targets or spectral centroid for one-shots; this must be measured from references (ZeroAnalyze). Qualitative: dubstep basses/leads very bright and dense (low crest factor); 808 sub-heavy with saturated harmonics; rap-trap pads/plucks darker.

### Cited Findings
- Sample packs list format (24-bit/44.1 kHz) and counts but no loudness specs — [Loopmasters Phuture Sounds](https://www.loopmasters.com/genres/49-Dubstep/products/18178-Phuture-Sounds?bid=13923), [Gems Color Bass](https://www.loopmasters.com/genres/49-Dubstep/products/18227-Gems-Color-Bass-Sample-Pack); "mixed to immediately sound good" — [Loop Cult Tension](https://loopcult.gumroad.com/l/tension-dubstep-sample-pack)
- OTT increases presence/brightness on synth leads — [SampleFocus](https://blog.samplefocus.com/blog/ott-compression-guide-what-it-is-and-how-to-use-multiband-compression)
- Upper-mid EQ boost on melodic dubstep chords/leads — [Bradford College](https://digitalspace.bradfordcollege.ac.uk/10683756/?p=478)
- Metro Boomin melodic palette "slightly ominous" (listening analysis) — [Internet Tattoo](https://www.internettattoo.com/blog/what-vst-plugins-does-metro-boomin-use)

### Inferences (estimates, to be verified with ZeroAnalyze on references)
- Dubstep bass/lead one-shots: very loud, short-term ≈ -8 to -5 LUFS, PLR ≈ 6-9 dB (heavily compressed/clipped).
- Dubstep chords: ≈ -10 to -7 LUFS; bright (sizeable energy above 4 kHz).
- 808: ≈ -10 to -6 LUFS, most energy below 150 Hz; distorted versions with significant 200 Hz-2 kHz harmonics. Strong transient (pitch-drop click) then sustained.
- Trap pads: quieter, -16 to -12 LUFS, dark (low % above 4 kHz), soft attack. Trap plucks/bells: sharp transient, high PLR (12-16 dB), moderately bright.
- FX: risers broadband, impacts heavy low end + long tail.

### Gaps
- No published LUFS/centroid data for one-shots; all numbers above are estimates.

---

## 7. Register (MIDI note range) per type

### Takeaway
808 root notes cluster around C1-E2 (32.7-82.4 Hz), sweet spot E1-G1 (41-49 Hz); dubstep sub fundamentals similar (sub weight 30-60 Hz). Other ranges are estimates.

### Cited Findings
- 808 tuning range C1 (32.70 Hz) to E2 (82.41 Hz); E1 (41.20 Hz)-G1 (49.00 Hz) "balanced, punchy" (syndicated article with some inconsistent band descriptions) — [Grand Ave / syndicated 808 guide](https://grandavehousing.calpoly.edu/news/in808-the-ultimate-guide)
- F1 (44 Hz) needs more harmonic support than A1 (55 Hz) — [Violet Recording 808s harder](https://violetrecording.com/how-to-make-808s-hit-harder/)
- Ideal sub range 30-60 Hz; main weight 40-80 Hz — [Amped Studio](https://www.ampedstudio.com/blog/secrets-of-808-bass/), [LANDR](https://blog.landr.com/what-is-an-808/)
- Tune 808 to the track key — [LANDR](https://blog.landr.com/what-is-an-808/)

### Inferences (estimates; MIDI numbers with C4 = 60)
| Type | Dubstep | Trap |
|---|---|---|
| Bass / 808 | Sub root E1-A1 (28-33); growl mids played C1-C3 (24-48) | 808 C1-E2 (24-40), sweet spot E1-G1 (28-31); slides up to +12 st |
| Lead | C4-C6 (60-84); screech often higher | C4-C6 (60-84); festival brass C3-C5 |
| Chord | C3-C5 (48-72), triads/7ths, root doubled C2 optional | C3-C5 (48-72) |
| Pad | C3-C5 (48-72) | C3-C5 (48-72), often darker C3-G4 |
| Pluck | C4-C6 (60-84) | C4-C6 (60-84); bells up to C7 (96) |
| FX | n/a (pitch sweeps over 1-2 octaves) | n/a |

### Gaps
- No artist-specific register data.

---

## 8. Artists exemplifying each type

### Takeaway
Attribution is from genre descriptions and press, not from sound-design confirmations.

### Cited Findings
- Melodic dubstep chords/supersaws: Seven Lions, Illenium, Slander, Said The Sky — [Futureproof](https://genreexplorer.futureproofmusicschool.com/genre/melodic-dubstep); Illenium supersaw pad — [Syntorial](https://www.syntorial.com/preset-recipe/illenium-crawl-outta-love-pad/); ADSR "Millenium" pack inspired by Illenium/Seven Lions — [rekkerd](https://rekkerd.org/adsr-sounds-launches-millenium-sound-pack-by-skifonix-sounds/)
- Dubstep screech leads: Skrillex, Zomboy, Barely Alive — [Soundbridge](https://www.soundbridge.io/modern-dubstep-leads)
- Riddim/heavy bass: Subtronics — [EDMProd riddim](https://www.edmprod.com/how-to-make-riddim/), [EDM.com interview](https://edm.com/interviews/subtronics-on-bass-music), [Dancing Astronaut Scream Saver](https://dancingastronaut.com/2020/04/subtronics-breaks-down-his-hardest-hitting-project-yet-scream-saver-ep-interview/)
- Resampled/wavetable bass: Virtual Riot — [Antares](https://www.antarestech.com/blog/virtual-riot-interview), [Splice](https://splice.com/blog/virtual-riot-modestep-producer-tips/)
- Festival trap brass + 808: RL Grime ("Scylla", VOID), Baauer ("Harlem Shake" riff, brass edit of "Tell Me") — [EDMTunes](https://www.edmtunes.com/?p=86801), [Fux With It](https://fuxwithit.com/?p=36203)
- Rap trap melodies (bells, choirs, pads, plucks): Metro Boomin (analysis) — [Internet Tattoo](https://www.internettattoo.com/blog/what-vst-plugins-does-metro-boomin-use); Pierre Bourne gear — [Equipboard](https://equipboard.com/pierre-bourne)

### Inferences
- Note era: classic Skrillex FM screech and LFO wobble (2010-2013) are "older classics"; current (2022-2026) heavy dubstep leans on resampled/riddim growls and wavetable warp modulation (Subtronics, Virtual Riot); melodic dubstep supersaw chords remain current.

### Gaps
- Nothing found for Excision, Marauda, Zomboy-specific basses, Southside, Wheezy. Their patches would need video breakdowns (YouTube) which were not reachable here.

---

## 9. FX (both genres)

### Takeaway
Risers = white noise + pitch envelope + HP sweep + reverb automation over 4-8 bars; downlifters = reverse envelope on LFO rate/pitch with phaser/flanger; impacts undocumented.

### Cited Findings
- Riser: pitch modulation envelopes — [Loopmasters](https://www.loopmasters.com/articles/2096-Dubstep-Riser-FX-in-Native-Instruments-Massive-Part-4-4); reverb automation for washy rise — [MusicTech](https://musictech.net/tutorials/weekend-workshop-make-risers-and-downlifters-with-odin-2/); pitch-bend automation, rhythmic gating, LFO movement — [Udemy transition FX](https://www.udemy.com/sound-design-create-your-own-transition-fx/)
- White noise riser with heavy reverb — [Looperman](https://www.looperman.com/loops/detail/312600/simple-white-noise-riser-free-120bpm-dubstep-fx-loop)
- Downlifter: reverse envelope on LFO rate + phase/flanger — [ADSR](https://www.adsrsounds.com/ni-massive-tutorials/fx-with-massive-part-2-downlifter/)

### Inferences (estimates)
- Engine FX defaults: noise + saw stack source; HP cutoff sweep 200 Hz -> 8 kHz and pitch +12/+24 st over 4 bars (6.9 s at 140, 6.4 s at 150); gate rate accelerating 1/8 -> 1/32; reverb mix rising 20% -> 60%. Downlifter = reverse. Impact = sub sine pitch drop (like 808, +24 -> 0 st over 300-800 ms) + noise burst + 2-4 s reverb tail.

### Gaps
- No sourced numbers for impacts or sweep ranges.
