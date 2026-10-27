# Mix/Master Style Analysis, Modeling and Transfer (for an EDM "release-ready" VST3)

## Differentiable audio effects, effect-parameter estimation and audio-effects style transfer

### Takeaway
Between 2020 and 2025 the field settled on one pattern: put an interpretable chain of DSP effects (EQ, compressor, distortion, reverb, stereo, limiter) under a controller, and drive it with a learned "production style" embedding taken from a reference track. The controller is either a network that predicts parameters in one pass (DeepAFx-ST, Diff-MST; about 1 s) or an inference-time optimizer that searches the parameters (ST-ITO, ITO-Master; about 1 min). Most of the key code is open (Apache/MIT/GPL) and written in PyTorch, which suits offline training. Real-time VST3 use means running the DSP chain natively and keeping the neural part to parameter prediction.

### Cited Findings
- **DeepAFx-ST: "Style Transfer of Audio Effects with Differentiable Signal Processing"** (Steinmetz, Bryan, Reiss; JAES, Sept 2022). It takes an input recording and a style reference, and a network predicts the effect control settings. The effects are treated as differentiable operators and the model is trained end-to-end with an audio-domain loss. Training is self-supervised, so no labeled or paired data is needed. It generalized to unseen recordings and to sample rates not seen in training. Code: github.com/adobe-research/DeepAFx-ST. — [arXiv 2207.08759](https://arxiv.org/pdf/2207.08759); [Adobe Research](https://research.adobe.com/publication/style-transfer-of-audio-effects-with-differentiable-signal-processing)
- A third-party index lists the DeepAFx-ST repo as Python, with an "other" (MIT-style) license, last pushed May 2023. That suggests it is no longer actively maintained (unverified snapshot). — [ecosyste.ms](https://awesome.ecosyste.ms/projects/github.com%2Fadobe-research%2Fdeepafx-st)
- **dasp-pytorch** (Steinmetz). Differentiable reverb, distortion, dynamic range processing, EQ and stereo processing. It is aimed at virtual-analog modeling, blind parameter estimation, automated DSP and style transfer. The functions are stateless and take (batch, channels, samples) tensors. Install with `pip install dasp-pytorch`. Apache 2.0, so academic and commercial use is allowed. — [GitHub csteinmetz1/dasp-pytorch](https://github.com/csteinmetz1/dasp-pytorch); [PyPI](https://pypi.org/p/dasp-pytorch)
- ITO-Master's public Hugging Face Space extends dasp with a `dasp_additionals.py` module, which shows that dasp is used as a building block in Sony mastering work. — [HF Space](https://huggingface.co/spaces/jhtonyKoo/ITO-Master/blob/main/networks/dasp_additionals.py)
- **ST-ITO: "Controlling Audio Effects for Style Transfer with Inference-Time Optimization"** (Steinmetz et al., QMUL; ISMIR 2024). It searches the parameter space of an arbitrary effect chain at inference time, including unseen and non-differentiable effects (for example real plugins). It uses a gradient-free optimizer and a self-supervised style metric encoder called AFx-Rep. It introduces a four-part benchmark for style metrics and style transfer. Limitations: the user must supply a suitable effect chain, and inference takes about 1 min, against about 1 s for direct parameter estimation. — [arXiv 2410.21233](https://arxiv.org/pdf/2410.21233); [ISMIR 2024 poster](https://ismir2024program.ismir.net/poster_249.html)
- A follow-up by Yu et al. (QMUL and Sony AI, arXiv 2505.11315) adds a Gaussian prior from the DiffVox vocal-preset dataset to the ST-ITO optimization, so the search avoids unrealistic parameter settings. — [arXiv 2505.11315](https://arxiv.org/pdf/2505.11315.pdf)
- **DiffVox: "A Differentiable Model..."** (2025) is a differentiable vocal effects model with a preset dataset that is used as a parameter prior. — [arXiv 2504.14735](https://arxiv.org/html/2504.14735v1)
- **Diff-MST: "Differentiable Mixing Style Transfer"** (Vanka, Steinmetz, Rolland, Reiss, Fazekas; ISMIR 2024). It combines a differentiable mixing console, a transformer controller and an audio-production-style loss. From raw tracks plus a reference song it predicts console parameters, so the mix can be adjusted afterward. It addresses earlier problems with fixed track counts, artifacts and end-to-end black boxes. — [arXiv 2407.08889](https://arxiv.org/pdf/2407.08889); [ISMIR poster](https://ismir2024program.ismir.net/poster_299.html)
- A late-breaking demo puts Diff-MST inside Steinberg Cubase. It handles up to 20 raw tracks and lets the user tweak the predicted parameters. This is precedent for DAW/plugin integration. — [arXiv 2411.06576](https://arxiv.org/html/2411.06576v1)
- **"Automatic multitrack mixing with a differentiable mixing console of neural audio effects"** (Steinmetz, Pons, Pascual, Serrà; ICASSP 2021). It uses pre-trained neural sub-networks as effect proxies, weight sharing, and a sum/difference stereo loss. It works from few training examples, ignores input track order, accepts any number of sources, and outputs human-readable parameters. Engineers preferred it over baselines in a listening test. — [arXiv 2010.10291](https://arxiv.org/pdf/2010.10291); [IEEE Xplore](https://ieeexplore.ieee.org/document/9414364)
- **NablAFx** (Comunità, Steinmetz, Reiss; 2025, AES). An open-source PyTorch framework for black-box (neural) and gray-box (DDSP block chains with controllers) effect modeling, with logging, plotting and metrics. Its gray-box models are currently series chains only; graph-based and hybrid models are listed as future work. Code: github.com/mcomunita/nablafx. — [arXiv 2502.11668](https://arxiv.org/pdf/2502.11668)
- **GRAFX: "An Open-Source Library for Audio Processing Graphs in PyTorch"** (Lee, Martínez-Ramírez, Liao, Uhlich, Fabbro, K. Lee, Mitsufuji; DAFx 2024 demo). It handles graph data structures and batched node processing that is faster than computing processors one by one. Its demo optimizes mixing-graph processor parameters by gradient descent. Code: github.com/sh-lee97/grafx. A companion DAFx 2024 paper prunes mixing graphs and removes about two-thirds of processors while keeping full-console quality. — [arXiv 2408.03204](https://arxiv.org/pdf/2408.03204); [DAFx archive](https://dafx.de/paper-archive/details/lEeeistLRcuFbzDwUhc-Lw)
- **Blind estimation of audio effects (BE-AFX)** (Peladeau and Peeters; ICASSP 2024). An auto-encoder recovers the effects applied to a processed signal using only that signal, and it is trained on an audio-quality loss rather than parameter error. It compares DDSP against neural proxies of common mastering effects. The audio-loss approach reproduces the sound better, while the parameter-loss approach recovers parameters more accurately. — [arXiv 2310.11781](https://arxiv.org/pdf/2310.11781)
- **"Differentiable signal processing with black-box audio effects"** (2021). A deep encoder predicts effect parameters using only input/target paired audio as supervision. Applied to automatic mastering, it scored comparably to a specialized state-of-the-art commercial product in a listening test. — [arXiv 2105.04752](https://arxiv.org/abs/2105.04752v1)

### Inferences
- For a VST3, the most practical design is a native C++ DSP chain (EQ, multiband dynamics, saturation/clipper, stereo, limiter) whose parameters come from (a) a small predictor network trained offline with dasp-pytorch, GRAFX or NablAFx gray-box chains, or (b) an offline ST-ITO-style search against genre target embeddings. ST-ITO at about 1 min per optimization suits an "analyze" button, not real-time use.
- ST-ITO's ability to optimize non-differentiable chains means you can tune the plugin's own C++ DSP directly against a learned style metric (AFx-Rep) without writing differentiable versions of it.
- Gray-box chains map cleanly onto VST knobs, so the user keeps control and the predicted settings stay interpretable. This matches Diff-MST's stated aim.

### Gaps
- Exact licenses and maintenance status for the Diff-MST, ST-ITO/AFx-Rep, NablAFx and GRAFX repos were not checked in this pass. Diff-MST code is believed to be at github.com/sai-soum (not verified here).
- No source found that benchmarks running these predictor networks in real time inside a plugin (latency/CPU).
- The "Modeling of black-box effects" title from the brief was not located as a specific paper. BE-AFX (2310.11781) and the 2021 black-box paper (2105.04752) are the closest matches found.

## Automatic mixing/mastering systems and reference-based mastering

### Takeaway
Sony (Martínez-Ramírez, Koo, Lee, Mitsufuji) and QMUL (Steinmetz, Reiss, Vanka) lead the research. The key enabler is FX normalization, which lets wet, already-processed stems serve as training data. For mastering specifically, ITO-Master (ISMIR 2025) is the most relevant open reference-based system, and Matchering 2.0 is a simple, production-proven, non-neural baseline under GPL-3.0. GPL licensing matters if the plugin is closed-source.

### Cited Findings
- **"Automatic music mixing with deep learning and out-of-domain data"** (Martínez-Ramírez et al., Sony; ISMIR 2022). It computes average audio-effect features on wet music-source-separation (MSS) data and normalizes wet stems with them (FxNorm), which enables supervised training. The same normalization is applied to dry inputs at inference. It was validated by highly experienced mixing engineers in a redesigned listening test. Code: sony/FxNorm-automix (last push Mar 2024 per a tracker). — [arXiv 2208.11428](https://arxiv.org/pdf/2208.11428); [repo tracker](https://gittrend.io/repo/sony/FxNorm-automix)
- **"Automatic Music Mixing using a Generative Model of Effect Embeddings"** (Sony; arXiv 2511.08040, ICASSP 2026). Extends the FxNorm line, using domain adaptation to train on both dry and wet recordings. — [arXiv 2511.08040](https://arxiv.org/pdf/2511.08040); [Sony AI](https://ai.sony/publications/automatic-music-mixing-using-a-generative-model-of-effect-embeddings)
- **"Music Mixing Style Transfer: A Contrastive Learning Approach to Disentangle Audio Effects"** (Koo et al., Sony and SNU; ICASSP 2023). The FXencoder, pre-trained with a contrastive objective, extracts only the effects information from a reference. The MixFXcloner converts the input's mixing style to match it. Training is self-supervised on wet multitracks: two segments are FX-normalized and then the system learns to match them. Code and pre-trained weights are public. — [arXiv 2211.02247](https://arxiv.org/pdf/2211.02247); [GitHub jhtonykoo/music_mixing_style_transfer](https://github.com/jhtonykoo/music_mixing_style_transfer)
- **"ITO-Master: Inference-Time Optimization for Audio Effects Modeling of Music Mastering Processors"** (Koo et al., Sony; ISMIR 2025; LBD at ISMIR 2024). It optimizes the reference embedding at inference time for finer user control, and compares black-box and white-box (differentiable chain) mastering models. Text steering via CLAP embeddings is evaluated. ITO improves mastering-style similarity in objective tests and listening tests. The paper notes that mastering needs particular care with limiters and dynamic range, because heavily compressed commercial music is harder to match. A public demo is on Hugging Face. — [arXiv 2506.16889](https://arxiv.org/pdf/2506.16889); [Sony AI](https://ai.sony/publications/ito-master-inference-time-optimization-for-audio-effects-modeling-of-music-mastering-processors); [ISMIR 2025 poster](https://ismir2025program.ismir.net/poster_59.html)
- **SonicMaster: "Towards Controllable All-in-One Music Restoration and Mastering"** (arXiv 2508.03448, 2025; ICML 2026 poster). It builds a large paired dataset of degraded and high-quality tracks using 19 degradation functions in 5 groups. A flow-matching generative model maps inputs to cleaned, mastered output with text prompts or an automatic mode. Listeners preferred it over baselines. Being generative, it re-synthesizes the audio rather than processing it with DSP. — [arXiv 2508.03448](https://arxiv.org/html/2508.03448v3); [ICML](https://icml.cc/virtual/2026/poster/63870)
- **Matchering 2.0** (sergree / wokashi-rg). Given a TARGET and a REFERENCE, it outputs the TARGET mastered with the reference's RMS, frequency response, peak amplitude and stereo width. It is pure Python 3 (no MATLAB) and includes its own open-source brickwall limiter. GPL-3.0. Available as a Docker image or a Python library (needs libsndfile). — [GitHub wokashi-rg/Matchering](https://github.com/wokashi-rg/Matchering); [bonedo](https://www.bonedo.de/artikel/matchering-20-erlaubt-kostenloses-mastering-auf-open-source-basis)
- **AI Mastering Detector** (ISMIR 2025 LBD). Built 135 pairs of human-mastered vs. iZotope Ozone-mastered audio. A classifier on Demucs encoder features tells them apart with 88.2% test accuracy, so AI masters carry a detectable signature. — [ISMIR 2025 LBD](https://ismir2025program.ismir.net/lbd_465.html)
- 2021 paper: differentiable black-box effects plus an encoder used for automatic mastering performed comparably to a commercial state-of-the-art solution in a listening test. — [arXiv 2105.04752](https://arxiv.org/abs/2105.04752v1)

### Inferences
- A "release-ready out of the box" plugin can ship genre preset embeddings or targets (per-band spectrum, loudness, width, crest factor) computed offline from reference corpora, and run a Matchering-style matching stage, a learned parameter predictor, or both. That removes the need for a user-supplied reference track.
- GPL-3.0 means Matchering code cannot go into a closed-source VST without open-sourcing the plugin. Reimplementing the published idea (RMS, spectrum, peak and width matching) is the safer route.
- The 88.2% detectability of Ozone masters suggests that simple chains leave a fingerprint. That is a potential quality criterion, or a warning that "AI-mastered sound" is itself recognizable.

### Gaps
- No primary technical documentation was found for LANDR or iZotope Ozone's Master Assistant internals in this pass.
- No research-grade, publicly released paired pre-master/post-master dataset of real professional masters was found. SonicMaster uses synthetic degradations and MUSDB-XL uses a single commercial limiter.
- Matchering 2.0's internal EQ/loudness algorithm details were not retrieved. The README points to a Habr write-up.

## Can engineer/genre styles be identified and modeled, and which descriptors matter?

### Takeaway
Measurable mix variation reduces to a few dimensions: loudness/dynamics, brightness, bass and stereo width. The effect of the individual engineer is small compared with the effects of song and genre (eta² = 0.021 for mix preference). So genre-level targets are better supported and more achievable than "engineer fingerprints". Masters cluster tightly in loudness and true peak, and electronic/dance releases sit at the loud end.

### Cited Findings
- Wilson and Fazenda (JAES 2016) analyzed 1,501 mixes of 10 songs from Cambridge Multitracks. Mixes varied along four main dimensions: amplitude (loudness/dynamics), brightness, bass and width. The mix engineer had a small effect on preference ratings (eta² = 0.021). — [Salford repository](https://salford-repository.worktribe.com/OutputFile/1493955); [101 mixes study](https://salford-repository.worktribe.com/output/1410046/101-mixes-a-statistical-analysis-of-mix-variation-in-a-dataset-of-multi-track-music-mixes)
- De Man, Leonard, King and Reiss (ISMIR 2014) had 8 songs each mixed by 8 engineers. Using dynamic, spatial and spectral features, they tested which processing depends on instrument, song or engineer. This is the closest controlled "engineer fingerprint" study found. — [ISMIR 2014 PDF](https://archives.ismir.net/ismir2014/paper/000293.pdf)
- The McGill/Mitacs project proposes gathering data to quantify an engineer's unique "sound profile". It is a proposal; no published results were found. — [Mitacs](https://www.mitacs.ca/our-projects/automatic-mixing-through-machine-learning/)
- **"Exploring trends in audio mixes and masters: Insights from a dataset analysis"** (Mourgela et al., AES NY 2024). Uses MixCheck Studio data on integrated loudness, mono compatibility, clipping, phase, compression and tonal profile across 30 genres; the dataset is on Zenodo. Findings:
  - Masters cluster tightly around -14 LUFS, and true peak is often at or just below 0 dBTP.
  - Mixes lean narrow in stereo field (39.04% narrow vs 17.94% wide), while masters lean wide (39.36% wide vs 16.45% narrow).
  - 46.43% of mixes "need more compression", while 51.63% of masters hit the optimal mark.
  - Masters show fewer stereo and phase issues.
  — [arXiv 2412.03373](https://arxiv.org/pdf/2412.03373); [Zenodo dataset](https://zenodo.org/records/13683186); [AES](https://aes2.org/publications/elibrary-page/?id=22689)
- iZotope's analysis of 10 top-charting dance/electronic songs reports most commercial releases mastered around -8.3 LUFS (±1 LU across recent genres), and says electronic music is "known for 'needing' to be really loud", north of -6 LUFS. This conflicts with the -14 LUFS clustering in the MixCheck dataset; the likely reason is that MixCheck holds user uploads, not chart releases. — [iZotope dance/electronic analysis](https://www.izotope.com/en/learn/dance-electronic-analysis); [iZotope rock analysis (method comparison)](https://www.izotope.com/community/blog/rock-alternative-analysis)
- A vendor guide lists EDM/trap targets of -6 to -8 LUFS and links low loudness range (LRA) with EDM, trap and modern pop. This is marketing material and should be treated as a rule of thumb. — [soundboost.ai](https://soundboost.ai/lufs-meter)
- A practitioner guide puts club/festival releases at -6 to -9 LUFS integrated, against streaming normalization near -14 LUFS, and recommends a -1 dBTP ceiling. — [Violet Recording](https://violetrecording.com/how-to-master-edm/)
- **Learned style descriptors (alternative to hand-crafted ones):**
  - AFx-Rep (ST-ITO) is a self-supervised production-style embedding.
  - FXencoder (Koo 2023) is a contrastive effects-only embedding.
  - CLAP text embeddings are used to steer ITO-Master.
  — [arXiv 2410.21233](https://arxiv.org/pdf/2410.21233); [arXiv 2211.02247](https://arxiv.org/pdf/2211.02247); [arXiv 2506.16889](https://arxiv.org/pdf/2506.16889)

### Inferences
- A compact target descriptor set backed by the evidence:
  - integrated LUFS and short-term LUFS distribution / LRA
  - true peak
  - crest factor (peak-to-RMS, ideally per band)
  - spectral tilt / long-term average spectrum against a genre curve (brightness and bass dimensions)
  - stereo width per band (mid/side ratio) with mono compatibility below about 100–150 Hz
  - phase correlation
  - clipping incidence
- Transient density is plausible but not covered by the sources found.
- Per-subgenre (dubstep vs techno vs synthwave) targets have to be derived in-house from curated reference sets; none were found published.
- Model "genre/era style" plus an optional user reference, rather than named engineers. The evidence says engineer identity carries little variance, and named-engineer cloning raises IP and marketing concerns.

### Gaps
- No peer-reviewed per-EDM-subgenre loudness/spectral statistics were found (dubstep, techno, tech-house, trap, synthwave).
- No published classifier accuracy for identifying mix engineers from audio was found; full texts of the 2014/2016 studies were not accessed.
- No source quantified "transient density" as a mastering descriptor.

## Datasets

### Takeaway
Multitrack data (MUSDB18/HQ, MedleyDB, Cambridge Multitracks) is mostly pop/rock, with little EDM. Mastering-specific data is scarce or synthetic (MUSDB-XL, SonicMaster, MixCheck metrics). An EDM product will probably need its own reference corpus of commercial EDM masters for feature statistics (not for audio redistribution), plus synthetic degrade-then-restore pairs.

### Cited Findings
- **musdb-L / musdb-XL** (Jeon et al.): limiter-applied versions of musdb-HQ made with the iZotope Ozone 9 Maximizer, introduced in "Towards robust music source separation on loud commercial music" (with LimitAug augmentation). — [DeepAI](https://deepai.org/publication/towards-robust-music-source-separation-on-loud-commercial-music); [Zenodo](https://zenodo.org/record/8191153)
- **musdb-XL-train**: 300k limited segments, distributed as sample-wise gain parameters (.npy) rather than audio for copyright reasons, and rebuilt from musdb18-HQ with the repo's code. A June 2024 update fixed a phase shift in about 7% of segments. — [Zenodo](https://zenodo.org/records/8191153/latest); [GitHub jeonchangbin49/De-limiter](https://github.com/jeonchangbin49/De-limiter)
- **De-limiter** (WASPAA 2023), using sample-wise gain inversion. Reconstructing musdb-HQ from musdb-XL reaches 24.0 dB SI-SDR in the arXiv version; DeepAI lists 23.8 dB. The model is 9 MB. — [arXiv 2308.01187](https://arxiv.org/pdf/2308.01187); [DeepAI](https://deepai.org/publication/music-de-limiter-networks-via-sample-wise-gain-inversion)
- **Cambridge Multitracks** mixes (1,501 mixes of 10 songs) were used for mix-variation statistics. — [Salford](https://salford-repository.worktribe.com/OutputFile/1493955)
- **MixCheck Studio metrics dataset** (Zenodo, 2024): loudness, mono compatibility, clipping, phase, compression and tonal profile across 30 genres. — [Zenodo 13683186](https://zenodo.org/records/13683186)
- **SonicMaster dataset**: paired degraded/high-quality tracks from 19 degradation functions. — [arXiv 2508.03448](https://arxiv.org/html/2508.03448v3)
- **DiffVox preset dataset**: vocal-effect parameter distributions, used as an optimization prior. — [arXiv 2504.14735](https://arxiv.org/html/2504.14735v1); [arXiv 2505.11315](https://arxiv.org/pdf/2505.11315.pdf)
- **135 human vs Ozone master pairs** (AI Mastering Detector LBD). — [ISMIR 2025 LBD](https://ismir2025program.ismir.net/lbd_465.html)
- The FxNorm-automix and Koo style-transfer work train on wet MSS multitracks (MUSDB-style stems). — [arXiv 2208.11428](https://arxiv.org/pdf/2208.11428); [arXiv 2211.02247](https://arxiv.org/pdf/2211.02247)

### Inferences
- The MUSDB-XL "parameter-only distribution" trick (shipping gains, not audio) is a useful precedent for sharing derived data from copyrighted masters.
- Applying FxNorm to EDM stems (from sample packs or the user's own projects) plus synthetic degradations is a viable training recipe, given how little dry/wet EDM data exists.

### Gaps
- Details of MUSDB18, MedleyDB and MixingSecrets-derived datasets (licenses, genre makeup) were not re-verified in this pass. They are widely known, but no source was fetched here.
- No public paired dataset of professional EDM pre-master and post-master audio was found.

## Practical engineer guidance on the "finished" EDM sound, and whether it can be measured

### Takeaway
Practitioner sources agree on the usual EDM finishing chain:
- tightly controlled, mono sub
- multiband compression in the OTT style (upward and downward)
- saturation and clipping before the limiter
- -6 to -9 LUFS for club masters with about -1 dBTP ceilings

Most of these leave measurable traces (LUFS, LRA, crest factor, per-band width, low-band dynamics, clipping incidence). Sidechain pumping and mono-bass guidance were poorly covered by authoritative sources in this search.

### Cited Findings
- Sound On Sound: polyphonic material sounds nasty under even light clipping, but monophonic bass and lead vocals can tolerate flat-topping. Clipping's added odd harmonics are heard more as a timbral change; on bass it shows up as a square-wave-like tone. — [Sound On Sound](https://www.soundonsound.com/node/4918581)
- Mastering engineer Daniel Wyatt on mastering electronic dance music (Sonnox tips page; content not retrieved in detail). — [Sonnox](https://www.sonnox.com/tips/daniel-wyatt-mastering-electronic-dance-music)
- Practitioner guide: high-pass at 20–25 Hz, and 2–3 dB of multiband compression below about 120 Hz to keep the sub consistent. Club/festival masters at -6 to -9 LUFS, -1 dBTP ceiling. — [Violet Recording](https://violetrecording.com/how-to-master-edm/)
- Forum consensus (anecdotal): compressor before clipper, possibly in parallel, and a clipper before the limiter to catch overshoots. — [KVR forum](https://www.kvraudio.com/forum/viewtopic.php?p=8190501)
- OTT (Xfer) splits audio into three bands (low/mid/high) and applies upward and downward compression to each, with per-band output. It is common in EDM and dubstep. — [EDMProd](https://www.edmprod.com/ott-plugin/); [Bedroom Producers Blog](https://bedroomproducersblog.com/2023/07/31/xfer-records-ott-2/)
- Upward compression lifts only low-level signal, unlike static EQ gain. Full-band upward compressors pump on EDM because the dominant low end leaves no "low levels" while the bass plays, which is why multiband designs are preferred. — [Gearspace thread](https://gearspace.com/threads/multiband-upward-compression-plugins.1384521/); [Sound On Sound Woott review](https://www.soundonsound.com/reviews/bram-bos-woott?amp)
- Commercial multiband up/down compressors include Slate MO-TT and McDSP ML4000. — [Unison](https://unison.audio/ott-plugin/); [McDSP](https://mcdsp.com/?p=4462)
- Chart dance/electronic masters run around -8.3 LUFS, with genre reputation for louder than -6 LUFS. — [iZotope](https://www.izotope.com/en/learn/dance-electronic-analysis)

### Inferences
Mapping from technique to measurable target, which can drive the VST's analysis and auto-set stage:

| Technique | Measurable target |
|---|---|
| Mono bass | Side/mid energy ratio below about 120 Hz kept near zero; correlation near +1 in the sub band |
| Sub control | Low-band crest factor / short-term level variance |
| OTT-style density | Reduced per-band crest factor and LRA; raised low-level detail (spectral flatness of quiet passages) |
| Clipping/saturation | Peak-to-loudness ratio (PLR); harmonic distortion content; flat-top sample counts |
| Final loudness | Integrated LUFS per genre (for example about -6 to -9 LUFS for club EDM); true peak at or below -1 dBTP; per-band width rising with frequency |

Sidechain pumping could be measured as periodic gain modulation synced to tempo (envelope autocorrelation at the beat period), but no source validating this was found.

### Gaps
- No authoritative (Sound On Sound or iZotope Learn) source was retrieved on sidechain compression or mono-bass crossover frequencies for EDM.
- No iZotope Tonal Balance Control genre curves (dB-per-band targets) were retrieved.
- Practitioner loudness numbers conflict (-6 to -9 vs -8.3 vs "louder than -6" LUFS) and vary by subgenre (dubstep/riddim is likely louder than synthwave), but no subgenre-specific measurements were found.
