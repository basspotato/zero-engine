# Neural / Differentiable Sound Synthesis and Synthesizer Sound Matching (for a "finished-sound" VST3 synth)

Note on sourcing: items marked [verified] were confirmed via search results this session (abstracts, project pages, proceedings listings). Items marked [prior knowledge] are well-known foundational papers cited by arXiv ID from memory and were not re-fetched; the arXiv IDs should be spot-checked. Full-text result tables were generally not read, so specific accuracy numbers are scarce. Where I have none, it is listed as a gap.

## 1. DDSP and differentiable synthesizers (wavetable, FM, subtractive, modular)

### Takeaway
Differentiable synthesizers are mature as research tools: DDSP, differentiable wavetable synthesis, DiffMoog, torchsynth and SynthAX all have open code. They are cheap at inference because the expensive part is a small control network driving classic DSP, so they suit a real-time plugin well. Their sound quality is capped by the DSP they model, and real-time DDSP timbre transfer has scored lower in listening tests than offline demos. The 2025–2026 trend is to compile differentiable graphs into real-time DSP and to discover modulation.

### Cited Findings
- **DDSP (Engel et al., ICLR 2020):** an autoencoder that predicts the controls of differentiable harmonic oscillators, filtered noise and reverb. This is the foundational work. — [arXiv 2001.04643](https://arxiv.org/abs/2001.04643) [prior knowledge]; summary at [EmergentMind DDSP topic](https://www.emergentmind.com/topics/ddsp-framework)
- One summary reports that real-time DDSP plugin timbre transfer scored lower in listening tests than offline demos. — [EmergentMind](https://www.emergentmind.com/topics/ddsp-framework) (secondary source)
- **Review:** "A Review of Differentiable Digital Signal Processing for Music & Speech Synthesis" (Hayes, Shier, Fazekas, McPherson, Saitis, 2023) surveys differentiable oscillators, filters, FM and wavetable methods. — [arXiv 2308.15422](https://arxiv.org/pdf/2308.15422) [verified]
- **Differentiable Wavetable Synthesis (Shan et al., ICASSP 2022):** learns a dictionary of one-period wavetables end to end and predicts per-frame mixing weights. It reports high-fidelity synthesis with only 10–20 wavetables and beats the DDSP additive model on reconstruction error with 20 wavetables. A PyTorch reimplementation exists (gudgud96/diff-wave-synth). The learned wavetables can be exported to a standard wavetable oscillator, which makes the approach directly relevant to Serum/Vital-style engines. — [arXiv 2111.10003](https://arxiv.org/pdf/2111.10003); [DDSP review](https://arxiv.org/pdf/2308.15422) [verified]
- **DiffMoog (Uzrad, Barkan et al., arXiv Jan 2024):** a differentiable modular synth with modules typically found in commercial instruments: FM/AM, LFOs, filters, envelope shapers, and user-defined signal chains. It adds a "signal-chain loss" and an encoder that predicts parameters for a user-defined architecture. The code is open at github.com/aisynth/diffmoog. — [arXiv 2401.12570](https://arxiv.org/abs/2401.12570v1) [verified]
- **torchsynth (Turian et al., DAFx 2020):** a GPU modular synth in PyTorch with the synth1B1 dataset of 1 billion sounds. It runs 16,200x faster than real time, which is a throughput figure, not latency. — [arXiv 2104.12922](https://arxiv.org/abs/2104.12922v2); [docs](https://torchsynth.readthedocs.io/en/v1.0.1/) [verified]
- **SynthAX (Cherep & Singh, AES 155, 2023):** a JAX port of torchsynth that runs more than 90,000x faster than real time at peak. It is the synth behind CTAG. — [GitHub PapayaResearch/synthax](https://github.com/PapayaResearch/synthax); [PyPI](https://pypi.org/project/synthax) [verified]
- **Modulation Discovery with DDSP (Mitcheltree, Tan, Reiss, arXiv Oct 2025):** neural sound matching that recovers the modulation curves (envelopes, LFOs) behind a sound using constrained control-signal parameterizations. The test synth is a wavetable oscillator with controllable position, a resonant filter and an envelope. The test set uses unseen modulation curves from Vital's default presets. The authors provide code, audio and VST plugins of the trained DDSP synths. — [arXiv 2510.06204](https://arxiv.org/abs/2510.06204) [verified]
- **Compiling Differentiable Audio Graphs to Real-Time DSP (DAFx 2026 demo):** compiles a PyTorch differentiable model to FAUST code and hot-reloads it in a plugin while training runs. — [arXiv 2606.21277](https://arxiv.org/pdf/2606.21277) [verified]
- **NoiseBandNet (2023):** a controllable, time-varying neural synthesizer for sound effects built on filterbanks. — [arXiv 2307.08007](https://arxiv.org/pdf/2307.08007) [verified]
- **NablAFx (2025):** a framework for differentiable black-box and gray-box modeling of audio effects, relevant to learning FX chains. — [arXiv 2502.11668](https://arxiv.org/pdf/2502.11668) [verified]

### Inferences
- For a release-ready EDM synth, differentiable models are best used offline, to fit wavetables, filters, envelopes and modulation to reference sounds. The result can then be rendered by a conventional, high-quality, alias-free C++ DSP engine at 44.1/48 kHz, which avoids neural-inference cost on the audio thread.
- Modulation discovery and differentiable wavetables map directly onto Serum/Vital-style architectures (wavetable position plus LFO/envelope modulation), the dominant design for dubstep and EDM sound design.

### Gaps
- I did not retrieve quantitative MSS/MOS numbers for DiffMoog or the modulation-discovery paper.
- I found no differentiable models of supersaw/unison detune stacks, OTT-style multiband compression or distortion chains validated against EDM presets. These are key to "finished" EDM sounds.

## 2. Synth parameter inference and sound matching (audio-to-preset)

### Takeaway
Sound matching has moved from regression over parameters (InverSynth 2019, Sound2Synth 2022) to generative and probabilistic inversion: flow matching (Hayes et al., ISMIR 2025 best student paper), off-manifold robustness (ISMIR 2026), neural proxies for black-box VSTs (JAES 2025), RL fine-tuning on out-of-domain audio (SynthRL, IJCAI 2025), and joint audio-preset embeddings (DAFx 2026). The field now agrees that parameter-space error is a poor metric and that audio-domain metrics should be used. Matching real-world or commercial sounds that the synth cannot exactly produce ("off-manifold") remains the main open problem. The one shipped commercial product is Synplant 2's Genopatch.

### Cited Findings
- **InverSynth (Barkan et al., IEEE/ACM TASLP 2019):** strided CNNs infer parameters from a spectrogram or raw audio for a subtractive synth with four FM oscillators, an envelope generator and a gater. It beat the baselines, and depth affected accuracy. — [Open University listing](https://cris.openu.ac.il/en/publications/inversynth-deep-estimation-of-synthesizer-parameter-configuration/) [verified]
- **Sound2Synth (Chen et al., IJCAI 2022):** a multimodal pipeline with a Prime-Dilated Convolution network for estimating FM parameters. The authors call it the first real-world usable results on Dexed (DX7 clone). The plug-in repo supports only a modified Dexed. — [arXiv 2205.03043](https://arxiv.org/abs/2205.03043v2); [IJCAI](https://www.ijcai.org/proceedings/2022/682); [GitHub plug-ins](https://github.com/Sound2Synth/Sound2Synth-Plug-Ins) [verified]
- **"Audio synthesizer inversion in symmetric parameter spaces with approximately equivariant flow matching" (Hayes, Saitis, Fazekas, ISMIR 2025, Best Student Paper):**
  - Synth parameter spaces have symmetries such as permutation of identical oscillators, so different presets produce the same sound and inversion is ill-posed.
  - Regressing point estimates degrades under symmetry, even with permutation-invariant losses.
  - Permutation-equivariant continuous normalizing flows trained by flow matching beat regression and generative baselines on audio-reconstruction metrics on **Surge XT**, a full-featured open-source synth. — [arXiv 2506.07199](https://arxiv.org/abs/2506.07199v1); [ISMIR 2025 poster](https://ismir2025program.ismir.net/poster_7.html); [C4DM award news](https://c4dm.eecs.qmul.ac.uk/news/2025-09-30.C4DM-ISMIR_Awards/) [verified]
- **"Off-manifold robustness in synthesizer inversion with joint distribution flow matching" (Hayes, accepted ISMIR 2026):** targets the case where a user supplies audio the synth could not have produced, while training data only covers synth output. That case is exactly what matching professional reference sounds involves. — [arXiv 2609.29320](https://arxiv.org/pdf/2609.29320) [verified, abstract-level only]
- **Neural Proxies for Sound Synthesizers (Combes, Weinzierl, Obermayer, JAES vol. 73 no. 9, 2025):**
  - Trains preset encoders (feedforward, RNN, transformer) to map presets into a pretrained audio-embedding space.
  - This gives a differentiable proxy for non-differentiable black-box VSTs, so audio-embedding losses can be used for sound matching.
  - Evaluated on three popular software synths with synthetic and hand-crafted presets. It reports "encouraging results for all synthesizers", with benefits weighed against resource costs.
  - Code at github.com/DBraun/synth-proxy. — [arXiv 2509.07635](https://arxiv.org/abs/2509.07635); [GitHub](https://github.com/DBraun/synth-proxy) [verified]
- **SynthRL (IJCAI 2025):** cross-domain sound matching via reinforcement learning, which allows fine-tuning on out-of-domain sounds without ground-truth parameter labels. — [IJCAI 2025 PDF](https://www.ijcai.org/proceedings/2025/1129.pdf) [verified, abstract-level]
- **FM-SynAPSE, "FM Synthesizer Audio-Parameter Shared Embeddings" (Braun & Finkelstein, DAFx 2026):**
  - Learns joint audio and DX7-preset embeddings with a GNN whose message passing mirrors FM operator routing, trained with a SLAP-style objective.
  - Supports preset retrieval and generalizes better than two baselines to held-out routing topologies.
  - Weights on Hugging Face. — [arXiv 2608.18226](https://arxiv.org/pdf/2608.18226); [DAFx archive](https://dafx.de/paper-archive/details/9cruKGWLvrCksEZIpcFtcg); [HF model](https://huggingface.co/davidbraun/fm-synapse-dx7-gnn) [verified]
- **Salimi et al. (arXiv 2025):** evaluates sound-similarity metrics for differentiable, iterative sound matching. Cited by community tools as the basis for multi-distance losses. — referenced in [Akurganow/sound-to-patch README](https://github.com/Akurganow/sound-to-patch) (secondary; I did not locate the paper directly)
- **Stanford/CCRMA ASA 2025 handout, "Neural Parameter Estimation for Musical Sound Synthesis":** a tutorial-level overview. — [CCRMA PDF](https://ccrma.stanford.edu/~jos/pdf/ASA-2025-SynthMatch-handout.pdf) [verified, not read in full]
- **Community Serum/Vital tools:** "sound-to-patch" and "sound-to-serum" (Akurganow, GitHub) convert an audio stem to a Vital or Serum 2 preset plus MIDI. Their README notes that audio metrics, not parameter distance, should score a match, and cites a 2026 Serum 2 thesis that has no license. These are not peer reviewed. — [sound-to-patch](https://github.com/Akurganow/sound-to-patch); [sound-to-serum](https://github.com/Akurganow/sound-to-serum) [verified, README-level]
- **CTAG (Cherep, Singh, Shand, ICML 2024):** this is text-to-patch rather than audio-to-patch. It uses gradient-free evolutionary optimization (LES via Evosax) of a 78-parameter SynthAX modular synth to maximize LAION-CLAP text-audio similarity. Variants range from 18 to 130 parameters. The result is an inspectable, editable synth patch. The code is open (PapayaResearch/ctag). — [PMLR v235](https://proceedings.mlr.press/v235/cherep24a.html); [arXiv 2406.00294](https://arxiv.org/html/2406.00294v1); [GitHub](https://github.com/papayaresearch/ctag) [verified]
- **Commercial precedent, Synplant 2 Genopatch (Sonic Charge, Oct 2023):**
  - Generates a synth patch from an audio file using several ML techniques, running locally on the CPU.
  - Iteratively gets closer to the source but "won't always be a perfect match".
  - Patches are editable in a DNA editor. Price €149, VST/AU. — [MusicRadar](https://www.musicradar.com/news/sonic-charge-synplant2-ai); [CDM](https://cdm.link/2023/10/the-road-to-synplant-2/) [verified]

### Inferences
- The state-of-the-art recipe for "learn from pro reference sounds" is a generative inverse model trained on synthetic renders of your own synth, followed by iterative audio-domain refinement. Flow matching handles one-to-many symmetry. For refinement, use gradients if the engine is differentiable, otherwise a neural proxy, evolutionary search or RL. Losses should combine audio-embedding terms (CLAP/MERT/codec embeddings) with multi-resolution spectral terms.
- Designing your own engine with a differentiable twin is a big advantage over targeting Serum/Vital as black boxes. A differentiable twin enables gradient refinement; a black box needs proxies or evolutionary search.
- Accuracy on real commercial presets (off-manifold) is the weakest point. Expect "close in character, not identical" results, which matches Synplant's own messaging.

### Gaps
- I found no published accuracy numbers (MSS, LSD, MOS, parameter MAE) for Serum/Vital-scale synths with hundreds of parameters, nor head-to-head benchmarks across papers. No common benchmark exists.
- I did not obtain the specific results tables for Hayes 2025/2026, Combes 2025, or SynthRL.

## 3. Neural audio synthesis: RAVE/BRAVE, neural codecs, latent diffusion for one-shots and drums, timbre transfer

### Takeaway
RAVE and its low-latency redesign BRAVE (JAES 2025: under 10 ms latency, about 3 ms jitter, proof-of-concept plugin) are the best options for real-time neural timbre transfer. Their fidelity is generally below commercial presets, and quality is sensitive to training data. Neural codecs (DAC, EnCodec, SNAC) give 44.1/48 kHz latents for generative models but are lossy at low bitrates. Latent diffusion (Stable Audio Open 1.0, and Stable Audio Open Small with 341M parameters and about 75 ms on an H100) can generate convincing 44.1 kHz stereo one-shots and loops offline, not per-note in real time. Dedicated one-shot drum generation research is thin: CRASH (2021) is still the main academic reference.

### Cited Findings
- **RAVE (Caillon & Esling, 2021):** a real-time convolutional VAE for 48 kHz audio, widely used for timbre transfer. — [arXiv 2111.05011](https://arxiv.org/abs/2111.05011) [prior knowledge]
- **BRAVE, "Designing Neural Synthesizers for Low-Latency Interaction" (Caspe, Shier, Sandler, Saitis, McPherson, JAES vol. 73 no. 5, 2025):** analyzes the sources of RAVE's latency and jitter and redesigns it. BRAVE preserves pitch and loudness better than RAVE with similar timbre modification. The NIME 2025 follow-up reports under 10 ms latency and about 3 ms jitter, with a proof-of-concept timbre-transfer plugin. — [arXiv 2503.11562](https://arxiv.org/abs/2503.11562v2); [AES](https://aes2.org/publications/elibrary-page/?id=22820); [NIME 2025 PDF](https://instrumentslab.org/data/andrew/caspe_nime2025.pdf) [verified]
- A Stanford student project training BRAVE found the low latency appealing but noisy training data hurt transfer quality. — [CCRMA 220C](https://ccrma.stanford.edu/~nshaheed/220c_sp25) [verified]
- **Neutone SDK (2025):** an open framework for wrapping PyTorch neural audio models as real-time plugins. — [arXiv 2508.09126](https://arxiv.org/pdf/2508.09126) [verified]
- **Real-time neural audio on Apple Silicon (DAFx 2026):** Apple's BNNSGraph (ahead-of-time compiled graph) is designed for audio-thread use. RTNeural is real-time safe but supports limited layer types. — [DAFx26 paper 16](https://dafx.de/paper-archive/2026/papers/DAFx26_paper_16.pdf) [verified]
- **Neural codecs, cross-paper comparisons:**
  - In one listening study, DAC at 16 kb/s beat EnCodec at 24 kb/s on music, but DAC struggled with tonal items such as glockenspiel. — [arXiv 2509.18823](https://arxiv.org/pdf/2509.18823) [verified via snippet]
  - EnCodec's 48 kHz model is stereo and music-trained. — same source.
  - On 150 stereo FMA tracks, SI-SDR was DAC 6.7 (8.9 kbps), EnCodec 8.7 (12 kbps) and SNAC 4.2 (4.8 kbps). — [VCNAC, arXiv 2601.14960](https://arxiv.org/pdf/2601.14960) [verified via snippet]
  - SNAC uses multi-scale RVQ, where coarse codebooks run at lower frame rates. Its speech MUSHRA is 88.4 at about 0.98 kbps vs DAC 85.0 at 2.5 kbps. — [arXiv 2410.14411](https://arxiv.org/pdf/2410.14411) [verified]
  - TQCodec targets high-bitrate 44.1 kHz music (32–128 kbps). — [arXiv 2603.01592](https://arxiv.org/html/2603.01592v1) [verified via snippet]
  - Original papers: DAC — [arXiv 2306.06546](https://arxiv.org/abs/2306.06546); EnCodec — [arXiv 2210.13438](https://arxiv.org/abs/2210.13438) [prior knowledge]
- **Stable Audio Open 1.0 (Stability AI, 2024):**
  - Architecture: VAE autoencoder, T5 text conditioning and a DiT working in latent space.
  - Generates up to 47 s of 44.1 kHz stereo audio; well suited to drum beats, riffs, foley and samples, but not full songs or vocals.
  - Can be fine-tuned on custom data.
  - License: Stability community license, with commercial terms via Stability. — [HF model card](https://huggingface.co/stabilityai/stable-audio-open-1.0); [W&B summary](https://wandb.ai/byyoung3/ml-news/reports/Stability-AI-Releases-Stable-Audio-Open--Vmlldzo4MjQyNTQ3); paper [arXiv 2407.14358](https://arxiv.org/abs/2407.14358) [prior knowledge]
- **Stable Audio Open Small (Stability AI + Arm, May 2025):**
  - 341M parameters (vs 1.1B), distilled.
  - Up to 11 s of audio generated in under 8 s on Armv9 phone CPUs, down from 240 s (about 30x).
  - About 75 ms for 44.1 kHz stereo on an H100.
  - Paper on arXiv, weights on HF, code on GitHub. These figures are vendor-reported. — [Stability AI](https://stability.ai/news/stability-ai-and-arm-release-stable-audio-open-small-enabling-real-world-deployment-for-on-device-audio-control); [The Decoder](https://the-decoder.com/stability-ai-and-arm-bring-offline-on-device-generative-audio-to-mobile-devices/) [verified]
- **CRASH (Rouard & Hadjeres, 2021):** score-based diffusion on raw audio for controllable, high-resolution drum one-shot synthesis. — [arXiv 2106.07431](https://ar5iv.arxiv.org/html/2106.07431) [verified]
- **DOSE (SNU, Apr 2025):** extracts one-shot drum sounds from full mixes using neural-codec language models, without source separation. It could be useful for building reference datasets from tracks. — [arXiv 2504.18157](https://arxiv.org/html/2504.18157v1) [verified]
- **tiny-audio-diffusion:** a hobbyist repo for waveform diffusion of about 0.75 s, 44.1 kHz stereo drum one-shots. — [repo mirror](https://git.tdem.in/crlandsc/tiny-audio-diffusion) [verified]
- **Commercial drum generation:** Fadr's DrumGPT is a DAW plugin that generates percussive one-shots; this is a vendor/review claim. — [AudioCipher](https://www.audiocipher.com/post/fadr-drumgpt-ai-drum-generator) [verified, secondary]

### Inferences
- Pure neural synthesis cannot yet match a well-programmed wavetable synth for clean, phase-coherent, alias-free bass and lead tones at low latency. Its strengths are drums, FX, textures and one-shots generated offline, which then become sampler or wavetable content.
- A hybrid design is the realistic path. A diffusion or codec model generates or transforms one-shots offline in a background thread. A deterministic DSP or sampler engine plays them back in real time, optionally with a BRAVE-style low-latency neural layer for timbre effects.

### Gaps
- I found no rigorous listening test comparing neural-generated EDM sounds with commercial preset/sample packs.
- No 2025–2026 peer-reviewed dedicated drum one-shot generator (text- or reference-conditioned) was found beyond CRASH and Stable Audio fine-tunes.
- I found no streaming-latency figures for the DAC/SNAC codecs.

## 4. Generative models conditioned on reference audio or embeddings (CLAP, MERT); text-to-sample

### Takeaway
CLAP is the de facto conditioning and scoring embedding. It is used as the objective for CTAG text-to-synth and as the conditioning or eval space in text-to-audio. Neural proxies map presets into pretrained audio-embedding spaces, which enables "match this reference" in embedding space. Text-to-sample works today via Stable Audio Open and its fine-tunes. Conditioning directly on a reference audio embedding for one-shot synth sounds is still underexplored in the literature I found.

### Cited Findings
- **LAION-CLAP:** contrastive audio-text embeddings. — [arXiv 2211.06687](https://arxiv.org/abs/2211.06687) [prior knowledge]
- CTAG uses LAION-CLAP as the fitness function for optimizing synth parameters from text. — [PMLR](https://proceedings.mlr.press/v235/cherep24a.html) [verified]
- **MERT:** a self-supervised music representation model, suitable as a timbre/music embedding for losses or retrieval. — [arXiv 2306.00107](https://arxiv.org/abs/2306.00107) [prior knowledge]
- Neural proxies map presets into pretrained audio-model embedding spaces and compare several pretrained audio models as targets. — [arXiv 2509.07635](https://arxiv.org/abs/2509.07635) [verified]
- FM-SynAPSE builds a shared audio-preset embedding (SLAP objective) for preset retrieval. — [arXiv 2608.18226](https://arxiv.org/pdf/2608.18226) [verified]
- Codec embedding distances are proposed as metrics for evaluating generative audio. — [arXiv 2509.18823](https://arxiv.org/pdf/2509.18823) [verified]
- Stable Audio Open supports text-to-sample at 44.1 kHz stereo and can be fine-tuned on custom data. — [HF](https://huggingface.co/stabilityai/stable-audio-open-1.0) [verified]
- P-MUSE (2026) is a prompt- and MIDI-optional model for unified instrumental synthesis and editing. — [arXiv 2608.01920](https://arxiv.org/pdf/2608.01920) [verified, title/abstract-level only]

### Inferences
- CLAP is semantic: it captures "dubstep growl" or "techno rumble kick" well but captures fine timbral detail weakly. MERT, codec-latent or multi-resolution STFT losses should complement it for close matching.
- A "reference-sound → embedding → preset/sample" product is technically feasible with existing open code: CTAG, synth-proxy and the Hayes flow-matching approach.

### Gaps
- I found no paper on CLAP- or MERT-conditioned diffusion specifically for synth one-shots in EDM genres.
- No genre-specific (dubstep/techno/trap) datasets or benchmarks were found.

## 5. Which approaches produce audio competitive with commercial presets at 44.1/48 kHz?

### Takeaway
The only approaches that reach commercial preset quality today render through a real DSP synth engine. That covers parameter inference into a high-quality synth (Synplant 2, Surge XT in Hayes et al., Vital/Serum community tools) and differentiable-DSP fitting exported to conventional DSP. For drums, FX and loops, latent diffusion (Stable Audio Open) produces usable 44.1 kHz stereo material offline. RAVE/BRAVE-style real-time neural synthesis trades fidelity for latency and data sensitivity.

### Cited Findings
- Hayes et al. evaluate inversion on Surge XT, a full-featured open-source synth, so output quality equals that synth's DSP. — [arXiv 2506.07199](https://arxiv.org/abs/2506.07199v1) [verified]
- Synplant 2 ships audio-to-patch matching in a commercial VST/AU with local CPU inference. — [MusicRadar](https://www.musicradar.com/news/sonic-charge-synplant2-ai) [verified]
- Modulation discovery tests on modulation curves from Vital presets and ships DDSP VST plugins. — [arXiv 2510.06204](https://arxiv.org/abs/2510.06204) [verified]
- Stable Audio Open generates 44.1 kHz stereo audio suited to samples and drums. — [HF](https://huggingface.co/stabilityai/stable-audio-open-1.0) [verified]
- Real-time DDSP plugin timbre transfer scored lower than offline demos. — [EmergentMind](https://www.emergentmind.com/topics/ddsp-framework) (secondary)
- BRAVE quality is sensitive to noisy training data. — [CCRMA 220C](https://ccrma.stanford.edu/~nshaheed/220c_sp25) [verified]
- DAC has artifacts on tonal content at low bitrates, which matters for codec-latent generative models. — [arXiv 2509.18823](https://arxiv.org/pdf/2509.18823) [verified via snippet]

### Inferences
- "Finished, release-ready" EDM character comes heavily from post-processing: unison stacks, distortion, OTT multiband compression, EQ and reverb. Any matching system should therefore include a differentiable or proxied FX chain (NablAFx-style), not just the oscillator, filter and envelope core.

### Gaps
- I found no published MOS or ABX tests against commercial presets (e.g., Serum factory banks) for any method.

## 6. Promising research NOT yet productized (2024–2026, open code) and novel combinations for a new kind of instrument

### Takeaway
Several 2024–2026 methods have working code but, as far as I found, no commercial plugin:
- Hayes's equivariant and off-manifold flow-matching inversion
- Combes/Braun neural proxies for black-box synths
- Mitcheltree's modulation discovery (research VST only)
- CTAG text-to-synth
- FM-SynAPSE shared embeddings
- Differentiable wavetable learning
- BRAVE low-latency timbre transfer (proof-of-concept plugin only)
- Differentiable-graph-to-FAUST compilation

Synplant 2 is the main commercial precedent, and it covers only audio-to-patch on a simple synth engine.

### Cited Findings (unproductized research with code)
- **Equivariant flow-matching synth inversion** (ISMIR 2025 best student paper), plus the off-manifold extension (ISMIR 2026) aimed at real-world references. — [arXiv 2506.07199](https://arxiv.org/abs/2506.07199v1); [arXiv 2609.29320](https://arxiv.org/pdf/2609.29320)
- **Neural proxies:** differentiable stand-ins for arbitrary VSTs, with code. — [GitHub synth-proxy](https://github.com/DBraun/synth-proxy)
- **Modulation discovery:** recovers LFO and envelope curves from audio; research VSTs released. — [arXiv 2510.06204](https://arxiv.org/abs/2510.06204)
- **CTAG:** text to editable synth patch via CLAP plus evolutionary search; code open. — [GitHub](https://github.com/papayaresearch/ctag)
- **FM-SynAPSE:** a shared audio-preset embedding for retrieval, with HF weights. — [HF](https://huggingface.co/davidbraun/fm-synapse-dx7-gnn)
- **Differentiable wavetable synthesis:** learns compact wavetable dictionaries from audio. — [arXiv 2111.10003](https://arxiv.org/pdf/2111.10003)
- **BRAVE:** a proof-of-concept low-latency timbre-transfer plugin. — [NIME 2025](https://instrumentslab.org/data/andrew/caspe_nime2025.pdf)
- **Differentiable-graph-to-FAUST compiler** with hot-reload during training. — [arXiv 2606.21277](https://arxiv.org/pdf/2606.21277)
- **DOSE:** one-shot drum extraction from mixes, useful for mining reference one-shots from tracks. — [arXiv 2504.18157](https://arxiv.org/html/2504.18157v1)
- **Stable Audio Open Small:** a fast, distilled text-to-audio model that can run on-device inside a plugin. — [Stability AI](https://stability.ai/news/stability-ai-and-arm-release-stable-audio-open-small-enabling-real-world-deployment-for-on-device-audio-control)

### Inferences (novel combinations; these are design ideas, not established results)
1. **"Reference-to-editable-patch" engine with a differentiable twin.** Build a Serum/Vital-class wavetable engine with C++ DSP for playback and a PyTorch/JAX twin.
   - Amortized prediction: a flow-matching inverse model (Hayes) proposes several candidate patches, which naturally gives "variations".
   - Refinement: gradient descent on the twin with modulation discovery (Mitcheltree) and learned wavetables (DWTS). The output is a fully editable preset, not a sample.
   - Goes beyond Synplant by targeting a modern wavetable/FX engine and by handling off-manifold references.
2. **Learned FX chain as part of the patch.** Extend inversion to a differentiable post-chain (distortion, OTT-style compression, EQ, reverb, NablAFx-style). Release-ready EDM sounds depend heavily on processing.
3. **Embedding-space "sound steering".** Use a shared audio-preset embedding (FM-SynAPSE/SLAP, neural proxies) plus CLAP/MERT so users can interpolate between reference sounds, or move along text directions ("more aggressive", "darker"), while the synth stays parametric and editable. CTAG shows the text-side optimization works.
4. **Hybrid generative layer.** A background-thread distilled diffusion model (SAO Small-class) generates transient or noise layers: kick clicks, risers, foley textures. These are layered with the parametric tonal engine and can be resynthesized into wavetables via DWTS.
5. **Real-time neural "character" stage.** A BRAVE-class low-latency neural timbre layer as an insert inside the voice path, deployed with RTNeural or BNNSGraph-type AOT runtimes for audio-thread safety.
6. **Dataset bootstrapping.** DOSE-style extraction plus the user's own reference library produces per-genre target sets; torchsynth/SynthAX-style GPU rendering produces millions of synthetic training pairs for the inverse model.

### Gaps
- I could not exhaustively verify that no commercial plugin implements each method. "Not productized" is based on not finding one in these searches.
- Licensing of weights and code (e.g., Stable Audio community license, synth-proxy, ctag) needs per-repo checks before commercial use.
