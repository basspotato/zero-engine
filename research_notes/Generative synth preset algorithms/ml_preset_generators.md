# ML models that generate synthesizer presets (parameters, not audio)

Scope note: about 17 web searches, snippets only (no full-text fetch tool was available). Numbers below are what the abstracts, project pages and press said; table-level results (spectral convergence, MSE etc.) were not retrieved and are listed under Gaps.

## 1. What approaches exist and how good are they (FlowSynth, preset VAEs, transformers, diffusion/flow matching, text-to-preset, judge-ranked random generation, proxies, vendor features)?

### Takeaway
Almost all academic "preset generation" work is really **synth inversion** (audio to parameters) or **preset interpolation**. The models are VAEs/flows (2019-2022), transformer autoencoders (2022-2024), and, since 2025, conditional generative models (equivariant flow matching, masked discrete diffusion), fine-tuned on rendered-audio rewards. Unconditioned "make me a good new patch" generation shows up mainly in products: Synplant 2's PhenoType (text/tags to patch from about 60k hand-tagged patches) and Genopatch, plus a wave of LLM-writes-preset-JSON tools for Vital and Serum 2. Text-to-preset without training (CTAG) works by gradient-free search scored by CLAP, not by a generator network.

### Cited Findings
**VAE / normalizing flows (FlowSynth, Esling et al., IRCAM ACIDS)**
- "Universal audio synthesizer control with normalizing flows" (DAFx 2019, arXiv 1907.00971) learns a structured latent audio space tied by an invertible mapping (normalizing flows) to the synth's parameters. One model handles parameter inference from audio, macro-control learning and preset exploration, and it beat a large set of baselines on parameter inference and audio reconstruction — [DAFx archive](https://dafx.de/paper-archive/details/cur8Axp8MOYa1QzdZW_Nug); [arXiv](https://arxiv.org/abs/1907.00971v1); [project page](https://acids-ircam.github.io/flow_synthesizer)
- The arXiv and DAFx versions describe the flow differently ("disentangling flows" vs "regression flows") — [arXiv](https://arxiv.org/abs/1907.00971v1); [DAFx](https://dafx.de/paper-archive/details/cur8Axp8MOYa1QzdZW_Nug)
- Training target: u-he Diva, about 11k synthesized sounds with known preset values, according to a later review — [Audio representations review, arXiv 2201.02490](https://arxiv.org/pdf/2201.02490)
- The FlowSynth demo (IJCAI 2020) adds preset visualisation and interpolation, real-time use, voice control and LeapMotion navigation of the latent space — [IJCAI 2020](https://www.ijcai.org/proceedings/2020/767); [mlanthology](https://mlanthology.org/ijcai/2020/esling2020ijcai-flowsynth); [ACIDS page](https://acids.ircam.fr/flowsynth)
- Name collision: a separate 2025 "FlowSynth" (Smule/UCSD, arXiv 2510.21667) does instrument *audio* generation by distributional flow matching plus test-time search, trained on NSynth (305,979 notes). It is not a preset model — [arXiv 2510.21667](https://arxiv.org/pdf/2510.21667)

**Le Vaillant & Dutoit preset VAEs (UMONS)**
- DAFx 2021, "Improving synthesizer programming from variational autoencoders latent space". A spectral VAE with multi-channel (multi-note) spectrogram input infers pitch- and velocity-dependent parameters. Parameters are modelled as mixed numerical and categorical variables. It reports significant gains in parameter inference and audio accuracy, on full or partial parameter sets — [DAFx archive](https://dafx.de/paper-archive/details/lHfxRikaTqQ0X8tXETM6TQ); [PDF](https://orbi.umons.ac.be/bitstream/20.500.12907/43332/1/DAFx20in21_paper_7.pdf)
- Data: Dexed (DX7 clone), a published database of more than 30k presets, 144 learnable parameters each — [preset-gen-vae site](https://gwendal-lv.github.io/preset-gen-vae); [repo](https://gh.hhj520.top/gwendal-lv/preset-gen-vae)
- ICASSP 2023 (SPINVAE, arXiv 2210.16984): a transformer autoencoder for preset interpolation. It was smoother than linear per-parameter interpolation, with 35 audio features significantly smoother and average smoothness 12.6 % better. Linear per-parameter interpolation driven by a macro is called the de facto baseline — [arXiv 2210.16984](https://arxiv.org/pdf/2210.16984); [SPINVAE demo](https://gwendal-lv.github.io/spinvae)
- IEEE TASLP 2024 (SPINVAE-2): a VAE with multi-head self-attention and timbre-attribute regularisation, for black-box non-differentiable synths. Evaluation uses linearity and smoothness metrics, checked against listening tests — [TASLP accepted PDF](https://orbi.umons.ac.be/bitstream/20.500.12907/49507/1/taslp24_accepted.pdf); [SPINVAE-2 demo](https://gwendal-lv.github.io/spinvae2); [TechRxiv](https://www.techrxiv.org/doi/pdf/10.36227/techrxiv.170327281.10174081/v4)

**Flow matching / diffusion over parameters (2025-2026)**
- Hayes, Saitis, Fazekas, "Audio synthesizer inversion in symmetric parameter spaces with approximately equivariant flow matching" (ISMIR 2025, best student paper). Many settings give the same sound (for example, permuted oscillators), so point regression averages across them. Treating equivalent solutions as modes of a distribution with a conditional generative model helps, a permutation-equivariant continuous normalizing flow helps further, and a relaxed equivariance learns the symmetries from data. On Surge XT it beat regression and generative baselines on audio metrics — [arXiv 2506.07199](https://arxiv.org/abs/2506.07199v1); [ISMIR 2025 poster](https://ismir2025program.ismir.net/poster_7.html); [QMUL award news](https://c4dm.eecs.qmul.ac.uk/news/2025-09-30.C4DM-ISMIR_Awards/)
- Hayes, "Off-manifold robustness in synthesizer inversion with joint distribution flow matching" (to appear, ISMIR 2026). It targets the core limit of render-trained models: paired data can only come from the synth's own output, so real audio off that manifold breaks them. The method and results were not retrieved — [arXiv 2609.29320](https://arxiv.org/pdf/2609.29320)
- DDSynth-RL (ISMIR 2026, arXiv 2608.03032): masked **discrete diffusion** over (quantised) Dexed parameters. The supervised model is competitive with autoregressive and flow-matching baselines, and GRPO fine-tuning on rewards computed from rendered audio improves out-of-domain matching. Code and checkpoints are public; Dexed renders the rewards — [arXiv](https://arxiv.org/pdf/2608.03032); [HF checkpoints](https://huggingface.co/MINNE-WU/DDSynth-RL)
- SynthRL: parameter-loss pretraining, then RL fine-tuning on out-of-domain sounds with a sound-similarity reward and no labels — [Lacuna summary](https://lacuna.tiptreesystems.com/work/synthrl-cross-domain-synthesizer-sound-matching-via-reinforcement-learning/wrk_29c597a6c4ffd5e4a8ffa11a26fd8ec8)
- No 2023-2026 paper turned up that does *unconditional* or *tag-conditioned* diffusion/flow generation of new presets (as opposed to inversion) — search result, see Gaps.

**Earlier inversion baselines (context)**
- InverSynth (TASLP 2019): strided CNNs from spectrogram or raw audio to the parameters of a subtractive synth with 4 FM oscillators, an envelope and a gater — [arXiv 1812.06349](https://arxiv.org/pdf/1812.06349)
- Sound2Synth (IJCAI 2022): Prime-Dilated Convolution on Dexed, claimed as the first real-world applicable results — [IJCAI](https://www.ijcai.org/proceedings/2022/682)
- Masuda & Saito (ISMIR 2021): pretrain with parameter loss on random in-domain renders, then fine-tune with spectral loss through a DDSP synth on out-of-domain audio — [ISMIR 2021 PDF](https://archives.ismir.net/ismir2021/paper/000053.pdf); [semi-supervised follow-up](https://hyakuchiki.github.io/SSSSM-DDSP/)
- NI/Massive sound matching with an Audio Spectrogram Transformer (2024): 16 parameters, trained on random renders — [arXiv 2407.16643](https://export.arxiv.org/pdf/2407.16643); [DAFx24 paper 95](https://dafx.de/paper-archive/2024/papers/DAFx24_paper_95.pdf)

**Synth proxies / preset embeddings**
- Combes, Weinzierl, Obermayer, "Neural Proxies for Sound Synthesizers" (arXiv 2509.07635, JAES). A "preset encoder" maps a preset straight into a frozen pretrained audio model's embedding space, giving a differentiable stand-in for a black-box synth. It compares MLP, RNN and transformer proxies on 3 synths, trained on 10M synthetic presets, and gives encouraging sound-matching results with gains "nuanced by resource requirements". Code is public (PyTorch) — [arXiv](https://arxiv.org/abs/2509.07635v1); [HTML](https://arxiv.org/html/2509.07635v1); [GitHub DBraun/synth-proxy](https://github.com/DBraun/synth-proxy)

**Text-to-preset without a trained generator**
- CTAG (Cherep, Singh, Shand, ICML 2024): a 78-parameter virtual modular synth. Gradient-free optimisation maximises LAION-CLAP text-audio similarity; no new model is trained. Outputs are easy to inspect and tweak — [arXiv 2406.00294](https://arxiv.org/abs/2406.00294); [PMLR](https://proceedings.mlr.press/v235/cherep24a.html); [MIT Media Lab](https://www.media.mit.edu/projects/ctag/overview/)
- A third-party review says CTAG uses Evosax population-based evolution because gradients through its differentiable synth were unstable. The exact optimiser (CMA-ES or another) was not verified — [Moonlight review](https://www.themoonlight.io/tw/review/creative-text-to-audio-generation-via-synthesizer-programming)
- SynthScribe: CLAP-based text search and modification of existing Diva presets (retrieval, not generation) — [arXiv 2312.04690](https://arxiv.org/pdf/2312.04690)

**LLM-writes-the-preset tools (2024-2026, mostly hobby or commercial)**
- Serum Agent: an LLM outputs parameter JSON, which is written to .SerumPreset and read back to check. It does basses, pads, leads, plucks, keys, arps, drones and FX — [christianjtapp.com](https://www.christianjtapp.com/blog/serum-agent)
- synth-rl: audio-LLM agents program Vital from text or a reference sound, listen to their own render and refine it (2 rounds); it includes a benchmark and GRPO training. Self-described as early — [GitHub luwux/synth-rl](https://github.com/luwux/synth-rl)
- sound-to-patch (Vital, Serum 2), Prompt2Patch (Vital), PresetLab.ai (Diva, Serum 2): marketing claims, unverified — [sound-to-patch](https://github.com/Akurganow/sound-to-patch); [Prompt2Patch](https://prompt2patch.com/); [PresetLab](https://presetlab.ai/)
- Serum presets are an opaque binary chunk, which makes them harder to generate than Vital's JSON — [0xdevalias notes gist](https://gist.github.com/0xdevalias/5a06349b376d01b2a76ad27a86b08c1b)

**Products**
- Sonic Charge PhenoType (Synplant 2, free script, 2026): text to an editable Synplant seed, offline. Its neural net uses about 200 internal tags and about 1,000 synonyms and was trained on almost 60,000 patches hand-tagged by Lidström. It shows a confidence score for how close each result is to the description, and an RND button suggests a random prompt — [CDM](https://cdm.link/synplant-phenotype/); [MusicRadar](https://www.musicradar.com/music-tech/is-synplant-2s-new-prompt-based-patch-generator-the-future-of-synthesis); [Sonicstate](https://sonicstate.com/news/2026/06/11/generate-synplant-patches-from-text/). CDM calls it "a randomizer with intention". The figures all trace back to one announcement.
- Synplant 2 Genopatch: audio (up to 2 s, mono) to patch. It combines several ML algorithms tuned over about 10 years, its networks are trained on semi-random sounds rendered by the synth itself, it runs locally, and it shows candidate patches as branches to audition. Synthetic sources match best — [MusicRadar review](https://www.musicradar.com/reviews/sonic-charge-synplant-2-review); [MusicRadar news](https://www.musicradar.com/news/sonic-charge-synplant2-ai); [MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/)
- Vital: "text to wavetable" (Pro tier, needs internet) turns a phrase into a wavetable, not into a preset. A student team's AI preset generator for Vital (by sound type and genre) was a time-limited website — [MusicTech](https://musictech.com/news/matt-tytel-vital-free-synth/); [Vital forum](https://forum.vital.audio/t/we-made-an-ai-preset-generator-for-vital/15666)
- Arturia, NI, Serum: no AI preset-generation feature found in this search (see Gaps).

### Inferences
- For a ~60-parameter subtractive/wavetable synth, the two most relevant generator families are (a) a conditional VAE/flow over presets with mixed continuous/categorical heads (Le Vaillant), and (b) masked discrete diffusion over quantised parameters plus render-reward fine-tuning (DDSynth-RL). Both are small models by modern standards.
- PhenoType and Genopatch are the closest analogue to Zero Engine's Generate. A local net trained on the synth's own patches proposes, a confidence score ranks, and the user picks among several candidates. That pattern (generator + judge + several candidates) matches the existing render-and-check Generate.
- CTAG is a reminder that "search scored by a frozen audio embedding" competes with trained generators when the synth is fast to render, which Zero Engine is.

### Gaps
- Quantitative tables (spectral convergence, parameter MSE, MOS) for FlowSynth, the Le Vaillant VAEs, DDSynth-RL and Hayes 2025: not retrieved (no full-text access).
- PhenoType's architecture (classifier ranking random patches vs a direct generator) is not public. Press only mentions "neural network", tags and a confidence score.
- No primary source found for Arturia, NI or Serum 2 AI preset generation; none may exist.
- No "SynthGPT"-named model was found.

## 2. How do models condition on category/genre/tags and avoid bland "average" presets?

### Takeaway
Blandness is a known, named failure. Regression (MSE to parameters) averages across the many settings that give the same sound, and the best fix in the literature is to model a *distribution* (flows, diffusion, VAEs sampled rather than decoded at the mean), ideally aware of symmetries. Tag conditioning is documented mainly in PhenoType (hand-tagged data). Academic work conditions on audio or timbre attributes, not genre.

### Cited Findings
- Hayes et al.: point regression struggles because equivalent solutions (for example, permuted oscillators) are averaged. Treating them as modes of a distribution with a conditional generative model "substantially improves performance", and equivariant flows improve it further — [arXiv 2506.07199](https://arxiv.org/abs/2506.07199v1); [ar5iv](https://ar5iv.labs.arxiv.org/html/2506.07199)
- DDSynth-RL names the same many-to-one problem as its first challenge and answers it with a generative (discrete diffusion) model — [arXiv 2608.03032](https://arxiv.org/pdf/2608.03032)
- SPINVAE-2 regularises latent dimensions to timbre attributes, a form of attribute-aligned control — [TASLP PDF](https://orbi.umons.ac.be/bitstream/20.500.12907/49507/1/taslp24_accepted.pdf)
- FlowSynth learns macro controls and semantic dimensions in its latent space — [ACIDS](https://acids.ircam.fr/flowsynth)
- PhenoType conditions on about 200 tags and 1,000 synonyms learned from about 60k hand-tagged patches and reports a per-result confidence — [CDM](https://cdm.link/synplant-phenotype/); [MusicRadar](https://www.musicradar.com/music-tech/is-synplant-2s-new-prompt-based-patch-generator-the-future-of-synthesis)
- The Vital student generator conditioned on "sound types and genres" (no technical detail) — [Vital forum](https://forum.vital.audio/t/we-made-an-ai-preset-generator-for-vital/15666)
- Render-reward fine-tuning (GRPO in DDSynth-RL and synth-rl, RL in SynthRL) pushes a generator towards outputs that *sound* right, not towards parameter averages — [DDSynth-RL](https://arxiv.org/pdf/2608.03032); [synth-rl](https://github.com/luwux/synth-rl); [SynthRL](https://lacuna.tiptreesystems.com/work/synthrl-cross-domain-synthesizer-sound-matching-via-reinforcement-learning/wrk_29c597a6c4ffd5e4a8ffa11a26fd8ec8)

### Inferences
- For Zero Engine, a genre/type-conditioned model should be *sampled* (for example, conditional VAE sampled away from the mean with temperature above 1, discrete diffusion, or a flow), never a regressor. Its outputs should then go through the existing render-and-check plus trait scoring, which acts as the "judge" that throws out bland or broken candidates.
- The owner's favourites, history keeps and rejects can serve as a preference reward (a GRPO-style or simple reweighting) on top of a generator trained on renders.

### Gaps
- No paper found that measures preset diversity or mode collapse for tag- or genre-conditioned preset generation.
- No published method for "interesting, not average" beyond sampling plus judge. Human-in-the-loop preferential Bayesian optimisation exists for melodies and image-model merging, not synth presets — [arXiv 2010.03190](https://arxiv.org/pdf/2010.03190); [GimmBO arXiv 2601.18585](https://arxiv.org/html/2601.18585v1); [Lin et al. 2203.11382](https://ar5iv.labs.arxiv.org/html/2203.11382)

## 3. How much data is needed; does training on the synth's own random renders work; small-data tricks?

### Takeaway
Own renders are the standard training source (Genopatch, Hayes, DDSynth-RL, NI Massive, synth-proxy). They are free and unlimited, but uniformly random parameters land far from musical presets, and models trained only on them transfer poorly to hand-made sounds. Known fixes: sample parameters from the distribution of real presets, pretrain on parameter loss then fine-tune with audio, embedding or reward losses, and use symmetry-aware or generative heads. Dataset sizes range from about 11k (FlowSynth/Diva) and 30k (Dexed) curated presets to 10M random presets (proxies).

### Cited Findings
- FlowSynth: about 11k Diva sounds with known presets — [review arXiv 2201.02490](https://arxiv.org/pdf/2201.02490)
- Le Vaillant: more than 30k Dexed presets, 144 parameters — [preset-gen-vae](https://gwendal-lv.github.io/preset-gen-vae)
- PhenoType: about 60k hand-tagged Synplant patches — [CDM](https://cdm.link/synplant-phenotype/)
- Synth proxies: 10M random presets. Proxies trained on millions of random presets transfer poorly to hand-crafted presets, especially for FM; the authors suggest (untested) sampling training parameters to resemble human presets — [Pith summary of arXiv 2509.07635](https://pith.science/paper/2509.07635); [arXiv](https://arxiv.org/abs/2509.07635v1)
- The NI Massive AST paper samples parameters from the factory-preset parameter distribution rather than uniformly, to stay in the usable part of the space. Random sampling gives "arbitrarily large, fully-labeled datasets" — [arXiv 2407.16643](https://export.arxiv.org/pdf/2407.16643); [DAFx24 paper 95](https://dafx.de/paper-archive/2024/papers/DAFx24_paper_95.pdf)
- Genopatch: networks trained on semi-random sounds from Synplant's own engine — [MusicRadar review](https://www.musicradar.com/reviews/sonic-charge-synplant-2-review)
- Hayes 2026: render-only training restricts models to the synth's output manifold, the motivation for joint-distribution flow matching — [arXiv 2609.29320](https://arxiv.org/pdf/2609.29320)
- Pretrain with parameter loss on random renders, then fine-tune with spectral loss (Masuda & Saito) or with RL or similarity rewards (SynthRL, DDSynth-RL). A 2025 study found that adding an audio-embedding loss to parameter loss improved out-of-domain matching — [ISMIR 2021](https://archives.ismir.net/ismir2021/paper/000053.pdf); [SynthRL](https://lacuna.tiptreesystems.com/work/synthrl-cross-domain-synthesizer-sound-matching-via-reinforcement-learning/wrk_29c597a6c4ffd5e4a8ffa11a26fd8ec8); [DDSynth-RL](https://arxiv.org/pdf/2608.03032); [arXiv 2506.22628](https://arxiv.org/pdf/2506.22628)

### Inferences
- Zero Engine already has the ideal non-uniform sampler: the 72 profiles plus `generateProfile` variations. Renders of those (filtered by the existing check and trait scoring) are a "preset-like" distribution, which avoids the random-preset transfer gap the proxy paper found. Tens of thousands of renders are cheap offline (the 30k-60k scale of Dexed and PhenoType is reachable in hours).
- One user's choices (favourites and history: hundreds, not thousands) are too few to train a generator from scratch. They suit a reranker or reweighting of a render-trained generator, or a small preference model on SoundTraits features.

### Gaps
- No study found on the minimum data for a ~60-parameter subtractive synth specifically.
- No study found on learning from a single user's keep/reject choices for synth presets.

## 4. Running small nets in a C++ plugin (RTNeural, ONNX Runtime) and inference cost

### Takeaway
Small MLPs and transformers for parameter generation are easy to run on CPU. ONNX Runtime is fastest for stateless models, RTNeural is competitive for small recurrent ones, and all engines can occasionally overrun deadlines. Since preset generation is not per-sample audio, it belongs on a background thread (as Generate already does), so real-time safety is a non-issue.

### Cited Findings
- ANIRA (2025): ONNX Runtime has the lowest runtimes for stateless models and LibTorch for stateful ones. Every engine showed real-time violations, which ANIRA mitigates by moving inference off the audio callback to a static thread pool — [arXiv 2506.12665](https://arxiv.org/pdf/2506.12665)
- DAFx comparison on an embedded CPU (TFLite, TorchScript, ONNX Runtime, RTNeural): all can run in real time with good code practices. The general-purpose engines were slightly better than the real-time-specific one (RTNeural) for audio classification — [DAFx archive](https://www.dafx.de/paper-archive/details/qcQo4dIbC0k5_sjLxWUmKA)
- DAFx 2026 Apple Silicon benchmark (BNNSGraph, RTNeural, LibTorch, ONNX Runtime, ANIRA): isolated benchmarks mislead under DAW load — [DAFx archive](https://dafx.de/paper-archive/details/rBxXewYH3YxRTiGXqSabwQ); [DAFx26 paper 18](https://www.dafx.de/paper-archive/2026/papers/DAFx26_paper_18.pdf)
- PhenoType and Genopatch both run fully locally inside or alongside the plugin — [MusicRadar](https://www.musicradar.com/music-tech/is-synplant-2s-new-prompt-based-patch-generator-the-future-of-synthesis); [MusicRadar review](https://www.musicradar.com/reviews/sonic-charge-synplant-2-review)

### Inferences
- A ~60-parameter preset generator (an MLP or VAE decoder of a few hundred thousand weights, or a small 6-layer discrete-diffusion transformer over about 60 tokens) needs only milliseconds per sample on CPU. Rendering and checking each candidate (Zero Engine's existing step) will cost far more than inference. Hand-written C++ weights or RTNeural (BSD-3) avoid the ONNX Runtime DLL's size; ONNX Runtime (MIT) is fine if a transformer is used.
- Licences: RTNeural BSD-3, ONNX Runtime MIT (permissive). The preset-gen-vae, synth-proxy and DDSynth-RL repo licences were not checked, so verify before reusing code; offline-only use avoids the GPL issue either way.

### Gaps
- No benchmark found specifically for preset-generator-sized models in plugins; the cost estimate above is an inference, not measured.
- Licences of the research repos were not confirmed.
