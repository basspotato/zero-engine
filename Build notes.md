# Build notes: finished-sound VST engine

Points pulled from the research report ([AI finished sound VST engine](reports/AI%20finished%20sound%20VST%20engine.md)) to carry into the build.

## Product principles
- **No cloud, no subscription, all in the plugin, no bullshit.** Everything runs locally; no logins, no credits, no online caps.
- **Workflow inspired by Unisynth:** pick a genre, then a sound type (chord, lead, bass, etc.), then generate. Keep that simplicity.
- **Generation mode is open:** generate-then-play, real-time note-on synthesis, or a hybrid of both.
- **Focus:** big, fun electronic sounds (EDM, dubstep, techno, tech-house, trap, synthwave). Mixing/mastering research should still cover all genres, pop included, to understand "finished" broadly.
- **Training data:** my own original, high-quality, fully processed audio samples, combined with the research methods and other learning approaches.
- **Starting point:** the recommended architecture and roadmap from the report. Start on the engine.

## Core research to build on
1. **Generative synth inversion:** flow-matching models that propose editable patches from a target sound.
   - Key paper: Hayes, Saitis and Fazekas, permutation-equivariant flow matching (ISMIR 2025 best student paper).
2. **Audio-effects style transfer:** models that predict or search the settings of a mastering-style DSP chain from a style reference (DeepAFx-ST, ST-ITO, ITO-Master, Diff-MST).
3. **Style is genre/role, not individual engineer:** Wilson and Fazenda (Salford) analyzed 1,501 mixes.
   - Mix variation reduces to four dimensions: **loudness/dynamics, brightness, bass, width**.
   - The engineer explained very little of listener preference (eta² = 0.021).
   - So: model style profiles by genre and sound role on these dimensions.

## Unproductized methods worth using
- **Modulation discovery:** recovers the envelope and LFO curves behind a sound, tested on Vital preset modulation (arXiv 2510.06204). Covers the "motion" that makes EDM basses feel alive.
- **Differentiable wavetable synthesis:** learns compact dictionaries of 10–20 wavetables that export straight into a normal wavetable oscillator (arXiv 2111.10003).
- **Neural proxies:** map black-box VST presets into a pretrained audio-embedding space, making them differentiable for matching (arXiv 2509.07635; GitHub).
- **FM-SynAPSE:** joint audio/preset embeddings for retrieval (arXiv 2608.18226).
- **CTAG:** optimizes an inspectable synth patch toward a CLAP text target with evolutionary search (arXiv 2406.00294).
- **PyTorch to FAUST compiler (DAFx 2026):** turns differentiable graphs into FAUST DSP and hot-reloads into a plugin while training runs (arXiv 2606.21277).
- **Effects-side toolkits:**
  - dasp-pytorch (Apache-2.0): differentiable EQ, compression, distortion, reverb, stereo.
  - NablAFx: gray-box effect chains (arXiv 2502.11668).
  - GRAFX: batched processing graphs (arXiv 2408.03204).

## Engine decisions
- **Real DSP engine for tone.** For clean, phase-coherent, alias-free bass and lead tones, only a real DSP engine reaches commercial quality.
- **Neural generation in a supporting role:** transients, noise layers and textures, rendered into buffers off the audio thread.
- **Novel R&D the product will own:** nobody has validated differentiable models of these against EDM presets yet:
  - supersaw unison stacks
  - OTT-style multiband upward/downward compression

## Sound design principles
- **Clean sub, dirty mids.** One clean mono sine owns everything below ~100–150 Hz and skips the distortion and OTT; the movement and grit live in a high-passed mid layer (~150 Hz to 3.5 kHz). Applies to every bass profile (dubstep first), and the other half applies to leads, chords, pads and plucks: high-pass them so nothing fights the sub. Research, the engine gap and a per-profile table: [bass clarity](research_notes/bass_clarity.md).

