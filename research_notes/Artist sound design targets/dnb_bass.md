# Drum & bass / bass-music bass and lead sound design: targets for Zero Engine's EDM and Dubstep Bass/Lead profiles

Scope note: researched 2026-10-27 with web search only (no full-page fetch was available, so several articles were read only as search excerpts; where an excerpt cut off, that is marked). First-hand artist statements with patch numbers are rare. Almost every number below comes from **third-party reconstructions/tutorials** (Attack Magazine, MusicTech, MusicRadar, Computer Music, Splice, Syntorial, forums), not from the artists. Values marked **(calc)** are my arithmetic; values marked **(estimate)** are my judgement, not sourced.

## Oscillator source, unison voice counts and detune

### Takeaway
Current DnB bass splits into three source families: (1) **detuned-saw reese** (2 voices / 2 oscillators detuned roughly ±0.07 to ±0.6 semitone, or many tight voices); (2) **"foghorn"/jump-off bass** (Chase & Status "Baddadan", 2023) built from stacked saws at root + fifth + octave-and-fifth plus light FM; (3) **FM/wavetable growl** for neuro and dubstep (sine/basic-shape carrier, FM modulator up 1–3 octaves, warp ~20–50%). Unison counts are low for bass (2–6 voices), high for supersaw leads (7–9 per osc).

