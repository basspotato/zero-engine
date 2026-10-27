# Zero Engine

A VST3 instrument that produces finished, release-ready electronic sounds (EDM, dubstep, techno, tech-house, trap, synthwave) out of the box. Simple UI: genre + type selectors, six macro knobs (Weight, Grit, Punch, Width, Motion, Air), a display, output.

Background and decisions: `reports/AI finished sound VST engine.md` (research), `Build notes.md` (what the owner wants to use), `research_notes/` (sources).

## Build (Windows)

```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target ZeroEngine_VST3 ZeroEngine_Standalone
```

JUCE 8 is fetched by CMake. Outputs land in `build/ZeroEngine_artefacts/Release/` (`VST3/Zero Engine.vst3`, `Standalone/Zero Engine.exe`).

## Analysis tool (offline, not shipped)

`tools/analyze/` builds `ZeroAnalyze.exe` (target `ZeroAnalyze`, output in `build/ZeroAnalyze_artefacts/Release/`). It measures loudness (BS.1770 LUFS, peak-to-loudness ratio), brightness (centroid, % above 4 kHz), bass (% below 150/60 Hz) and width (side share, correlation).

- `ZeroAnalyze analyze <folder> [out.csv]`: measure any folder of audio.
- `ZeroAnalyze render [outDir]`: render all 72 profiles to WAV and measure them.
- `ZeroAnalyze compare <refFolder> [outDir]`: measure references laid out as `<Genre>/<Type>/*.wav`, render the matching profiles, print ref vs engine.

- `ZeroAnalyze screen [shortlist.csv] [perPair]`: search Freesound for CC0 sounds per genre/type (needs `FREESOUND_CLIENT_SECRET`).
- `ZeroAnalyze fetch [shortlist.csv] [outDir] [max]`: download the shortlist at full quality after browser approval (needs `FREESOUND_CLIENT_ID` too), with `credits.csv`. CC0 only. The owner runs these with their own credentials.

Output goes to `analysis/` by default (git-ignored).

## Layout

- `Source/Parameters.h`: parameter IDs and layout (APVTS)
- `Source/SynthVoice.*`: voice DSP
- `Source/PluginProcessor.*`: synth, finishing chain, state
- `Source/PluginEditor.*`: UI

## Ground rules

- No cloud, no subscription, no logins. Everything runs locally inside the plugin.
- Tone comes from a real DSP engine; ML only chooses parameters, or renders transients/textures off the audio thread.
- Audio thread is real-time safe: no allocation, locks, file I/O or model inference in `processBlock`/`renderNextBlock`.
- Training data: the owner's sounds, audio rendered from this engine, and other sources whose licence permits it. Check terms before use; Splice, Loopmasters and NI licences forbid AI training.
- Git: commit locally; don't push unless the owner asks.
- Keep GPL code (Matchering, DawDreamer, Surge, Vital) out of the shipped plugin; offline tooling only.
