# Zero Engine

A VST3 instrument that produces finished, release-ready electronic sounds (EDM, dubstep, techno, tech-house, trap, synthwave, plus DnB, pop, house, trance, future bass, hardstyle) out of the box. Personal studio tool, not for sale. UI: genre + type selectors, six macro knobs (Weight, Grit, Punch, Width, Motion, Air), a display (live wavetable / waveform / spectrum), an output meter, a Generate tesseract with history and favourites (plus a mini tesseract in the display for any genre, and a heart that teaches Generate your taste), and an Advanced page with every parameter.

Background and decisions: `reports/AI finished sound VST engine.md` (original research), `reports/Artist sound design targets.md` (per genre/type targets the profiles are built from), `Build notes.md` (what the owner wants to use), `research_notes/` (sources).

## Build (Windows)

```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target ZeroEngine_VST3 ZeroEngine_Standalone
```

JUCE 8 is fetched by CMake. Outputs land in `build/ZeroEngine_artefacts/Release/` (`VST3/Zero Engine.vst3`, `Standalone/Zero Engine.exe`). A running standalone locks the `.exe`, so close it before rebuilding. `dist/` holds a prebuilt VST3 for testing on other machines; GitHub Actions (`.github/workflows/build.yml`) also builds it on every push.

## Analysis tool (offline, not shipped)

`tools/analyze/` builds `ZeroAnalyze.exe` (target `ZeroAnalyze`, output in `build/ZeroAnalyze_artefacts/Release/`). It measures loudness (BS.1770 LUFS, peak-to-loudness ratio), brightness (centroid, % above 4 kHz), bass (% below 150/60 Hz) and width (side share, correlation).

