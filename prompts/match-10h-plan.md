# Reference Match: 10-hour run (started 2026-10-28, about 22:15)

Owner's goal: 45-50 % of their real sounds CLOSE or better, or Match gets cut
or shrunk to "closest preset". Owner approved (2026-10-28): run rounds for the
full 10 hours (no 8-round cap), ML and GPL code allowed, downloads of Microsoft
ONNX Runtime and Spotify basic-pitch (official GitHub releases), keep-awake.

## Each round
- One change. Build with `cmake --build build --config Release --target ZeroAnalyze`
  (`build2/` is a second copy for diagnostics while a bench holds build/).
- Score: `build/ZeroAnalyze_artefacts/Release/ZeroAnalyze.exe libbench 60 42 source=owner-library,jupiter,owner > analysis/library/roundN_<name>.txt`
  in the background (about 20 min; frozen judge).
- Keep if CLOSE-or-better rises, or the means clearly improve without losing
  CLOSE sounds; else revert. Commit locally (never push), log in
  `analysis/match_log.md`.
- Renew keep-awake (`request_keep_awake`, session_idle) each turn.

## Queue
1. Round 4: 3x search budget (`budget=240 second=180`): is the search the limit?
2. Round 5: render at the reference's tempo (file name, else the project's);
   wobble over the whole phrase when the first note is short; wobble snapped
   to a synced division (built, uncommitted).
3. Round 6: onsets of low notes the flux misses (bass under an arp:
   WW_HDNB_175_Arp_Synth_Angelo_Fm): flux weighted per band / a low-band flux.
4. Rounds 7-8: Basic Pitch transcription (see research_notes/match_scan_options.md):
   official onnxruntime.dll loaded at runtime + Spotify's nmp.onnx + a port of
   NeuralNote v1.1.0 `Lib/Model/Notes.cpp`. Third-party files in a git-ignored
   `third_party/` folder; licences noted.
5. Rounds 9-11: neural starting points: dataset of engine renders (profile ->
   print features), a small MLP trained in C++ (no Python here), its
   predictions added as extra search starts.
6. If time: two-layer matching (lead + bass loops).
7. End: `ZeroAnalyze test`, CLAUDE.md update, verdict and report for the
   owner; preset retune from packs as a dry run only (owner reviews before
   any write).