## Open items
- Generation mode decision (generate vs. note-on vs. hybrid).
- Still to verify from the report: Unisynth's current features, licenses for Diff-MST / ST-ITO / synth-proxy / CTAG / model weights.
- Retune the genre profiles from the owner's new sample packs (C:\samples, C:\Production 2024, D:\Cubase Production\Production 2024; owner OK'd, 2026-10-28): map packs to genres by pack name, `ZeroAnalyze tune` dry run, review the per-genre before/after with the owner, then write. Measurement only, no training on pack audio. After the Reference Match rounds.
  - Done 2026-10-29, not written: sorted by pack name, 490 sounds in 28 genre/type groups; about half were loops, build-ups or full mixes and the tuner hit its limits (+12 dB, +-6 dB tilt, full OTT, 2x width). One-shots only (80, 9 groups) still asked for +5-12 dB and up to +6 dB tilt (packs are mastered hot), and the cleanest group (House Pluck, 13) was mostly vocal chops, FX and loops under a Pluck label. The library's type labels are too loose for these packs; the presets stay as designed. To revisit: hand-picked one-shots per genre/type (3+ each), `tune --tone` first.
- Neural Generate (owner's focus from 2026-10-28, Match rounds stopped): see [Synplant 2 Genopatch method](reports/Synplant%202%20Genopatch%20method.md). First step: a judge net that ranks many light-rendered Generate candidates by genre/type fit, then the existing full check on the best few (PhenoType's approach); training data = engine renders, the 72 profiles, favourites and the owner's keep/skip, never commercial packs.
- Generate build plan (2026-10-28): see [Generative synth preset algorithms](reports/Generative%20synth%20preset%20algorithms.md). Done: SoundTraits scoring, push step, any-genre mini tesseract, hearts + taste model (Bradley-Terry, Thompson sampling, stages 1 and 6). Next: harder score (width 200 Hz-2 kHz with a mono check, harmonic-only movement, harshness penalty, falling past the cap), octave check, a 96-candidate screen, a per-type archive of distinct sounds; then the neural judge.
- Overnight 2026-10-29: Generate's tuning gates (pitch tracked through the note, wobble and noise vs the designed sound), a calmer arp (plucks/leads/basses only, octaves and fifths), an octave check, and the neural judge (GenerateNet, trained on 100k engine renders, screens 400 candidates to the 24 rendered: picks +0.84 vs +0.49, more varied). Next: harmonic-only width and movement in the score (research: newest_methods.md), the per-type archive, and fine-tuning the judge on the owner's hearts once there are enough.
- Owner's blind A/B (2026-10-29, 24 pairs): new Generate won 9, plain variations 5, 10 "both good". Moods: lush pads and warm keys good; glassy bells "too much / harsh on top", an airy drone harsh, one warm keys with distorted mids. Bells and drones made darker (filter, air, tilt), keys cleaner (grit, OTT, spectral), and a harshness penalty in the mood score.
- A/B round 2 (2026-10-29, 12 pairs, 24 moods, 8 Mutate): new Generate 5, plain 6, 1 tie (no longer ahead); the Techno Chord pick "too distorted". Moods all ok except SOARING LEAD 8 ("too loud, hurts ears") and LUSH PAD 18 ("too loud, distorted"): they measured -6.5 and -4 LUFS with 6.5 and 4 dB PLR, crushed into the clipper, and the size score rewarded density without a floor. Mutate: "hard to hear any difference" (b preferred twice of 8). Fixed: a crushed penalty in both scores (denser than the owner's type median less 1.5 spreads costs, steeply), a loudness penalty for moods (over 2 dB above the designed type), and Mutate makes two big audible moves (wavetable, table position, filter by an octave or more, envelope, unison, LFO, space) plus four small nudges, aiming at 1.1 from the sound (re-render: crushed moods 6 -> 3, the worst -5.4 LUFS was the deep bass the owner liked; mutations 0.97-1.93 from their sound, was about 0.75).
  - Mutate re-listen (generate_ab3): b preferred 3 of 8 (1, 3, 6), a 4, one unsure; no "can't hear it" this time. A mutation that wins about 3 in 7 against a sound Generate already chose is a useful variation.
  - Still "too subtle" for the owner, so Mutate got the looser fit bounds (Genre mode's rejected the audible changes), three big moves and an aim of 1.5. Re-listen (generate_ab4): "all the b's sound better", 8 of 8.
- A/B round 3 (2026-10-29, after the crushed fix, own-project targets and the retrained judge): genre pairs 6-6 (new won 3, 4, 5, 6, 10, 11); all 24 moods "sound good" (round 2: two too loud); Mutate fine on 5 of 8, "a/b sound the same" on both basses and the pad: their moves were ones a mono bass or a washy pad hides. Mutate now picks moves by type (bass: table, position, filter, wobble, drive; pad: no envelope or space) and has a drive move (grit and OTT together).
  - Re-listen (generate_ab_round3b, pairs 1, 4, 7): "sounds better".
- A/B round 4 (2026-10-30, width at half weight, no movement reward): genre pairs new 6, plain 4, 2 ties (new ahead again). Moods: 19 of 24 good or ok; soaring leads "weak, nothing like a soaring lead", bells lost their glass, lush pad wants more top, deep bass "like a regular bass", warm keys panned left and wanting 80-300 Hz weight. The lean was the delay: a ping-pong fed only the left line, so every first echo came from the left; now each side enters its own cross-fed line. The mood score pulled every mood darker; warmth now counts only for warm moods, soaring lead is scored on size, and the recipes moved (lead bigger, brighter, legato glide; bells, pads brighter; deep bass lower with the sub on; keys more weight). Mutate: 6 of 8 good, pad and pluck too close: they get four moves. Genre A/B rounds end here (taste learning takes over); one short last round on these fixes (analysis/generate_ab_final).
  - Last round: all 10 fixed moods "good"; Mutate pluck good, the Future Bass pad still "same" (that pad is squashed, -6.8 LUFS, PLR 6.8, so OTT and the clipper flatten changes). Pads now always change wavetable, and any mutation under 0.8 from its sound costs steeply.
- 2026-10-30: the owner heard the eased pads and chords (Input Trim) and the centred delay against the old ping-pong: both "sound good"; the centred delay stays the only mode.
- 2026-10-30 taste session: 165 hearts, 400 judgments in each model (the cap). Learned: darker, less bass share, more dynamic, calmer, simpler; Analog/Organ/Choir tables, filter and mod envelopes; away from synced LFOs, long sustain, reverb, delay, open filters, many unison voices, House and Trance. The owner wants variety too (big reverbed layers, not all dark): taste capped at 1 and a third of clicks free of it. House Bass was "peak time leads": now a round near-sine design, its variations kept round (every candidate), and Genre mode breeds only from hearts of its own genre. Pops: Saw, Square and S&H LFOs jumped in one sample; 3 ms of smoothing (clickcheck: 1 click in 216 picks before, 0 after). Voices taken over while sounding now carry on smoothly. The owner re-tested arp and LFO patches: "clean now".
- macOS build (planned 2026-10-31, waiting on the owner's testing first): Apple Silicon only (arm64), formats AU, VST3 and Standalone. A macOS job in `.github/workflows/build.yml`, run on demand if the repo is private (macOS minutes cost 10x there); plugin checks with pluginval and `auval -v aumu Zen1 Zeng`. CMake: add AU on macOS, `CMAKE_OSX_ARCHITECTURES arm64`, a Mac stack option for ZeroAnalyze (or skip the tool). Nothing signed: copy by USB or share, or clear the download flag with `xattr -dr com.apple.quarantine`. The owner tests on their MacBook and Mac tower (both assumed Apple Silicon; check). The macOS 60 Hz space screen's drawing speed is the main unknown.