- `ZeroAnalyze analyze <folder> [out.csv]`: measure any folder of audio.
- `ZeroAnalyze render [outDir]`: render all 72 profiles to WAV and measure them.
- `ZeroAnalyze compare <refFolder> [outDir]`: measure references laid out as `<Genre>/<Type>/*.wav`, render the matching profiles, print ref vs engine.
- `ZeroAnalyze tune <refFolder> [--write] [--tone]`: for each `<Genre>/<Type>` group with 3+ references, adjust level trim, OTT depth, tilt, Weight and stereo width until the render matches the references; `--write` saves to `Source/ProfileTuning.h`. `--tone` moves tilt only (±2 dB), for dry references such as the owner's Jupiter-8 recordings. `tunetest` checks the tuner. Freesound references proved too mixed to write wholesale; review every dry run with the owner.
- `ZeroAnalyze test`: every profile at several rates and block sizes, plus 4 Generate results per profile and four Reference Match cases (a plucked lead, a 5 Hz filter wobble, a dark pad, a pluck loop; the wobble case is an expected failure, reported but not counted); fails on non-finite output, clipping, tails that don't die, allocations in `processBlock`, state or favourites that don't round-trip, Generate history, or a mod wheel that does nothing. Takes about 5 minutes. `bench`: CPU with 16 voices.
- `ZeroAnalyze gencheck`: Generate's time per click and how many of its 24 candidates pass the check, per profile. `gendebug2 <Genre> <Type>`: every candidate's scores for two clicks. `genab [dir] [pairs]`: a blind A/B pack (plain variation vs Generate's pick, x/y in random order, key.csv) plus any-genre renders. `tastesim`: a simulated listener checks the taste model learns (temporary folder only). `library traits [source=] [--write]`: SoundTraits of library sounds per type; `--write` saves Generate's targets to `Source/GenerateTargets.h` (measurement only).
- `ZeroAnalyze gendebug <Genre> <Type> <seed> [id=value...]`: one Generate result's key settings and its level and voice count each second through a note and its tail; overrides help find what holds a sound up. `segments <wav> [s]`: loudness and brightness per slice.
- `ZeroAnalyze match <wav> <Genre> <Type> [-v]` or `match <folder> [-v]` (laid out `<Genre>/<Type>/*.wav`): runs Reference Match as the plugin does (it searches every genre/type; the one given is only where the plugin "is") and prints the closeness word and what it chose. `-v`: the reference's transcription, the starting-preset ranking, each worker's progress, the finalists and the loss term by term, and the result again with OTT, reverb/delay or the spectral stage off. `notes <wav>`: just the transcription.
- `ZeroAnalyze matchbench [refFolder] [budget=N] [cmaes=1] [light=0] [w=mel,mfcc,centroid,loudness]`: scores Reference Match on 12 engine-made targets (notes and arp loops): the match replays the target's notes and is judged by log-mel dB difference and MFCC timbre. Writes targets and matches to `%TEMP%/zero_matchbench`; with refFolder, matches the owner's references there and writes what it found to `analysis/match_v2`. Use it to judge any change to SoundPrint or ReferenceMatch.
- `ZeroAnalyze noise <Genre> <Type>|all [id=value...]`: noise between the harmonics of a held C4 (dB under them), to find stages that add hiss.
- `ZeroAnalyze harmonics <out.h> <5 wavs>`: harmonic spectra of recorded notes as a wavetable bank (made `Source/JupiterHarmonics.h`).
- `ZeroAnalyze snapshot <png> [size=WxH] [button...] [dialog]`: the editor as an image, after playing a chord; clicks buttons by label (e.g. `THE WEEDS` then a tab name). Send the owner a preview after GUI changes. `foldstrip <png>`: the tesseract's fold, frame by frame.
- `ZeroAnalyze screen` / `fetch`: Freesound CC0 search and download (needs the owner's `FREESOUND_CLIENT_SECRET` / `FREESOUND_CLIENT_ID`).

Output goes to `analysis/` by default (git-ignored).

## Layout

- `Source/Parameters.h`: parameter IDs and layout (APVTS)
- `Source/Profiles.*`: the 72 designed profiles (type template, genre shift, signatures, `applyResearchedGaps`), `generateProfile` for Generate
- `Source/SynthVoice.*`, `Source/Envelope.h`: voice DSP (wavetables, unison, filter, tempo-synced LFO); analog-style exponential envelopes (fall stages stretched 1.9x so presets kept their length when they replaced the linear ones)
- `Source/Wavetable.*`: wavetable banks, including the Jupiter bank from `JupiterHarmonics.h`
- `Source/FinishingChain.*`: saturation, OTT, EQ, width, spectral stage, clipper, meters
- `Source/SpectralCompressor.*`: our own many-band (512-point STFT) up/down compressor, level-neutral, leaves < 150 Hz alone, lifts only bins within 30 dB of the loudest and never the gaps between harmonics, gains smoothed over 9 bins; adds 512 samples of latency
- `Source/MacroMap.h`: what each macro pushes per profile; the mod wheel adds to Motion
- `Source/Arpeggiator.*`: tempo-synced arp with a 16-step pattern that shapes every mode (rests, transpose, accent); mode "Sequence" plays all held keys per step. Runs on the MIDI before the synth; its settings are not part of profiles
- `Source/PluginProcessor.*`: synth, finishing chain, state, Generate, history, favourites (`%APPDATA%/Zero Engine/Favourites`). Generate: 24 variations (some bred from hearted sounds) measured on 4 private engines in parallel; kept if still in the genre/type's character; scored per trait (width, body, density, movement, richness) against the owner's sounds as a soft capped guide, plus novelty and the learned taste; softmax pick, then pushed twice towards a higher score. Any genre (mini tesseract): candidates from every genre and two-genre blends, loaded as the leading genre. Hearts and taste judgments (kept = played 6 s+, skipped = clicked past in 4 s without a note) in `%APPDATA%/Zero Engine/Hearts.xml` and `Taste.xml`
- `Source/SoundTraits.*`, `Source/GenerateTargets.h`, `Source/TasteModel.*`: what Generate measures, its per-type targets (from 1,187 of the owner's sounds), and the taste model (Bradley-Terry on traits, Laplace uncertainty, Thompson sampling)
- `Source/MatchNet.*`, `tools/analyze/MatchNetTrain.*`: neural-net scaffolding (features, MLP, C++ trainer: `netdata`, `nettrain`); not trained yet
- `Source/SoundPrint.*`, `Source/ReferenceMatch.*`, `Source/Cmaes.h`: Reference Match. Fingerprint = log-mel spectrograms over the whole phrase (two resolutions), compared as a composite loss (mel + MFCC + centroid + loudness per frame + width); a reference is transcribed into notes (spectral-flux onsets, pitch from each note's arrived energy; echoes, ghost notes and held wobbles merged) and candidates replay those notes with a searched hold (gate). Search: every preset at the reference's pitch (FX only for unpitched sounds), the closest 6 refined in parallel on a low-priority pool (octave, wavetable, delay time, sub layer, wobble seed, pattern search over 23 settings), the 2 best get a second round; search renders are light (no spectral stage or oversampling), finalists re-scored at full quality (spectral also tried off). Can change genre/type. About 20 s. Our CMA-ES is optional (lost to the pattern search at this budget). No ML.
- `Source/PluginEditor.*`: UI (look and feel, display, meter, tesseract, Advanced page); laid out at 900 x 600 and scaled with the window
- `Resources/Fonts/`: Barlow Condensed and Share Tech Mono (OFL), from the owner's Potato Kicker

## Ground rules

- No cloud, no subscription, no logins. Everything runs locally inside the plugin.
- Tone comes from a real DSP engine. Since 2026-10-28 the owner allows ML anywhere it helps (e.g. Basic Pitch transcription, learned starting points for Match), and GPL code in the plugin, as it is a personal tool (GPL code means the repo and any build shared from it fall under the GPL).
- Audio thread is real-time safe: no allocation, locks, file I/O or model inference in `processBlock`/`renderNextBlock`.
- Training data: the owner's sounds, audio rendered from this engine, and other sources whose licence permits it. Check terms before use; Splice, Loopmasters and NI licences forbid AI training, personal use included.
- Git: commit locally; push to `github.com/basspotato/zero-engine` only when the owner asks.
- GPL code (Matchering, DawDreamer, Surge, Vital) is allowed (see above); note where it is used. Model and dataset licences still apply to training data.
