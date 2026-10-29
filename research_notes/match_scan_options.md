# Reference Match: scan and reproduce options (owner's research, 2026-10-28)

Two notes from the owner: "Scanning Audio to Reproduce Sounds in a VST3"
(a Claude Doc) and a survey of open-source tools. What applies to Zero
Engine, given the ground rules (no cloud, no GPL/AGPL in the plugin, ML only
picks parameters or works off the audio thread, licences checked).

## Already what Match does
- Render-in-the-loop parameter matching against our own engine, loss built
  from mel-STFT, MFCC and centroid: the Instrumental paper's template
  (arXiv 2603.15905). Their findings hold for us: start from analysis, not at
  random (we start from the closest presets); CMA-ES beat gradient descent
  there, but our pattern search beat our CMA-ES at our budget (CMA-ES kept
  as an option, `ReferenceMatch::useCmaes`).
- A small parameter subset (oscillator, filter, amp/filter envelopes, then
  FX): Instrumental found more parameters didn't reliably help.

## Worth doing (in order)
1. **Wavetable from the reference** (owner approved, 2026-10-28): cut the
   reference's harmonics into a "Sample" wavetable bank the search can pick,
   then fit filter, envelopes and FX as now: the hybrid of resynthesis and
   parameter matching. Vital's audio-to-wavetable import is the model (GPL:
   study only, write our own).
2. **Better note transcription**: transcription errors are a top cause of
   misses on loops. Spotify Basic Pitch (Apache-2.0, polyphonic, pitch bends)
   is the candidate; NeuralNote (Apache-2.0) embeds it in a JUCE plugin with
   RTNeural and ONNX Runtime and is the C++ reference. Off the audio thread,
   it only decides which notes to replay, so it fits "ML picks parameters".
   Needs: the model file's licence checked, the runtime added to the build.
3. **Separation before transcription** (Demucs, MIT): helps on full mixes.
   Match is for one-instrument sounds, so only if mixed loops matter later.
4. **Learned starting points** (flow-matching inversion, Hayes et al.,
   ISMIR 2025; Synplant 2's approach): a small net trained on renders of our
   own engine proposes patches, the search refines them. Training data: our
   renders only. Inference via anira, RTNeural or ONNX Runtime on a worker
   thread (Torsion Audio's nn-inference-template shows the threading).

## Off the table
- Direct resynthesis (Loris, Paraphrasis, sms-tools, PartialFKR, SAMPLE):
  plays back the sample rather than giving a Zero Engine patch.
- RAVE: not knob-driven; public models CC-BY-NC (our own training would be
  fine, but it replaces the engine rather than driving it).
- Essentia (AGPL), aubio (GPL), Vital and Surge (GPLv3) code: never in the
  plugin; study or offline tooling only.
- Python hosting of the plugin (Pedalboard, DawDreamer GPL): we render
  in-process already.
- Closed "AI preset matchers" (MicroMusic for Vital): users found it shuffles
  a hidden preset's parameters.

## Fine for offline analysis
librosa (ISC), FluCoMa (BSD: NMF, HPSS, transient separation, descriptors),
sms-tools: prototyping only, nothing shipped.

## Basic Pitch in the plugin (researched 2026-10-28, nothing built yet)
- Licences: Basic Pitch code and weights Apache-2.0 (`basic_pitch/saved_models/icassp_2022/nmp.onnx`, 230 KB), NeuralNote Apache-2.0, RTNeural BSD-3, ONNX Runtime MIT.
- NeuralNote main (v2) moved to a transformer (MuScriptor, ggml, 210 MB+ downloads); the Basic Pitch version is tag v1.1.0: github.com/DamRsn/NeuralNote/tree/v1.1.0
- v1.1.0 layout: CQT front end in ONNX Runtime (`Lib/Model/Features.cpp`, `features_model.ort`), CNN in RTNeural (`Lib/Model/BasicPitchCNN.cpp`, 4 json models in `Lib/ModelData/`), note creation `Lib/Model/Notes.cpp` (frame threshold 0.3, onset 0.5, min note 125 ms, energy 11, MIDI 21-108). Input mono 22050 Hz.
- Catch: their prebuilt static ONNX Runtime links only with MSVC 19.35 (NeuralNote issue #21). Options: rebuild with ort-builder (needs Python), load Microsoft's onnxruntime.dll at runtime, run Spotify's full nmp.onnx in stock ORT and keep only Notes.cpp, or write the CQT in C++ (must match nnAudio).
- Expected weak spots on synths: octave errors on bright saws, weak sub-bass, notes under the minimum length dropped (fast arps need 35-60 ms), supersaws split into extra notes. Test on matchbench's engine targets first.
