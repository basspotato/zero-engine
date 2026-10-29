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
- Neural Generate (owner's focus from 2026-10-28, Match rounds stopped): see [Synplant 2 Genopatch method](reports/Synplant%202%20Genopatch%20method.md). First step: a judge net that ranks many light-rendered Generate candidates by genre/type fit, then the existing full check on the best few (PhenoType's approach); training data = engine renders, the 72 profiles, favourites and the owner's keep/skip, never commercial packs.
- Generate build plan (2026-10-28): see [Generative synth preset algorithms](reports/Generative%20synth%20preset%20algorithms.md). Done: SoundTraits scoring, push step, any-genre mini tesseract, hearts + taste model (Bradley-Terry, Thompson sampling, stages 1 and 6). Next: harder score (width 200 Hz-2 kHz with a mono check, harmonic-only movement, harshness penalty, falling past the cap), octave check, a 96-candidate screen, a per-type archive of distinct sounds; then the neural judge.
- Overnight 2026-10-29: Generate's tuning gates (pitch tracked through the note, wobble and noise vs the designed sound), a calmer arp (plucks/leads/basses only, octaves and fifths), an octave check, and the neural judge (GenerateNet, trained on 100k engine renders, screens 400 candidates to the 24 rendered: picks +0.84 vs +0.49, more varied). Next: harmonic-only width and movement in the score (research: newest_methods.md), the per-type archive, and fine-tuning the judge on the owner's hearts once there are enough.