### Cited Findings
- **Chase & Status "Baddadan" (2023) foghorn bass, Attack Magazine reconstruction (Serum 2-style, retrigger mode, 174 BPM):** OSC1 plain saw at volume 32; OSC2 saw tuned **+7 semitones** at volume 32; OSC3 tuned **+19 semitones** (octave + fifth) at full volume; OSC4 FM oscillator at volume 0, brought up by an envelope, FM knob at 10 "for a subtle edge". Later steps (filter, saturation) were cut from the excerpt; the article's summary describes the result as a "greasy, mid-rangey Reese bass" next to a punchy kick and constant sub, processed with EQ, Ableton Drum Bus, saturation and limiting — [Attack Magazine: The Baddadan Synth Bass Sound](https://www.attackmagazine.com/technique/synth-secrets/the-baddadan-synth-bass-sound/)
- **Chase & Status foghorn bass (MusicTech, Serum):** OSC A on the "PWM DS" wavetable "because it has plenty of harmonics", unison **+3 voices**; an envelope moves the wavetable position; OSC B = sine pitched **up 3 octaves**, level 0, FM into OSC A at **50%**; sub oscillator plus a **Low 24 filter at ~70 Hz** for the low end, cutoff modulated by Env 2; distortion afterwards (settings cut off). The same guide's reese: saw wavetable, **3 unison voices**, low-pass + distortion — [MusicTech: How to make gnarly bass sounds like Chase & Status](https://musictech.com/guides/essential-guide/chase-and-status-bass-garage-production-tutorial/)
- MusicTech notes the foghorn bass ("When It Rains", 2024 LP) "has become a staple sound for many DnB producers" — [MusicTech](https://musictech.com/guides/essential-guide/chase-and-status-bass-garage-production-tutorial/)
- **Classic reese (Renegade "Terrorist", 1994 — older classic):** 2 unison voices, spread 65%, detune 50%, adjusted to about **±61 cents** so the beating locks to tempo on at least one note; 24 dB/oct low-pass around 4 kHz — [MusicRadar: classic reese bass](https://musicradar.com/how-to/classic-reese-bass-renegade-terrorist)
- **Modern tight reese (Attack, Twin 3, Fred again-inspired):** mono, **64 unison voices**, spread 10%, two saws detuned only **±0.070 semitone**; width 200%; chorus 60% "double" mode, rate 0.005 Hz, depth 100%, delay 30 ms; filter cutoff lowered to **200 Hz** for the classic version — [Attack Magazine](https://www.attackmagazine.com/technique/synth-secrets/fred-again-inspired-reese-basslines-with-twin-3/)
- **Sine reese (Attack "Reese Bass Redux"):** two sines fine-tuned **±0.27 semitone**; amp sustain 1.00, release **24 ms**, mono voice mode — [Attack Magazine: Reese Bass Redux](https://www.attackmagazine.com/technique/tutorials/reese-bass-redux/)
- Forum practice: detune one or two oscillators about **±25 cents** (one up, one down) for a darker, edgier reese "very effective for drum and bass and dubstep" — [dnbforum: making a reese sound](https://dnbforum.com/threads/making-a-reese-sound.88029/)
- Futureproof: reese = 2 saws (one fine-detuned) or 1 osc with 2 unison voices; the signature comes from the voices passing 180° and cancelling — [Futureproof Music School](https://futureproofmusicschool.com/blog/how-do-i-make-a-reese-bass-from-scratch)
- A Serum reese variant uses **6 unison voices** for a wide sound — [RouteNote](https://create.routenote.com/blog/how-to-make-a-reese-bass/)
- **Neuro (Computer Music, 2016 — older):** Serum, 174 BPM, **mono + portamento**, saturation and filter movement in external plugins — [Computer Music via PressReader](https://www.pressreader.com/australia/computer-music/20160615/283201557570027)
- **Noisia-style (Attack, Ableton Operator):** FM synth (Operator) as source, layers through Overdrive (drive 25%) then OTT 100% — [Attack Magazine: Noisia-inspired synths with Operator](https://www.attackmagazine.com/technique/synth-secrets/learn-how-to-make-noisia-inspired-synths-with-abletons-operator/)
- **FM growl (dubstep):** two Basic Shapes oscillators, carrier/modulator with the modulator muted but active, FM ~**20%** — [BassGorilla robotic bass](https://bassgorilla.com/serum-bass-tutorial-robotic-digital-dubstep-bass/); wavetable on A, FM from B, warp ~**40%** (Virtual Riot-style) — [WA Production](https://blog.waproduction.com/how-to-create-virtual-riot-bass); modulator an octave down for darker FM — [AudioTent](https://www.audiotent.com/blogs/production-tips/synthesizing-a-bass-from-scratch)
- Growl wavetables made by FM have a saw-like "fart" quality via Bend warp; layer with sub, modulate, maybe OTT — [Sonicstate](https://sonicstate.com/news/2020/04/06/100-free-serum-wavetables/)
- Sustance (Splice, Serum 2): stack extra oscillators **on octaves** for definition and impact — [Splice: Sustance tips](https://splice.com/blog/?p=37509)
- Creature's D&B masterclass sub: hand-drawn wavetables with FM and LFOs — [Loopcloud](https://loopcloud.com/genres/121-Music-Courses/products/15159-Keeping-Production-Simple-Advanced-D-B-Masterclass)
- Wilkinson (MusicTech, March 2026, *INFINITY*): a new plugin won't unlock the sound; still uses a Novation Bass Station (hardware) — [MusicTech](https://musictech.com/features/interviews/wilkinson-nirva-infinity-novation/)
- Camo & Krooked (Image-Line, c. 2013 — older): soft synths only; "the right amount of saturation/tube" makes soft synths sound like hardware — [Image-Line](https://www.image-line.com/artists/camo-krooked-reinhard-markus); now on Bitwig — [Bitwig](https://www.bitwig.com/artists/camo-krooked-88/)

### Inferences
- **Reese beat rate (calc):** beating = f × (2^(Δcents/1200) − 1). At A1 = 55 Hz, ±25 c (50 c spread) → ~1.6 Hz; ±61 c (122 c) → ~4.0 Hz; ±7 c (0.070 st, 14 c) → ~0.45 Hz. So "slow roll" reese ≈ 0.5–2 Hz beating at the root; MusicRadar's tempo-locking aims for ~1 beat per 1/4 or 1/8 note at 174 BPM (2.9 / 5.8 Hz).
- **Engine mapping for an "EDM/Dubstep Bass" foghorn/jump-up variant (estimate):** 3 saws at 0 / +7 / +19 st with the top one loudest, 1–3 unison voices each, light FM amount envelope; this is the single most "current" DnB-crossover bass sound (2023–2025 charts).
- **Growl/neuro mapping (estimate):** FM ratio 2:1–8:1 (modulator +1 to +3 oct), FM index swept 20–50% by LFO; this is a better Dubstep Bass source than a plain detuned saw.

### Gaps
- No first-hand patch values from Chase & Status, Andy C, Sub Focus, Dimension, Hedex, Bou, Culture Shock, Noisia or Camo & Krooked were found. Andy C search results were career profiles only (e.g. [dropthebass.co](https://origin.dropthebass.co/artists/andy-c)).
- Hedex/Bou jump-up "wub" patch values: no source found.

## Clean sub vs dirty mids split: crossover, sub waveform, how much

### Takeaway
Universal practice: a clean **sine (or low-passed sine/triangle) sub** below roughly **80–120 Hz**, with the distorted mid bass high-passed around **100–150 Hz**; the mid bass lives roughly **120–800 Hz** with harmonics above. No authoritative source gives a single crossover number.

### Cited Findings
- Sub = low-passed sine/square with cutoff ~**80–90 Hz**; producer high-passes the synth at ~**120 Hz**, splits it into 3 bands, and runs the sub low-passed at ~**60–70 Hz**; others layer a second distorted sub high-passed ~**100 Hz** behind the main bass — [dnbforum: Do you just use sub bass & mid range synth?](https://dnbforum.com/threads/do-you-just-use-sub-bass-mid-range-synth.145928/)
- Useful reese range "more around **120 Hz–500 Hz**", roll off rather than hard-cut — [dnbforum: basslines and reese](https://dnbforum.com/threads/basslines-and-reese.79730/post-620070)
- Student DnB project low-passes a clean reese at ~**550 Hz** and multiband-ducks the sub under the kick — [Leeds Conservatoire](https://show.leedsconservatoire.ac.uk/shr4c004m002-25101066-composition-production/)
- DnB plugin guide: clean low end below **100 Hz**, mid bass **200–800 Hz** — [Output](https://output.com/blog/best-plugins-for-drum-and-bass)
- MusicTech C&S foghorn: sub oscillator under a **Low 24 at ~70 Hz** — [MusicTech](https://musictech.com/guides/essential-guide/chase-and-status-bass-garage-production-tutorial/)
- Attack "Baddadan": greasy mid-range reese sits alongside a **constant separate sub** — [Attack Magazine](https://www.attackmagazine.com/technique/synth-secrets/the-baddadan-synth-bass-sound/)
- Splice Serum 2 DnB pack: clean sine or triangle on one oscillator (or dedicated sub) keeps the low end stable under complex layers — [Splice](https://splice.com/sounds/packs/sample-tools-by-cr2/serum-2-drum-and-bass-essentials/story)
- Sustance: process bands separately; distortion and widening only from the mids up so low end doesn't get boomy — [Splice](https://splice.com/blog/?p=37509)
- Dubstep sub: sine layer for club translation, low cut ~**30 Hz** on the bass chain — [MusicRadar wobble bass](https://musicradar.com/how-to/lfo-wobble-bass) / [Dubspot](https://blog.dubspot.com/video-tutorial-making-a-dubstep-bass-in-ableton-live)

### Inferences
- **Engine target (estimate):** Sub Split crossover **~100 Hz (range 80–150)**, 24 dB/oct; sub = pure sine (optionally +small 2nd harmonic), mono, untouched by Grit; mids high-passed at the same point. Level: sub roughly equal in RMS to the mids (estimate; no source quantifies "how much"). Zero Engine's existing Sub Split (commit b6b1794) is consistent with this practice.
- High-pass 30 Hz on the whole bass for DC/subsonic hygiene.

### Gaps
- No source gives the sub-to-mid level ratio or a measured spectrum for a released DnB bass.

## Distortion chains, OTT, resampling, formant/comb filtering

### Takeaway
Chains are serial: **saturation/overdrive → OTT (often 100%) → more saturation → OTT/bus glue**, applied to the mid band only. Resampling (bounce, re-chop, re-process) is standard for neuro but poorly documented in sources found. Formant/comb material was thin.

### Cited Findings
- Noisia-style per-layer chain: Overdrive (drive **25%**) → OTT (amount **100%**); bus: Saturator (drive **+10 dB**) → another OTT — [Attack Magazine](https://www.attackmagazine.com/technique/synth-secrets/learn-how-to-make-noisia-inspired-synths-with-abletons-operator/)
- Letsynthesize Noisia tutorials (2026 roundup): multi-stage OTT, AM modulation, disperser; a midrange technique to "beef up the frequencies above the low end" — [Sonicstate: Noisia, Kings of the Midrange](https://sonicstate.com/news/2026/07/20/noisia-kings-of-the-midrange-/)
- Multiband distortion + OTT-type compressor as the neuro chain — [Blip Interactive forum](https://blipinteractive.co.uk/community/discussion/comment/11968)
- Resampling: bouncing layers through different time-stretch engines with pitch movement (forum, unanswered) — [Image-Line forum](https://forum.image-line.com/viewtopic.php?p=585189)
- Baddadan: EQ → Drum Bus → saturation → limiting — [Attack Magazine](https://www.attackmagazine.com/technique/synth-secrets/the-baddadan-synth-bass-sound/)
- Wobble chain: filter → compressor → distortion with drive modulated by the LFO — [MusicRadar: Serum effects on bass](https://musicradar.com/how-to/how-to-use-serums-effects-to-boost-a-bass-sound)
- Fixed formant filter over distorted sine/tri/saw approximates a growl with only volume modulation — [KVR forum](https://www.kvraudio.com/forum/viewtopic.php?p=7319839)
- High-pass + notch filter and LFO on FM amount as main shaping (Snail-style) — [WA Production](https://blog.waproduction.com/how-to-create-snails-filthy-bass)

### Inferences
- **Engine chain for Dubstep/neuro Bass (estimate):** mid band → waveshaper (drive ~+10–20 dB) → 3-band upward/downward compression at ~100% depth (OTT-like) → second softer saturation → formant/notch filter swept by LFO → recombine with clean sub. Grit macro = drive + OTT depth.
- Resampling can't happen in real time; an offline-rendered wavetable from a distorted FM patch is the engine-compatible equivalent (fits the ground rule "ML/textures rendered off the audio thread").

### Gaps
- Comb-filter delay times, formant vowel centres and OTT crossover points for DnB bass: no source found.

## Filter movement, LFO rates (170–175 and 140 BPM), envelope ms

### Takeaway
LFOs are tempo-synced: **1 bar to 1/32**, with **1/8 triplet** and 1/16 common for wobbles; jump-up "wub" uses slower 1/4–1/2 shapes per note (estimate), neuro fast 1/16–1/32 steps. Bass amp envelopes are near-gate (sustain 1.0, release ~20–30 ms); filter/FM envelopes give the attack "bark".

### Cited Findings
- Dubstep LFO rates 1/4, 1/8, 1/16, 1/32 synced; faster (1/16, 1/32) typical of "3 Fist" style — [Dubspot: Datsik 3 Fist](https://blog.dubspot.com/electronic-music-theory-analyzing-dubstep-basslines-datsiks-3-fist-style)
- Wobble set to **1/8 triplet** — [MusicRadar: LFO wobble bass](https://musicradar.com/how-to/lfo-wobble-bass)
- Serum wobble: LFO 1 rate **1 bar** to cutoff and Morph — [MusicRadar](https://musicradar.com/how-to/how-to-use-serums-effects-to-boost-a-bass-sound)
- Growl: HP12 filter, LFO in trigger mode, rate **1/2**, inverted mod — [BassGorilla super growl](https://bassgorilla.com/serum-tutorial-super-growl-bass-in-ableton-live/)
- Automate LFO rate so notes wobble at different speeds; draw custom shapes — [Violet Recording](https://violetrecording.com/how-to-make-a-wobble-bass/)
- Reese: amp sustain 1.00, release **24 ms**, mono — [Attack](https://www.attackmagazine.com/technique/tutorials/reese-bass-redux/)
- Foghorn: envelope sweeps wavetable position; Env 2 on sub filter cutoff; FM level brought in by envelope; retrigger mode — [MusicTech](https://musictech.com/guides/essential-guide/chase-and-status-bass-garage-production-tutorial/), [Attack](https://www.attackmagazine.com/technique/synth-secrets/the-baddadan-synth-bass-sound/)
- Neuro: mono with portamento — [Computer Music](https://www.pressreader.com/australia/computer-music/20160615/283201557570027)
- Sub Focus's usual tempo **174 BPM** — [MusicRadar via inkl](https://www.inkl.com/news/if-you-re-a-drum-n-bass-producer-you-just-become-familiar-with-the-varieties-of-amen-sub-focus-on-his-love-of-the-classic-break-and-how-he-uses-it)

### Inferences
- **Synced rates (calc):**
  - 174 BPM: 1 bar 0.73 Hz; 1/2 1.45 Hz; 1/4 2.90 Hz; 1/8 5.80 Hz; 1/8T 8.70 Hz; 1/16 11.6 Hz; 1/32 23.2 Hz.
  - 140 BPM: 1 bar 0.58 Hz; 1/2 1.17 Hz; 1/4 2.33 Hz; 1/8 4.67 Hz; 1/8T 7.0 Hz; 1/16 9.33 Hz; 1/32 18.7 Hz.
  - Half-time feel means a 174 BPM DnB wobble at 1/4 (2.9 Hz) sits close to a 140 BPM dubstep 1/8 (4.67 Hz) only loosely; the engine should sync to host tempo rather than fixed Hz.
- **Envelopes (estimate):** amp A 0–5 ms, S 1.0, R 20–60 ms; filter/FM env A 0–10 ms, D 80–250 ms (bark/"foghorn" swell on FM), glide 30–80 ms for neuro/reese. Motion macro = LFO depth + rate step.

### Gaps
- No artist-confirmed envelope ms values; no source for Hedex/Bou "wub" LFO shapes.

## Stereo: mono below which frequency, width of mids/highs

### Takeaway
Sub mono; width only on the mid/high band (Sustance; Splice). Mono-below-~100–150 Hz is convention but no authoritative source quantified it in this search.

### Cited Findings
- Width and distortion only from the mids up — [Splice: Sustance](https://splice.com/blog/?p=37509)
- Too much stereo spread in the low end causes mix problems — [eMastered](https://emastered.com/blog/reese-bass)
- Reese spread values: 65% with 2 voices (MusicRadar); 10% spread at 64 voices + width 200% (Attack Twin 3) — [MusicRadar](https://musicradar.com/how-to/classic-reese-bass-renegade-terrorist), [Attack](https://www.attackmagazine.com/technique/synth-secrets/fred-again-inspired-reese-basslines-with-twin-3/)
- Search summaries explicitly could not confirm a "mono below 100/120 Hz" source — [dnbforum](https://dnbforum.com/threads/do-you-just-use-sub-bass-mid-range-synth.145928/)

### Inferences
- **Engine target (estimate):** fully mono below the Sub Split crossover (~100 Hz); mid band (100 Hz–1 kHz) narrow-to-moderate (side share low); width mainly above ~1–2 kHz. Bass one-shot correlation should stay strongly positive (estimate > +0.5). Leads/stabs can be wide (supersaw unison spread).

### Gaps
- No measured side-share or correlation numbers for released DnB basses.

## Brightness/tilt, loudness (LUFS), transient character, register; leads and stabs

### Takeaway
Club DnB masters hit about **−4 to −6 LUFS** in drops (vendor-blog sources; integrated vs short-term ambiguous). DnB bass is a mid-range instrument (energy 120 Hz–4 kHz) with a separate sub; leads/stabs are supersaw stacks (7–9 voices, slight detune) with short punchy envelopes. No measured one-shot loudness data was found.

### Cited Findings
- DnB drops around **−4 to −5 LUFS integrated**; pushing to −2 causes artifacts — [Mastering The Mix](https://www.masteringthemix.com/blogs/learn/mixing-for-streaming-platforms-vs-clubs-key-differences); louder club tracks −4 to −6 — [Mastering The Mix](https://www.masteringthemix.com/blogs/learn/am-i-mastering-loud-enough-how-to-hit-the-perfect-lufs-every-time); but another MTM guide means **short-term** LUFS at the loudest part — [Mastering The Mix](https://www.masteringthemix.com/pages/how-loud-should-you-master) (conflict noted)
- No official club standard; −6 LUFS suggested as a high average — [Sweetwater](https://www.sweetwater.com/insync/loudness-standards-lufs-peaks-and-streaming-limits)
- Reese low-pass ~4 kHz (classic), ~550 Hz (smooth), 200 Hz (Attack classic version) — [MusicRadar](https://musicradar.com/how-to/classic-reese-bass-renegade-terrorist), [Leeds](https://show.leedsconservatoire.ac.uk/shr4c004m002-25101066-composition-production/), [Attack](https://www.attackmagazine.com/technique/synth-secrets/fred-again-inspired-reese-basslines-with-twin-3/)
- Foghorn adds harmonics at +7 and +19 st, i.e. strong energy 1.5× and 3× the root — [Attack](https://www.attackmagazine.com/technique/synth-secrets/the-baddadan-synth-bass-sound/)
- Supersaw lead (Syntorial, Spire): 3 saw oscs at **9-voice unison**, detune until "just audibly out of tune", two more oscs fine-tuned up/down, high-passed white noise layer, **12 dB high-pass** — [Syntorial](https://www.syntorial.com/tag/trance-lead/)
- Stabs: pitch-bent/gliding chord stabs via osc detune glide (Ableton Drift) — [Attack: pitch-bending chord stabs](https://www.attackmagazine.com/technique/synth-secrets/pitch-bending-chord-stabs-with-ableton-drift/)
- Sustance adds a little hall reverb inside the synth for glue — [Splice](https://splice.com/blog/?p=37509)
- Andy C's output characterised (profile, not first-hand) by "clear sub-bass foundation" — [dropthebass.co](https://origin.dropthebass.co/artists/andy-c)

### Inferences
- **Register (estimate):** DnB/dubstep bass notes mostly E1–G2 (~41–98 Hz fundamentals), keys F/F#/G common; sub stays below ~100 Hz; reese/foghorn often an octave up the keyboard with the sub doubling. Leads/stabs C4–C6.
- **One-shot loudness (estimate):** a finished bass one-shot that feeds a −5 LUFS drop would itself be roughly −6 to −9 LUFS short-term with a peak-to-loudness ratio ~6–9 dB (heavily compressed, near-flat envelope). Verify with `ZeroAnalyze analyze` on owner's references before using as tuning targets.
- **Transients (estimate):** bass = soft or gated onset with FM/filter "bark" in the first 50–150 ms; stabs = 0–2 ms attack, 150–400 ms decay; leads = sustained with moderate attack.
- **Transfer to EDM/Dubstep profiles:** Dubstep Bass ← neuro/growl chain (FM source, OTT, formant LFO at 140-synced rates, clean sine sub ~100 Hz). EDM Bass ← foghorn/jump-up (0/+7/+19 saws, retrigger, light FM, saturation + limiter). EDM/Dubstep Lead ← supersaw (9 voices, slight detune, HP12, noise layer, short reverb). Keep the clean-sub/dirty-mids split already in the engine.

### Gaps
- No measured LUFS/centroid for individual DnB bass one-shots; no artist-stated note ranges; nothing on Culture Shock, Dimension or Hedex specifically. Running `ZeroAnalyze` on owner-supplied or CC0 references is the reliable way to fill these.
