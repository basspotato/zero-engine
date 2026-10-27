# Tooling, Frameworks, Datasets, and Real-Time ML for an "AI finished sound" VST3 Instrument (as of Oct 2026)

Method note: research was done with web search only (no full-page fetch tool was available), so many findings come from search-result summaries of the primary pages. Items marked **[PK]** are from the researcher's prior knowledge, pointed at the canonical repo or page, but **not re-verified this session**. The report writer should treat [PK] items as lower confidence and check the linked LICENSE file before stating them as fact.

## 1. Plugin frameworks: JUCE 8, iPlug2, VST3 SDK, CLAP, nih-plug, DPF; cross-platform builds, AU/AAX

### Takeaway
The big licensing change is confirmed: Steinberg relicensed the VST3 SDK under **MIT** with VST 3.8.0 (press release 29 Oct 2025). That makes permissive or closed-source VST3 frameworks much easier to use. **JUCE 8** is still the most practical choice for a commercial VST3/AU/AAX instrument with ML. It is dual-licensed AGPLv3 / commercial, and its free Starter tier covers up to $20k in annual revenue with no splash screen. Native CLAP authoring was on JUCE's roadmap for JUCE 9, but no shipped release was found. **nih-plug** (Rust) is a strong option for VST3+CLAP but has no AU/AAX support.

