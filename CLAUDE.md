# Zero Engine

A VST3 instrument that produces finished, release-ready electronic sounds (EDM, dubstep, techno, tech-house, trap, synthwave, plus DnB, pop, house, trance, future bass, hardstyle) out of the box. Personal studio tool, not for sale. UI: genre + type selectors, six macro knobs (Weight, Grit, Punch, Width, Motion, Air), a display (live wavetable / waveform / spectrum), an output meter, a Generate tesseract with history and favourites, and an Advanced page with every parameter.

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
- `ZeroAnalyze test`: every profile at several rates and block sizes, plus 4 Generate results per profile; fails on non-finite output, clipping, tails that don't die, allocations in `processBlock`, state or favourites that don't round-trip, Generate history, or a mod wheel that does nothing. Takes about 2–3 minutes. `bench`: CPU with 16 voices.
- `ZeroAnalyze gencheck`: Generate's time per click and how many of its 6 candidates pass the check, per profile.
- `ZeroAnalyze gendebug <Genre> <Type> <seed> [id=value...]`: one Generate result's key settings and its level and voice count each second through a note and its tail; overrides help find what holds a sound up. `segments <wav> [s]`: loudness and brightness per slice.
- `ZeroAnalyze harmonics <out.h> <5 wavs>`: harmonic spectra of recorded notes as a wavetable bank (made `Source/JupiterHarmonics.h`).
- `ZeroAnalyze snapshot <png> [size=WxH] [button...] [dialog]`: the editor as an image, after playing a chord; clicks buttons by label (e.g. `ADVANCED MOTION`). Send the owner a preview after GUI changes. `foldstrip <png>`: the tesseract's fold, frame by frame.
- `ZeroAnalyze screen` / `fetch`: Freesound CC0 search and download (needs the owner's `FREESOUND_CLIENT_SECRET` / `FREESOUND_CLIENT_ID`).

Output goes to `analysis/` by default (git-ignored).

## Layout

- `Source/Parameters.h`: parameter IDs and layout (APVTS)
- `Source/Profiles.*`: the 72 designed profiles (type template, genre shift, signatures, `applyResearchedGaps`), `generateProfile` for Generate
- `Source/SynthVoice.*`: voice DSP (wavetables, unison, filter, envelopes, tempo-synced LFO)
- `Source/Wavetable.*`: wavetable banks, including the Jupiter bank from `JupiterHarmonics.h`
- `Source/FinishingChain.*`: saturation, OTT, EQ, width, spectral stage, clipper, meters
- `Source/SpectralCompressor.*`: our own many-band (256-bin STFT) up/down compressor, level-neutral, leaves < 150 Hz alone; adds 512 samples of latency
- `Source/MacroMap.h`: what each macro pushes per profile; the mod wheel adds to Motion
- `Source/PluginProcessor.*`: synth, finishing chain, state, Generate (render-and-check), history, favourites (`%APPDATA%/Zero Engine/Favourites`)
- `Source/PluginEditor.*`: UI (look and feel, display, meter, tesseract, Advanced page); laid out at 900 x 600 and scaled with the window
- `Resources/Fonts/`: Barlow Condensed and Share Tech Mono (OFL), from the owner's Potato Kicker

## Ground rules

- No cloud, no subscription, no logins. Everything runs locally inside the plugin.
- Tone comes from a real DSP engine; ML only chooses parameters, or renders transients/textures off the audio thread.
- Audio thread is real-time safe: no allocation, locks, file I/O or model inference in `processBlock`/`renderNextBlock`.
- Training data: the owner's sounds, audio rendered from this engine, and other sources whose licence permits it. Check terms before use; Splice, Loopmasters and NI licences forbid AI training, personal use included.
- Git: commit locally; push to `github.com/basspotato/zero-engine` only when the owner asks.
- Keep GPL code (Matchering, DawDreamer, Surge, Vital) out of the shipped plugin; offline tooling only.