### Cited Findings
**VST3 SDK**
- VST 3.8.0 (October 2025) changed the VST3 SDK license to MIT open source. Before that it was dual-licensed as "Proprietary Steinberg VST3" or GPLv3 — [Wikipedia: Virtual Studio Technology](https://en.wikipedia.org/wiki/Virtual_Studio_Technology); [Steinberg press release, 2025-10-29 (PDF)](https://ocl-steinberg-live.steinberg.net/_storage/asset/819253/storage/master/Press%20Release%20-%202025-10-29%20-%20VST%203.8%20-%20EN.pdf)
- Under MIT, developers no longer need to sign agreements with Steinberg. The only requirement is to keep the copyright and license text — [Sonicstate, 30 Oct 2025](https://sonicstate.com/news/2025/10/30/vst-3-now-available-under-mit-license); [rekkerd.org](https://rekkerd.org/vst-3-now-available-under-mit-license)
- At the same time, the ASIO SDK became available under GPLv3, alongside the proprietary option — [Headliner Hub](https://headlinerhub.com/steinberg-releases-vst-3-under-lit-license.html); [pixls.us discussion](https://discuss.pixls.us/t/vst3-becomes-open-source-asio-goes-gpl-compatible/53917)
- Steinberg's forum carries the MIT text, covering files in the SDK's public_sdk folder — [Steinberg forum: VST 3 SDK public_sdk License](https://forums.steinberg.net/t/vst-3-sdk-public-sdk-license/695592)
- Caveat: the MIT grant covers the SDK code. The "VST" trademark and logo have their own usage guidelines. **[PK]** — check against the press release.

**JUCE 8**
- Dual licensing: AGPLv3, or a commercial JUCE licence for closed-source projects — [JUCE forum: Licensing for a free plug-in](https://forum.juce.com/t/licensing-for-a-free-plug-in/66179); [JUCE forum: juce8 license and amateur programmers](https://forum.juce.com/t/juce8-license-and-amateur-programmers/61017)
- JUCE 8 tiers, per JUCE's EULA announcement:
  - **Starter:** free, up to $20k annual revenue or funding. The splash-screen requirement was removed and "income" was defined more narrowly.
  - **Indie:** up to $300k. $40/month per user (1-month minimum) or $800 perpetual per user.
  - **Pro:** no revenue limit. $175/month per user (12-month minimum) or $3,500 perpetual per user.
  - The Pro price went up and the Indie revenue cap went down.
  — [JUCE forum: Important changes to the JUCE EULA for JUCE 8](https://forum.juce.com/t/important-changes-to-the-juce-end-user-licence-agreement-for-juce-8/61265); [Amendments thread](https://forum.juce.com/t/amendments-to-the-juce-end-user-licence-agreement-for-juce-8/61265/1)
- JUCE staff say the EULA is the only binding text and that forum commentary is not legal advice — [JUCE forum EULA thread](https://forum.juce.com/t/amendments-to-the-juce-end-user-licence-agreement-for-juce-8/61265)
- JUCE's Q3 2024 roadmap says "JUCE 9 will add the ability to author CLAP plug-ins directly." The CLAP community newsletter also reported official CLAP support from JUCE 9 — [JUCE Roadmap Update Q3 2024](https://juce.com/?p=1627); [CLAP news July 2024](https://cleveraudio.org/news/july-2024/)
- Until then, `clap-juce-extensions` (free-audio) adds CLAP to JUCE 6/7/8 projects. It is not supported by the JUCE team — [ecosyste.ms: clap-juce-extensions](https://awesome.ecosyste.ms/projects/github.com%2Ffree-audio%2Fclap-juce-extensions)
- JUCE builds VST3, AU, AUv3, AAX, LV2 and Standalone from one codebase with CMake. AAX also requires the Avid AAX SDK and PACE/iLok signing (wraptool) to load in release Pro Tools. **[PK]** — [JUCE repo](https://github.com/juce-framework/JUCE)

**nih-plug (Rust)**
- An API-agnostic Rust framework. You export VST3 or CLAP with the `nih_export_vst3!` / `nih_export_clap!` macros — [nih-plug GitHub](https://github.com/robbert-vdh/nih-plug)
- The framework code is ISC-licensed, but the VST3 bindings it uses are GPLv3. Plugins built in VST3 format with nih-plug therefore fall under GPLv3. CLAP-only builds (`default-features = false`) avoid this — [culsynth README](https://github.com/rbmj/culsynth); [zenn.dev tutorial](https://zenn.dev/saisana299/articles/e9751a9e586681). Secondary sources only. Also, the GPL VST3 bindings predate Steinberg's MIT relicense, so this may change if the bindings are regenerated from the MIT SDK. That is an inference, not confirmed.
- A community fork, "nice-plug", now exists as a separate community-led project, which suggests reduced upstream activity — [docs.rs nice-plug](https://docs.rs/crate/nice-plug/0.3.0)
- nih-plug has no AU or AAX output. **[PK]**

**iPlug2, DPF, CLAP** (not re-verified this session)
- iPlug2: liberal zlib-style license. It targets VST2/VST3/AU/AUv3/AAX/CLAP/WAM (web) and standalone. **[PK]** — [iPlug2 GitHub](https://github.com/iPlug2/iPlug2)
- DPF (DISTRHO Plugin Framework): ISC license. It targets LADSPA/DSSI/LV2/VST2/VST3/CLAP, and has some AU support in recent versions **[PK, uncertain]**. It is used by the AIDA-X plugin (AU/CLAP/LV2/VST2/VST3) — [DPF GitHub](https://github.com/DISTRHO/DPF); [AIDA-X Arch package](https://archlinux.org/packages/extra/x86_64/aida-x-vst)
- CLAP: an MIT-licensed open plugin standard from u-he and Bitwig. Its relevant features for ML instruments are per-note modulation (MPE-like), a thread-pool extension, and explicit threading rules. **[PK]** — [CLAP GitHub](https://github.com/free-audio/clap); the JUCE roadmap calls CLAP "open source, permissively licensed" — [JUCE roadmap](https://juce.com/?p=1627)

### Inferences
- For a closed-source commercial instrument, JUCE 8 (Starter until $20k revenue, then Indie or Pro) plus the MIT VST3 SDK plus the Avid AAX SDK is the lowest-risk path. Add CLAP through clap-juce-extensions until JUCE ships native CLAP.
- For an open-source or GPL project, JUCE under AGPLv3 also works. Surge XT and Vital are GPLv3 JUCE projects, which shows the pattern.
- The MIT relicense removes the old reason GPL projects needed the VST3 GPL option, and makes permissive frameworks (iPlug2, DPF) cleaner for closed-source VST3.

### Gaps
- Whether JUCE 9 (or 8.x) with native CLAP had shipped by Oct 2026 could not be confirmed. Search found only the 2024 roadmap.
- Current JUCE pricing in 2026 could not be confirmed. The figures above are from the JUCE 8 launch (2024).
- nih-plug's maintenance status in 2026, and whether its VST3 bindings were relicensed after the MIT change, were not found.

## 2. Real-time ML inference in plugins: RTNeural, ONNX Runtime, LibTorch, TFLite/LiteRT, anira, Neutone, nn~, RAVE VST, Core ML/BNNS

### Takeaway
No engine wins across the board. Results depend on hardware, model type (stateless vs. stateful/recurrent), and block size.
- **RTNeural** is fastest for small dense, GRU, LSTM or conv models at small block sizes, but supports only a limited set of layers.
- **ONNX Runtime** is fastest for stateless models, and **LibTorch** for stateful models (anira paper, 2025).
- On Apple Silicon under realistic DAW load, Apple's **BNNSGraph** was the most robust for conv models (DAFx-26).
- General engines (ONNX Runtime, LibTorch, TFLite) are **not real-time safe**: they allocate memory and spawn threads. The standard fix is to run inference off the audio thread with a latency/buffer scheme, which is what **anira** does, or to use an engine built for real time (RTNeural, BNNSGraph).

### Cited Findings
**Benchmarks**
- **anira paper** (Ackva & Schulz, TU Berlin, arXiv 2506.12665, June 2025):
  - anira is a cross-platform C++ library wrapping ONNX Runtime, LibTorch and TensorFlow Lite.
  - It moves inference to a static thread pool off the audio callback, with built-in latency management and benchmarking tools.
  - Results across three audio-effect architectures: ONNX Runtime had the lowest runtimes for stateless models, and LibTorch was fastest for stateful models.
  - The first few inferences can be slower for some model/engine pairs and cause more real-time violations, so warm-up is needed.
  — [arXiv 2506.12665](https://arxiv.org/pdf/2506.12665); [anira GitHub](https://github.com/anira-project/anira); [JUCE Marketplace: anira](https://marketplace.juce.com/products/anira)
- **DAFx-26** "Real-Time Neural Audio on Apple Silicon: Benchmarking Inference Frameworks Under Realistic DAW Contention" (Balasubramaniam, Timoney — Maynooth). It compares BNNSGraph, RTNeural, LibTorch, ONNX Runtime and anira in mix sessions with configurable plugin loads. Key results:
  - "isolated benchmarks can be misleading"
  - BNNSGraph is the most robust for convolutional models on Apple Silicon
  - BNNSGraph compiles the whole graph ahead of time (op fusion, weight repacking, memory-copy elimination, use of the matrix co-processors) and can run on the audio thread without anira-style buffering
  - RTNeural's limited set of layer types and activations constrains which architectures can be deployed
  — [DAFx-26 paper PDF](https://dafx.de/paper-archive/2026/papers/DAFx26_paper_16.pdf); [DAFx archive entry](https://www.dafx.de/paper-archive/details/rBxXewYH3YxRTiGXqSabwQ). Specific latency and dropout numbers were not retrieved.
- Apple's WWDC24 session "Support real-time ML inference on the CPU" introduced BNNSGraph for real-time audio use — [Apple WWDC24 10211](https://developer.apple.com/videos/play/wwdc2024/10211/)
- **"Designing Neural Synthesizers for Low-Latency Interaction"** (arXiv 2503.11562, 2025) is directly relevant to an instrument. It built on RTNeural for causal block-based inference. The fastest implementation on an M1 Pro used RTNeural's STL backend, and LibTorch became more efficient at longer block sizes — [arXiv 2503.11562](https://arxiv.org/pdf/2503.11562)
- **DAFx-22** (Stefani, Peroni, Turchet) benchmarked TFLite, TorchScript, ONNX Runtime and RTNeural on a Raspberry Pi 4. All engines could run in real time "with appropriate code practices." Less specialized engines sometimes beat RTNeural slightly — [DAFx archive](https://www.dafx.de/paper-archive/details/qcQo4dIbC0k5_sjLxWUmKA)
- **RTNeural paper** (Chowdhury, 2021): an inference library for real-time systems, with compile-time-defined (templated) models and Eigen/xsimd/STL backends — [arXiv 2106.03037](https://arxiv.org/pdf/2106.03037); [RTNeural GitHub](https://github.com/jatinchowdhury18/RTNeural). License is BSD-3-Clause **[PK]**.
- Mobile reference: DAFx-24 "Neural Audio Processing on Android Phones" — [DAFx-24 paper 78](https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_78.pdf)

**Real-time safety in practice**
- In the JUCE forum thread "Our first commercial plugin using ONNX", the developer runs ONNX Runtime on the audio thread and accepts its memory allocations. They also chose smaller models — [JUCE forum](https://forum.juce.com/t/our-first-commercial-plugin-using-onnx/58195). This is a pragmatic but non-RT-safe shipping practice.
- ONNX Runtime's `intra_op_num_threads` controls parallelism within an operator. Plugin-oriented guidance says a single thread usually gives the lowest latency for small models — [ONNX Runtime threading docs (mirror)](https://www.mintlify.com/microsoft/onnxruntime/performance/threading); [Brainmodular ML Inference manual](https://www.brainmodular.com/manuals/hh7/en/modules/machine-learning/ml-inference)
- anira uses a single static thread pool to avoid oversubscription when many plugin instances run at once — [anira GitHub](https://github.com/anira-project/anira)

**Neutone, RAVE, nn~**
- **Neutone SDK** (Mitcheltree et al., AES 2025, arXiv 2508.09126):
  - Python/PyTorch SDK for wrapping models (TorchScript) for real-time and offline use.
  - Handles variable buffer sizes, sample-rate conversion, delay compensation and parameter handling.
  - Includes tooling to benchmark speed and latency across simulated DAW (sample rate, buffer size) combinations.
  - License: **LGPL-2.1**.
  — [arXiv 2508.09126](https://arxiv.org/html/2508.09126v1); [PyPI neutone-sdk](https://pypi.org/project/neutone-sdk); [AES e-Library](https://aes2.org/publications/elibrary-page/?id=23021)
- The **RAVE VST** repo (acids-ircam) is archived. Its README points users to **nn~** (a Max/MSP and Pure Data external) for real-time RAVE. Neutone grew out of the RAVE Audition VST (2021), which supported only one architecture — [rave_vst mirror](https://backiee.wasmer.app/https_github_com/acids-ircam/rave_vst); [arXiv 2508.09126](https://arxiv.org/html/2508.09126v1)
- RAVE uses `cached_conv` for streaming causal convolutions — [PyPI cached-conv](https://pypi.org/project/cached-conv/2.4.1/)
- Licenses for RAVE, nn~ and RAVE VST are CC BY-NC 4.0 / GPL-family **[PK, uncertain]**. Verify before use.

**Other engines** (not re-verified)
- ONNX Runtime: MIT license, with CPU, CoreML, DirectML and other execution providers.
- LibTorch: BSD-3. Its CPU binaries are large (hundreds of MB), which bloats the installer.
- TFLite, now LiteRT: Apache-2.0.
- Core ML: Apple-only; ANE/GPU dispatch is not deterministic enough for audio-thread use. BNNSGraph is Apple's CPU real-time path (see WWDC24 above).
- **[PK]** — [onnxruntime](https://github.com/microsoft/onnxruntime); [pytorch](https://github.com/pytorch/pytorch); [LiteRT](https://github.com/google-ai-edge/LiteRT)

### Inferences
- For an EDM instrument, use a two-tier design:
  - Run heavy generative models (latent or diffusion models that produce a whole one-shot or wavetable) **off the audio thread**, triggered on note-on or a "generate" button. Write the results into preallocated buffers or wavetables. This avoids per-sample real-time ML entirely.
  - For per-sample neural DSP (e.g., a learned saturation, "glue" or "finish" stage), use small RTNeural models at block sizes of 32–512, or anira wrapping ONNX Runtime, with the reported latency compensated through the plugin's latency API.
- Ship one inference engine per platform to keep binaries small: RTNeural everywhere, or ONNX Runtime plus BNNSGraph on macOS.
- Warm up models at load time to avoid the first-inference slowdowns noted in the anira paper.

### Gaps
- The exact µs-per-block numbers from the anira and DAFx-26 papers were not retrieved (snippets only).
- No benchmark specific to diffusion or latent generative synth models inside plugins was found.
- LiteRT's status for desktop plugins in 2026 was not researched.

## 3. Python offline analysis tooling: features, loudness, differentiable DSP, stem separation, embeddings

### Takeaway
The standard stack is: librosa or essentia for descriptors, pyloudnorm for BS.1770 LUFS, torchaudio, auraloss for multi-resolution STFT losses, dasp-pytorch for differentiable mixing/mastering effects, pedalboard for fast effect and VST rendering, and RoFormer-family models (BS-/MelBand-RoFormer) or HTDemucs for stems. RoFormers slightly beat HTDemucs on "other" and roughly tie on bass on pop/rock benchmarks. No electronic-music-specific separation benchmark was found. For embeddings, **LAION-CLAP is reportedly CC0**, while **MERT weights are CC BY-NC 4.0** (non-commercial), which matters for a commercial product.

### Cited Findings
**Stem separation**
- Mel-Band RoFormer beats BS-RoFormer on vocals, drums and other on MUSDB18HQ. It is weaker on bass because the mel-band mapping models bass less well than band-split — [arXiv 2310.01809](https://arxiv.org/pdf/2310.01809)
- MVSep multisong scores:

  | Stem | BS-RoFormer | HTDemucs4 |
  |---|---|---|
  | Bass (SDR) | 12.49 | 12.52 |
  | Other | 16.59 | 16.64 |

  — [MVSep algorithms](https://mvsep.com/en/algorithms)
- On an MVSep "other" validation set: Demucs4HT (6-stem) 12.19, BS-RoFormer (viperx) 14.13, MelRoFormer 13.99 — [MVSep algorithm 17](https://www.mvsep.com/algorithms/17). Single benchmark; treat as indicative.
- A 2026 arXiv paper argues SDR misses rhythm-relevant distortions from separation, which matters for transient-heavy EDM drums — [arXiv 2609.04224 "Beyond SDR"](https://arxiv.org/pdf/2609.04224)
- Ensemble and hierarchical separation comparison — [arXiv 2410.20773](https://arxiv.org/pdf/2410.20773)
- Demucs (facebookresearch) is MIT-licensed code, but the repo is archived and no longer maintained **[PK]**. The community ZFTurbo "Music-Source-Separation-Training" repo and UVR distribute RoFormer checkpoints, each with its own license **[PK]**.

**Embeddings**
- MERT-v0, v1-95M and v1-330M weights are **CC BY-NC 4.0** — [Mixpeek MERT-v1-95M](https://mixpeek.com/model/m-a-p/MERT-v1-95M); [HF MERT-v0](https://huggingface.co/m-a-p/MERT-v0/discussions)
- A third-party page lists a "MERT-v2-FullSong" with a Sept 2026 release, also CC BY-NC — [Mixpeek MERT-v2](https://mixpeek.com/model/m-a-p/MERT-v2-FullSong). Unconfirmed by an official source.
- Derived checkpoints, e.g., TuneJury's CLAP+MERT, inherit MERT's non-commercial license — [arXiv 2606.17006](https://arxiv.org/pdf/2606.17006)
- LAION-CLAP code and weights are reported as **CC0-1.0**, including music checkpoints (e.g., `larger_clap_music`) — [LAION-AI/CLAP GitHub](https://github.com/LAION-AI/CLAP); [ecosyste.ms index](https://awesome.ecosyste.ms/projects/github.com%2FLAION-AI%2FCLAP); [HF larger_clap_music](https://huggingface.co/another-blue/larger_clap_music). Secondary sources; confirm against the LICENSE file.

**DSP, feature, loss and rendering libraries** (not re-verified this session)

| Library | License | What it does |
|---|---|---|
| librosa | ISC | Features: STFT, mel, spectral descriptors, onset, tempo |
| essentia | AGPLv3 (commercial licence from UPF/MTG) | Rich descriptors, pretrained TF models; copyleft is a pitfall if bundled in a product |
| pyloudnorm | MIT | ITU-R BS.1770-4 integrated loudness |
| torchaudio | BSD-2 | Note: from 2.8/2.9 many I/O and legacy features were deprecated or moved toward TorchCodec. Check the current API. |
| auraloss | Apache-2.0 | Multi-resolution STFT, SI-SDR and other losses |
| dasp-pytorch | Apache-2.0 | Differentiable EQ, compressor, distortion, reverb for mix/master style learning |
| pedalboard (Spotify) | GPLv3, because it bundles JUCE | Loads VST3/AU and renders audio very fast; useful for batch rendering |

— [librosa](https://github.com/librosa/librosa); [essentia](https://github.com/MTG/essentia); [pyloudnorm](https://github.com/csteinmetz1/pyloudnorm); [torchaudio](https://github.com/pytorch/audio); [auraloss](https://github.com/csteinmetz1/auraloss); [dasp-pytorch](https://github.com/csteinmetz1/dasp-pytorch); [pedalboard](https://github.com/spotify/pedalboard)

### Inferences
- GPL/AGPL tools (essentia, pedalboard, DawDreamer) are fine in an **offline internal pipeline** whose outputs are data or model weights. They should not be linked into the shipped plugin binary. Rendered audio and trained weights are generally not "derivative works" of the tool's code, but this is a legal judgment.
- For a commercial product, use LAION-CLAP (CC0) rather than MERT (non-commercial) for any embedding model that ships or whose outputs directly drive product features.
- For EDM references, separate drums from "everything else" with a RoFormer or HTDemucs ensemble. Then validate transient fidelity yourself, since SDR under-reports drum smearing.

### Gaps
- No public separation benchmark on EDM or electronic test sets was found.
- License terms of specific community RoFormer checkpoints were not verified.
- torchaudio's 2026 deprecation state was not re-checked.

## 4. Datasets for synth/EDM sounds, and rendering your own (Surge XT, Vital, DawDreamer, pedalboard)

### Takeaway
Off-the-shelf datasets (NSynth, Slakh2100, Surge preset renders, DX7 renders) are mostly acoustic, GM-sample or older-synth timbres, not "finished" modern EDM. The practical route is to **batch-render your own** dataset from open-source synths through DawDreamer or pedalboard, ideally sampling parameters from preset distributions rather than uniformly. Surge XT and Vital are GPLv3. Rendering their audio for training does not by itself make your plugin GPL, but bundling their code or DSP would. Commercial synths (Serum) can be rendered for internal datasets subject to their EULAs. Factory presets carry their own copyright and licensing questions.

### Cited Findings
**Existing datasets**
- Zenodo "Pitch Audio Dataset": 3.4 hours of audio rendered with Surge from 2,084 factory presets. Listed uses include pitch prediction and preset prediction — [Zenodo 4677097](https://zenodo.org/records/4677097); companion DX7-clone "Timbre Audio Dataset" — [Zenodo 4677102](https://zenodo.org/record/4677102)
- "One Billion Audio Sounds from GPU-enabled Modular Synthesis" (synth1B1 / torchsynth) built on 31k human-designed DX7 presets — [arXiv 2104.12922](https://arxiv.org/pdf/2104.12922)
- **Sound-matching research:**
  - Audio Spectrogram Transformer sound matching sampled parameters from the distribution of Massive factory presets rather than uniformly, to get realistic sounds, and rendered with pedalboard — [arXiv 2407.16643](https://arxiv.org/pdf/2407.16643)
  - Flow-matching synth inversion in symmetric parameter spaces — [arXiv 2506.07199](https://arxiv.org/pdf/2506.07199)
- Sound2Serum (Aalborg, 2026) built a 40k audio/parameter dataset for Serum 2. Results were poor: in 82% of trials listeners rated outputs below a 300 Hz low-pass anchor. This is a warning about how hard naive parameter regression is on complex wavetable synths — [AAU project](https://projekter.aau.dk/sound2serum-neural-sound-matching-system-for-serum-2-6027defd.html)
- Slakh2100: 2,100 automatically mixed tracks rendered from Lakh MIDI with sample-based virtual instruments, with aligned MIDI. About 105 GB — [Zenodo Slakh2100](https://zenodo.org/record/4599666); [arXiv 1909.08494](https://arxiv.org/pdf/1909.08494). License not shown in retrieved text; CC BY 4.0 **[PK]**.
- Mixing Secrets: 258 multitracks from the "Mixing Secrets for the Small Studio" site, used for instrument recognition — [Papers with Code](https://cs.paperswithcode.com/dataset/mixing-secrets). Reuse terms not found. The site allows educational mixing practice, not redistribution or commercial training **[PK, uncertain]**.
- Licenses not retrieved this session, all **[PK]**:
  - NSynth (305,979 four-second notes, 1,006 instruments): CC BY 4.0 — [Magenta NSynth](https://magenta.tensorflow.org/datasets/nsynth)
  - Freesound: per-sound CC0 / CC BY / CC BY-NC, so filter by license — [Freesound](https://freesound.org)
  - FSD50K: per-clip CC licenses

**Rendering tools**
- DawDreamer is GPLv3, "to make it easier to comply with all of the dependent projects" (JUCE, pybind11, libsamplerate, Rubber Band, Steinberg VST, FAUST). It hosts VST plugins, sets parameters, sends MIDI, renders multiple processors and supports multiprocessing — [DawDreamer GitHub](https://GitHub.com/DBraun/DawDreamer); [PyPI](https://pypi.org/project/dawdreamer); [arXiv 2111.09931](https://arxiv.org/abs/2111.09931)
- DawDreamer's ISMIR paper credits RenderMan as the predecessor for synth-preset research — [ISMIR 2021 LBD](https://archives.ismir.net/ismir2021/latebreaking/000001.pdf)
- Surge XT is packaged as GPL-3.0 (with Airwindows components under GPL-2.0-or-later) — [Fedora spec](https://download.copr.fedorainfracloud.org/results/ycollet/audinux/srpm-builds/09585595/surge-xt.spec). Surge also offers a Python binding (`surgepy`) for headless rendering **[PK]** — [surge GitHub](https://github.com/surge-synthesizer/surge)
- Vital's open-source release (Vitalium/"vital" repo) is GPLv3. The "Vital" name and branding are reserved, and it has no first-party headless Python API **[PK]** — [mtytel/vital](https://github.com/mtytel/vital)

### Inferences
- **Surge XT via surgepy or DawDreamer** is the most scriptable open synth for large datasets. Vital needs a plugin host (DawDreamer or pedalboard) and state-file manipulation.
- **GPLv3 implications:**
  - Rendered audio is program *output*. GPL generally does not cover output unless the output contains the program itself.
  - Vital's and Surge's *factory wavetables and presets* may have separate licensing; check before redistributing.
  - Do not copy GPL DSP code into a closed plugin.
  - DawDreamer and pedalboard being GPL does not matter for internal rendering scripts that are not distributed.
- For "finished EDM" quality, raw synth renders are not enough. Add a processing stage in the rendering pipeline (saturation, OTT-style multiband compression, EQ, reverb via pedalboard), and/or learn targets from professional reference one-shots. Preset-distribution sampling (as in the Massive paper) beats uniform sampling.

### Gaps
- NSynth, Slakh and Mixing Secrets licenses were not confirmed from primary pages this session.
- No public large-scale Vital or Serum render dataset was found.
- No public drum one-shot dataset with clear commercial-use licensing was found. Commercial sample packs usually forbid ML training in their EULAs, but this was not researched.
- No legal source was found specifically on training ML models with GPL-synth renders.

## 5. Example open-source projects combining plugin frameworks + neural nets

### Takeaway
The proven open-source pattern is small recurrent or conv models (LSTM, GRU, WaveNet-lite) trained in PyTorch, exported to JSON or weights, and run with RTNeural or a custom Eigen/NAM core inside JUCE, iPlug2 or DPF plugins. Neural Amp Modeler, AIDA-X, GuitarML (Proteus, SmartAmp) and Chowdhury DSP (ChowTape, ChowCentaur) are the references. All are effects, not generative instruments.

### Cited Findings
- Neural Amp Modeler LV2 (mikeoliphant): MIT. It has no resampling, so the host must run at the model's training sample rate (a pitfall) — [Fedora spec](https://copr-be.cloud.fedoraproject.org/results/ycollet/audinux/srpm-builds/09094465/neural-amp-modeler.spec)
- The NAM LV2 UI variant bundles RTNeural and NeuralAmpModelerCore, under mixed MIT / BSD-3 / 0BSD / GPL-3.0 licenses — [AUR](https://aur.archlinux.org/packages/neural-amp-modeler-ui-lv2)
- AIDA-X: an AU/CLAP/LV2/VST2/VST3 plugin (DPF-based) that loads RTNeural models and cabinet IRs. GPL-3.0-or-later — [Arch package](https://archlinux.org/packages/extra/x86_64/aida-x-vst)
- AidaDSP's Automated-GuitarAmpModelling is the training pipeline that exports RTNeural-compatible models (a fork of GuitarML's) — [GitHub](https://github.com/AidaDSP/Automated-GuitarAmpModelling)
- NeuralBlender: an app and LV2 plugin built on RTNeural and NAM, GPLv3 — [SourceForge](https://sourceforge.net/projects/neuralblender/)
- Example JUCE + ONNX student/indie amp: 4000DB-NeuralAmp — [GitHub](https://github.com/EnrcDamn/4000DB-NeuralAmp)
- **[PK]** (not re-verified):
  - NAM plugin (sdatkinson/NeuralAmpModelerPlugin): iPlug2-based, MIT
  - NeuralAmpModelerCore: MIT, Eigen-based
  - GuitarML Proteus/SmartGuitarAmp: JUCE + RTNeural, GPLv3 / Apache mix
  - Chowdhury DSP's ChowTapeModel and ChowCentaur: JUCE + RTNeural, BSD-3 / GPL
  — [NAM plugin](https://github.com/sdatkinson/NeuralAmpModelerPlugin); [GuitarML](https://github.com/GuitarML); [Chowdhury-DSP](https://github.com/Chowdhury-DSP)
- HISE also discusses simple ML / RTNeural integration — [HISE forum](https://forum.hise.audio/post/86035)

### Inferences
- These projects prove that real-time neural DSP is viable on CPU with tiny models (roughly 1–20k parameters, LSTM/GRU hidden sizes of 8–40) at normal block sizes. A neural "finishing" stage in an EDM instrument should start in that size range.
- Generative timbre (RAVE- or Neutone-style) remains heavier, so plan it as an off-thread or prerender feature.

### Gaps
- No open-source *generative instrument* VST3 with JUCE + neural nets at production quality was surveyed in depth (beyond Neutone and RAVE).
- Per-project CPU usage figures were not retrieved.
