# Teach a synth to finish its own sounds

The best way to build this instrument is not to train a model that generates audio. Build a **parametric synth engine with its own differentiable twin**, and fuse two research lines that no shipping product combines. The first is **generative synth inversion**: flow-matching models that propose editable patches from a target. The second is **audio-effects style transfer**: models that predict or search the settings of a mastering-style DSP chain from a style reference. A compact, numbers-only "style profile" database sits underneath both. It is built from per-genre, per-role measurements of loudness, crest factor, spectral tilt, per-band width and learned production-style embeddings. The neural networks never touch the audio thread except as tiny models. They choose parameters; deterministic C++ DSP makes the sound, which is the only approach that reaches commercial preset quality today. The market gap is real but narrow. Unison's **Unisynth** ($197, March 2026) already sells "pick a genre, generate a patch". Ozone, LANDR and RoEx already sell reference-matched mastering. But nobody puts mastering-grade, reference-learned finishing *inside the voice* of a playable instrument. Nobody matches a sound to a reference at the sound-source level. Nobody guarantees per-user uniqueness against the main complaint about AI patch generators, which is "the same 1,000 presets". The legal design follows the same logic: render your own training data, store reference analysis only as statistics, and never train a generative model on sample packs whose licences now explicitly forbid it.

## Patch generators and mastering assistants exist, but nothing joins them

The AI sound-design field splits into three camps, and each has a known ceiling.

**Patch generators** drive a conventional synth engine:

- **Unisynth** bills itself as the "world's first (and only) AI-powered generative synth that's genre-specific". It offers 32 genres and 6 sound types, all reached through a central Patch Generator ([Unison](https://unison.audio/unisynth-out-now/); [Sonicstate](https://sonicstate.com/news/2026/03/20/ai-powered-generative-synth)). Users praise its instant polish. One says it "feels more like a combination of genre-based algorithms and a preset generator" ([Trustpilot](https://ie.trustpilot.com/review/unison.audio)).
- **Spire's Reveal Sound AI** requires an online login and caps generations per day. KVR users describe it as a random draw from a fixed pool of presets ([Synth Anatomy](https://synthanatomy.com/2024/09/reveal-sound-spire-synthesizer-new-1-5-17-update-adds-ai-preset-generator.html); [KVR](https://www.kvraudio.com/forum/viewtopic.php?p=8963087)).
- **Synplant 2's Genopatch** is the one shipping audio-to-patch product. It runs locally and iterates toward an imported sound, but it is limited to Synplant's own simple engine, converts input to mono, and is weak on sounds that are not synths ([MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/); [MusicRadar](https://www.musicradar.com/reviews/sonic-charge-synplant-2-review)).

**Neural-audio generators** produce clips, not instruments, and most are cloud-bound:

- **Stable Audio's DAW plugin** is an early beta: macOS-only, account-gated and paid in credits ([Stability KB](https://kb.stability.ai/knowledge-base/stable-audio-daw-plugin-guide)).
- **Magenta's Infinite Crate** needs an API key and caps sessions at 10 minutes ([Magenta](https://magenta.tensorflow.org/infinite-crate)).

**Retrieval tools** such as Output Co-Producer and Splice Create listen to the project but only fetch library samples. They do not synthesize anything ([Sound On Sound](https://www.soundonsound.com/reviews/output-co-producer); [MusicTech](https://musictech.com/news/gear/splice-ai-create/)).

The mastering side has converged on one definition of "engineer style": **a reference tonal-balance curve plus loudness, dynamics and width targets**, steering a fixed chain. Ozone 12's Master Assistant does this ([iZotope](https://www.izotope.com/en/learn/how-to-use-master-assistant-in-ozone.html)), as do RoEx Reference Match ([RoEx](https://www.roexaudio.com/mastering)) and LANDR. Of these, only LANDR claims training on engineers' work, and it exposes that only as coarse styles such as "Warm" ([LANDR](https://blog.landr.com/reference-mastering/)). Every one of them works on full mixes. None sits inside a synth, and none operates on a single sound.

Artist-chain plugins like KSHMR Essentials show that macro-only finishing sells on speed. They also show its failure mode: reviewers complain that only the intensity of each effect can be set ([Bonedo](https://www.bonedo.de/artikel/kshmr-essentials-kick-test)). And producers are visibly tired of preset packs, as shown by Serum 2 packs that are "just ported Serum 1 presets" ([r/edmproduction](https://lr.eu.psf.lt/r/EDMproduction)).

The pattern across all these complaints is consistent: **sameness, cloud lock-in, the "just presets" perception, engine ceilings, and output that cannot be played**. That defines the unoccupied space precisely. It is a *local*, *playable*, *editable* instrument that learns from *reference sounds* and *finishes* each voice to a measurable professional standard while staying unique per user.

Two caveats limit how confidently that space can be claimed. First, the research could not verify 2025–2026 AI features for Arturia Pigments, Phase Plant, XO, Atlas, Output Arcade and several others. Second, nobody has checked whether Unisynth has since added reference-audio input or loudness finishing. Both must be checked before positioning. Price anchors cluster tightly: Unisynth $197, Synplant 2 $149, Combobulator $149, Emergent Drums 2 $249, Serum 2 $249 ([KVR](https://www.kvraudio.com/news-print.php?id=63286); [NI](https://www.native-instruments.com/products/combobulator)). That makes **$149–199 perpetual, or rent-to-own**, the natural slot.

## "Finished" is mostly processing, and processing leaves measurable fingerprints

The key insight for this product is that **release-ready EDM character comes overwhelmingly from the post-chain, not the oscillator**. Practitioners agree on the standard finishing chain:

- a mono, tightly controlled sub, high-passed at 20–25 Hz, with 2–3 dB of multiband compression below about 120 Hz
- OTT-style three-band upward and downward compression
- saturation and clipping ahead of the limiter
- club masters at roughly −6 to −9 LUFS with a −1 dBTP ceiling

([Violet Recording](https://violetrecording.com/how-to-master-edm/); [EDMProd on OTT](https://www.edmprod.com/ott-plugin/); [KVR](https://www.kvraudio.com/forum/viewtopic.php?p=8190501)).

Sound On Sound adds a useful constraint. Monophonic bass tolerates flat-top clipping as a timbral effect, but polyphonic material sounds "nasty" under even light clipping ([Sound On Sound](https://www.soundonsound.com/node/4918581)). So the finishing chain should be **role-aware**: a bass voice gets a different clipper policy than a pad.

Each of these techniques leaves a measurable trace, which is what makes a style-profile database possible:

| Technique | Measurable target the instrument can steer toward |
|---|---|
| Mono bass | Side/mid energy below ~120 Hz near zero; sub-band phase correlation near +1 |
| Sub control | Low-band crest factor and short-term level variance |
| OTT-style density | Lower per-band crest factor and loudness range; lifted low-level detail |
| Clipping/saturation | Peak-to-loudness ratio, harmonic content, flat-top sample counts |
| Final loudness | Integrated and short-term LUFS per genre; true peak ≤ −1 dBTP; width rising with frequency |

The research evidence argues for modelling **genre-and-role style rather than named engineers**. Wilson and Fazenda analyzed 1,501 mixes and found that mix variation collapses onto four dimensions: loudness/dynamics, brightness, bass and width. The individual engineer explained very little of listener preference (**eta² = 0.021**) ([Salford](https://salford-repository.worktribe.com/OutputFile/1493955)). The closest controlled "engineer fingerprint" study had 8 engineers each mix 8 songs. It tested whether processing depends on instrument, song or engineer, but it did not deliver a usable engineer classifier ([ISMIR 2014](https://archives.ismir.net/ismir2014/paper/000293.pdf)).

Named-engineer cloning is therefore weak science and also a likeness and passing-off risk. "Style profiles" learned from references are both more defensible and more achievable.

Loudness targets are contested, and that matters for design. iZotope puts chart dance/electronic masters around **−8.3 LUFS**, with a genre reputation for going louder than −6 ([iZotope](https://www.izotope.com/en/learn/dance-electronic-analysis)). The MixCheck dataset clusters masters near **−14 LUFS** across 30 genres ([arXiv 2412.03373](https://arxiv.org/pdf/2412.03373); [Zenodo](https://zenodo.org/records/13683186)), most likely because it holds user uploads rather than chart releases.

No peer-reviewed per-subgenre statistics exist for dubstep, techno, tech-house, trap or synthwave. **The profile database is therefore proprietary work and a moat.** It is not something that can be downloaded.

A more important design problem follows. A single synth voice that self-limits to −7 LUFS will wreck a mix. The instrument should instead target *perceived* finish (density, crest, tilt, width structure) at a stem-safe output level. This "mix-safe finishing" problem is unsolved in any product and is part of the novelty.

The research gives two proven ways to drive such a chain:

- **One-shot parameter prediction.** DeepAFx-ST trains a network to predict effect settings from an input and a style reference. It is self-supervised and needs no labelled pairs ([arXiv 2207.08759](https://arxiv.org/pdf/2207.08759)). Diff-MST predicts editable console parameters with a transformer controller ([arXiv 2407.08889](https://arxiv.org/pdf/2407.08889)) and has already been demoed inside Cubase ([arXiv 2411.06576](https://arxiv.org/html/2411.06576v1)). This route takes about 1 s.
- **Inference-time optimization.** ST-ITO searches the parameters of *any* effect chain, including non-differentiable plugins, against a self-supervised style embedding (AFx-Rep) using a gradient-free optimizer. This route takes about a minute ([arXiv 2410.21233](https://arxiv.org/pdf/2410.21233)).

Sony's ITO-Master applies the same idea to mastering, adds CLAP text steering, and warns that heavily limited commercial masters are the hardest to match ([arXiv 2506.16889](https://arxiv.org/pdf/2506.16889)). A useful refinement adds a Gaussian prior from a real preset dataset (DiffVox), so the optimizer avoids unrealistic settings ([arXiv 2505.11315](https://arxiv.org/pdf/2505.11315.pdf)).

ST-ITO's ability to optimize a non-differentiable chain matters here: **you can tune your shipped C++ DSP directly**, with no need to keep a differentiable copy in sync.

One cautionary data point deserves design weight. A classifier separated human masters from Ozone masters with **88.2% accuracy** ([ISMIR 2025 LBD](https://ismir2025program.ismir.net/lbd_465.html)). That means simple automated chains leave a recognisable "AI-finished" signature. The product must avoid converging every sound onto one fixed chain topology.

## Unproductized 2024–2026 research supplies the core engine

The synth side of the problem has moved quickly. Sound matching has shifted from plain parameter regression (InverSynth, Sound2Synth) to **generative inversion**. The landmark is Hayes, Saitis and Fazekas's permutation-equivariant flow matching, which won Best Student Paper at ISMIR 2025 ([arXiv 2506.07199](https://arxiv.org/abs/2506.07199v1)). It recognises that synth parameter spaces contain symmetries: many different presets produce the same sound, so point regression fails. A flow model instead outputs a *distribution* of plausible patches, and it beat baselines on Surge XT.

This is directly what a "generate variations" button needs. Each sample from the flow is a different, valid, editable patch for the same target.

The ISMIR 2026 follow-up tackles **off-manifold** references, meaning audio your synth could not have produced exactly ([arXiv 2609.29320](https://arxiv.org/pdf/2609.29320)). That is precisely the situation when a user drops in a professional dubstep growl. A cautionary result shows why the method matters. **Sound2Serum** regressed parameters naively on a 40k-example Serum 2 dataset, and listeners rated 82% of its outputs below a 300 Hz low-pass anchor ([AAU](https://projekter.aau.dk/sound2serum-neural-sound-matching-system-for-serum-2-6027defd.html)). Brute-force regression onto a complex commercial synth does not work.

Several other open-code methods fill the remaining pieces, and none has been found in a commercial plugin:

- **Modulation discovery** recovers the envelope and LFO curves behind a sound, tested on modulation curves from Vital presets ([arXiv 2510.06204](https://arxiv.org/abs/2510.06204)). It addresses the "motion" that makes EDM basses feel alive.
- **Differentiable wavetable synthesis** learns compact dictionaries of 10–20 wavetables that export straight into a conventional wavetable oscillator ([arXiv 2111.10003](https://arxiv.org/pdf/2111.10003)).
- **Neural proxies** map presets of black-box VSTs into a pretrained audio-embedding space, which makes them differentiable for matching ([arXiv 2509.07635](https://arxiv.org/abs/2509.07635); [GitHub](https://github.com/DBraun/synth-proxy)).
- **FM-SynAPSE** learns joint audio/preset embeddings for retrieval ([arXiv 2608.18226](https://arxiv.org/pdf/2608.18226)).
- **CTAG** optimizes an inspectable synth patch toward a CLAP text target with evolutionary search ([arXiv 2406.00294](https://arxiv.org/html/2406.00294v1)).
- A **DAFx 2026 compiler** turns PyTorch differentiable graphs into FAUST DSP and hot-reloads it into a plugin while training runs ([arXiv 2606.21277](https://arxiv.org/pdf/2606.21277)).

On the effects side, **dasp-pytorch** (Apache-2.0) provides differentiable EQ, compression, distortion, reverb and stereo processing ([GitHub](https://github.com/csteinmetz1/dasp-pytorch)). **NablAFx** provides gray-box effect chains ([arXiv 2502.11668](https://arxiv.org/pdf/2502.11668)), and **GRAFX** provides batched processing graphs ([arXiv 2408.03204](https://arxiv.org/pdf/2408.03204)).

The research also sets limits on pure neural synthesis. **BRAVE** cut RAVE's latency below 10 ms with about 3 ms jitter, but only as a proof-of-concept plugin, and its quality is sensitive to training data ([arXiv 2503.11562](https://arxiv.org/abs/2503.11562v2); [NIME 2025](https://instrumentslab.org/data/andrew/caspe_nime2025.pdf); [CCRMA](https://ccrma.stanford.edu/~nshaheed/220c_sp25)). Real-time DDSP timbre transfer has scored below offline demos ([EmergentMind](https://www.emergentmind.com/topics/ddsp-framework)). Latent diffusion produces convincing 44.1 kHz stereo one-shots offline. Stable Audio Open Small, with 341M parameters, runs in about 75 ms on an H100 and in under 8 s on Arm phone CPUs ([Stability AI](https://stability.ai/news/stability-ai-and-arm-release-stable-audio-open-small-enabling-real-world-deployment-for-on-device-audio-control)). That is fine for a "generate" button but not for note-on synthesis, and its community licence needs commercial review.

The conclusion is firm: **for clean, phase-coherent, alias-free bass and lead tones, only a real DSP engine reaches commercial quality**. Neural generation earns a place for transients, noise layers and textures rendered into buffers off the audio thread. The research also has a hole: no differentiable models of supersaw unison stacks or OTT-style compression have been validated against EDM presets. Building those is novel R&D work the product will own.

## Real-time feasibility: neural nets steer, DSP plays

The tooling picture improved materially in late 2025. Steinberg relicensed the **VST3 SDK under MIT** with VST 3.8.0 (29 Oct 2025), so no agreement with Steinberg is required. You only keep the licence text, though the VST trademark has its own guidelines ([Steinberg press release](https://ocl-steinberg-live.steinberg.net/_storage/asset/819253/storage/master/Press%20Release%20-%202025-10-29%20-%20VST%203.8%20-%20EN.pdf); [Sonicstate](https://sonicstate.com/news/2025/10/30/vst-3-now-available-under-mit-license)).

**JUCE 8** remains the pragmatic framework for a closed-source VST3/AU/AAX instrument. It has a free Starter tier up to $20k in annual revenue with no splash screen, then Indie at $40/month or $800 perpetual up to $300k ([JUCE forum](https://forum.juce.com/t/important-changes-to-the-juce-end-user-licence-agreement-for-juce-8/61265)). Native CLAP is promised for JUCE 9 ([JUCE roadmap](https://juce.com/?p=1627)), but no shipped release was confirmed. **clap-juce-extensions** bridges the gap until then ([ecosyste.ms](https://awesome.ecosyste.ms/projects/github.com%2Ffree-audio%2Fclap-juce-extensions)).

Rust's **nih-plug** is attractive, but it lacks AU/AAX, and its VST3 bindings were GPLv3 ([nih-plug](https://github.com/robbert-vdh/nih-plug)). That makes it a poor fit for a commercial macOS-heavy EDM audience.

For inference, the benchmarks agree that no single engine wins:

- **The anira paper** found ONNX Runtime fastest for stateless models and LibTorch fastest for stateful ones. It also found that general engines are not real-time safe, because they allocate memory and spawn threads. Its fix is to move inference to a static thread pool off the audio callback, with latency management and warm-up ([arXiv 2506.12665](https://arxiv.org/pdf/2506.12665); [anira](https://github.com/anira-project/anira)).
- **DAFx-26** measured under realistic DAW load and warned that "isolated benchmarks can be misleading". It found Apple's ahead-of-time compiled **BNNSGraph** the most robust option for conv models on Apple Silicon, safe to run directly on the audio thread ([DAFx-26](https://dafx.de/paper-archive/2026/papers/DAFx26_paper_16.pdf)).
- **RTNeural** stays the safe choice for tiny dense, GRU and LSTM models, though its limited set of layer types constrains architectures ([arXiv 2106.03037](https://arxiv.org/pdf/2106.03037)).

Shipped open-source neural effects such as Neural Amp Modeler, AIDA-X and Chowdhury DSP's plugins prove the pattern at production scale: tiny recurrent or conv models running on the CPU inside JUCE, iPlug2 or DPF ([AIDA-X](https://archlinux.org/packages/extra/x86_64/aida-x-vst); [NAM](https://github.com/sdatkinson/NeuralAmpModelerPlugin)).

This dictates a **three-tier runtime**:

1. **Generation and analysis on a background thread.** The patch-generating flow model, reference analysis and ST-ITO-style refinement all run here, triggered by a button or a preset change. Results are written into preallocated parameter and wavetable buffers.
2. **An optional tiny per-sample "character" network on the audio thread**, under RTNeural or BNNSGraph, at standard block sizes.
3. **Everything else as deterministic C++ DSP.**

Ship one engine per platform to keep installers small: ONNX Runtime with BNNSGraph on macOS, or RTNeural throughout. LibTorch's binaries run to hundreds of megabytes.

Licences in the offline pipeline matter too:

- **LAION-CLAP is reportedly CC0** ([GitHub](https://github.com/LAION-AI/CLAP)), while **MERT weights are CC BY-NC 4.0** ([HF](https://huggingface.co/m-a-p/MERT-v0/discussions)). Any shipped embedding should therefore be CLAP-based or self-trained.
- Essentia (AGPL), pedalboard and DawDreamer (GPLv3), and Matchering (GPL-3.0) belong only in internal, undistributed scripts ([DawDreamer](https://GitHub.com/DBraun/DawDreamer); [Matchering](https://github.com/wokashi-rg/Matchering)). Matchering's RMS/spectrum/peak/width matching idea should be reimplemented rather than linked.

## Legal design: render your own data, keep references as numbers

No US court has ruled on fair use for audio training, but the direction of travel is clear and unfavourable to unlicensed generative training:

- **Germany.** The Munich Regional Court found against Suno on 31 Jul 2026. It enjoined training, memorisation in the model and infringing outputs, applied US law to US training, and *rejected* fair use. The ruling is not final ([Bird & Bird](https://www.twobirds.com/en/insights/2026/germany/munich-district-court-rules-on-ai-generated-music-gema-v-suno); [Variety](https://variety.com/2026/digital/news/suno-loses-ai-lawsuit-gema-1236825010/)). This followed GEMA v. OpenAI, which held that memorisation falls outside the EU text-and-data-mining (TDM) exceptions ([Taylor Wessing](https://www.taylorwessing.com/de/insights-and-events/insights/2025/11/gema-v-openai)).
- **United States, books.** Kadrey v. Meta found training fair use only because the plaintiffs failed to show "market dilution". The judge warned that copying to train generative models will often be illegal ([Authors Alliance](https://www.authorsalliance.org/2025/06/26/meta-wins-on-fair-use-for-now-but-court-leaves-door-open-for-market-dilution/)). The Copyright Office's Part 3 report endorses dilution as a factor-four harm ([Sidley](https://datamatters.sidley.com/2025/05/28/generative-ai-meets-copyright-scrutiny-highlights-from-the-copyright-offices-part-iii-report/)).
- **United Kingdom.** The government dropped its proposed commercial TDM exception in March 2026, so commercial training on UK works needs licences ([Reed Smith](https://www.reedsmith.com/articles/uk-copyright-and-ai-report-the-opt-out-is-dead-but-what-comes-next/)).

**A plugin that outputs sounds substituting for sample packs and presets is the textbook dilution case.**

The contractual layer is even clearer:

- **Splice** states that using its content for AI training "is not permitted" ([Splice Help](https://intercom.help/splice-b7a2d5b83f2e/en/articles/8652642)).
- **Loopmasters** bars use to train any AI that may "learn from or replicate the products" ([Loopmasters](https://help.loopmasters.com/hc/en-us/articles/27012699080348-Loopmasters-License-Agreement)).
- **Native Instruments' EULA** forbids use "for data mining or for training artificial intelligence technologies" ([NI EULA](https://www.native-instruments.com/pages/end-user-license-agreement)).

These are contract claims, so they bite even where individual one-shots are not copyrightable.

Against that, US law is permissive about the *output* this product makes. Under **17 U.S.C. §114(b)**, sound-recording rights cover only the actual fixed sounds, so deliberate imitation is not infringement ([Justia](https://law.justia.com/codes/us/1994/title17/chap1/sec114/)). The research found no case holding a synth timbre or patch copyrightable.

That produces a risk ladder, and the architecture should stay on its lower rungs:

1. **Lowest risk: user-directed local analysis of the user's own references.** This is the user's own act, and the developer ships no data.
2. **Developer-side aggregate statistics** such as genre target curves and descriptor distributions, computed transiently from lawfully purchased audio and stored only as numbers. The musdb-XL dataset sets a precedent: it is distributed as gain parameters rather than audio for copyright reasons ([Zenodo](https://zenodo.org/records/8191153/latest)).
3. **Discriminative or parametric models** trained on copyrighted audio.
4. **Highest risk: generative audio models trained on commercial audio or packs.**

The training corpus for the generative patch model should be **renders of your own engine**. Use patches designed by in-house or commissioned sound designers under work-for-hire contracts with explicit ML-training grants, plus procedurally sampled variations. Sampling from a designed-preset distribution, as the Massive sound-matching work did, beats uniform sampling ([arXiv 2407.16643](https://arxiv.org/pdf/2407.16643)).

Avoid artist-name prompting, which raises right-of-publicity and passing-off exposure. Do not ship third-party preset parameter dumps. Pursue **Fairly Trained "Licensed Model" certification** and publish a voluntary EU-style training-data summary as trust signals ([Fairly Trained](https://fairlytrained.org/blog/fairly-trained-launches-certification-for-generative-ai-models-that-respect-creators-rights)). None of this is legal advice. Engage counsel before any developer-side analysis of commercial releases, especially for UK and German sales.

## Recommended architecture: a profile-conditioned synth with finishing in every voice

The proposed product (working name: a "finished-sound engine") has five layers.

| Layer | What it is | Built from |
|---|---|---|
| Voice engine | Proprietary C++ wavetable + FM + noise oscillators, unison stack, filters, modulation; alias-free, deterministic | Own code (no GPL DSP); a PyTorch/JAX differentiable twin kept in sync, optionally via the FAUST-compilation route |
| Per-voice finishing chain | Role-aware saturation/clipper, 3-band up/down compressor, mono-sub crossover, dynamic EQ, width, soft ceiling with stem-safe gain staging | Native DSP; parameters predicted, never hard-wired |
| Style-profile database ("Sound DNA") | Per genre × role (sub, reese, growl, lead, pluck, pad, kick, perc, FX): distributions of short-term LUFS, per-band crest, spectral tilt vs genre curve, per-band M/S width, sub correlation, PLR, harmonic content, modulation-rate statistics, plus a self-supervised production-style embedding and CLAP semantics | Offline Python (pyloudnorm, librosa, dasp-pytorch, own AFx-Rep-style encoder); stored as numbers only |
| Generator | Conditional flow-matching model: (genre, role, profile embedding, user seed, macro state) → joint distribution over engine + finishing parameters; samples are patches, and resampling yields variations | Hayes-style equivariant flow matching, trained on millions of twin renders through the finishing chain |
| Refiner | Background "Match" step: drop a reference → local analysis → off-manifold inversion → ST-ITO-style gradient-free polish of the real C++ chain against profile + embedding distance, with a realistic-preset prior | ST-ITO, off-manifold flow matching, modulation discovery, DiffVox-style prior |

The **simple UI** maps onto this cleanly:

- **Six main knobs.** Not raw parameters, but *learned directions inside the finished-sound manifold*, for example Weight, Grit, Motion, Brightness, Space and Finish. Each turn re-queries the generator or moves along embedding-space directions, so the sound cannot leave "release-ready" territory. This answers the KSHMR-style "only intensity" complaint without exposing 400 parameters.
- **The display.** A live target-versus-current descriptor view: tonal tilt, width per band, density and loudness against the chosen profile's distribution. The user *sees* why the sound reads as finished.
- **A few extra controls:** genre/role selector, Generate/Variation (new seed), Match (reference drop), and an Advanced page exposing the full patch for editing.

A **sidechain input** lets the instrument listen to the mix bus and finish to *fit*. It carves masking frequencies and chooses headroom relative to the track, which turns Output Co-Producer's context listening from retrieval into synthesis.

What makes this new rather than another synth iteration:

| Capability | Unisynth | Synplant 2 | Spire AI | Ozone / RoEx | Proposed engine |
|---|---|---|---|---|---|
| Genre-conditioned patch generation | Yes | No | Timbre categories | n/a | Yes, as a sampled *distribution*, not a pool |
| Learns from user-supplied reference sounds | Not verified | Mono, own simple engine | No | Full mixes only | Yes, off-manifold, stereo, modern engine + FX |
| Mastering-grade finishing per voice | No (ordinary FX) | No | No | Mix bus only | Yes, predicted from measured profiles |
| Mix-aware finishing via sidechain | No | No | No | No | Yes |
| Uniqueness guarantee | Not claimed | Not claimed | Complaints of repetition | n/a | Seeded sampling + per-user profile personalization |
| Local/offline | Yes | Yes | Login required | Varies | Yes |
| Macros in a learned latent space | No | No | No | No | Yes |

Five distinct innovations stand out:

1. **Sound-source-level reference matching**, which no mastering or synth product does.
2. **A finishing chain predicted per voice from measured professional statistics** rather than a stock FX rack.
3. **A distributional generator**, so "generate" never returns a pool preset.
4. **Personalization.** The user's own reference folder becomes a private profile, so two users' "techno rumble" profiles diverge.
5. **A visible, explainable "why it's finished" display.**

These five rest on research that, as far as the notes could determine, has never been productized. They also sit on the low-risk rungs of the legal ladder.

## A five-phase roadmap that ships value before the hardest ML

The durations below are planning estimates for a small team, not sourced figures.

| Phase | Months | Deliverable | Exit criterion |
|---|---|---|---|
| 0. Foundations | 0–2 | Counsel review; data policy; descriptor pipeline; first profile set from a curated, lawfully acquired reference library (stored as numbers); in-house patch-design brief with ML-rights contracts | Descriptors separate genres and roles in held-out tests; per-subgenre targets drafted, since none are published |
| 1. Engine + finishing MVP | 2–6 | JUCE 8 VST3/AU instrument: C++ voice engine, per-voice finishing chain, display, six macros mapped by hand to profile directions; offline ST-ITO-style tuning of the chain per profile | Blind A/B against commercial presets on loudness-matched playback; CPU budget met with 16+ voices |
| 2. Generator | 6–10 | Differentiable twin; GPU rendering of millions of designed-distribution patches through the chain; flow-matching generator; Variation button; learned macro directions; ONNX/BNNSGraph background inference with warm-up | Variation diversity measured in embedding space; no audio-thread allocations; listener preference for generated over random patches |
| 3. Reference Match + personalization | 10–14 | Drag-and-drop reference; off-manifold inversion + refinement (seconds to about 1 minute); modulation discovery; user-folder profile learning | "Close in character" ratings on unseen professional one-shots; refinement converges inside the realistic-preset prior |
| 4. Context + differentiation | 14–18 | Sidechain mix-aware finishing; optional tiny RTNeural "character" stage; optional off-thread transient/texture generator trained on own renders; CLAP and AAX builds; Fairly Trained application | Stems sit in a mix without re-limiting; launch at $149–199 |

Phase 1 is deliberately shippable without any runtime neural network. Profiles plus an offline-tuned finishing chain already deliver "finished from the start". That de-risks the business while the generator and matcher, the genuinely novel parts, mature behind it.

## Conclusion

The novel product is not a smarter preset generator. It is an instrument whose *definition of quality is a measured, inspectable profile*, and whose AI chooses parameters toward that profile rather than inventing audio. That reframing resolves three problems at once:

- **Quality.** DSP renders every note, so output matches commercial synth fidelity.
- **Legal exposure.** Training data is self-rendered, and references persist only as statistics.
- **Sameness.** Patches are sampled from a distribution and personalized by the user's own references.

Two research questions remain open and are worth owning as IP. The first is how to finish a single voice so it sounds mastered while staying mix-safe. The second is how to keep automated finishing from acquiring the detectable "AI-mastered" signature that a classifier already catches 88% of the time. The competitive window is real but time-limited. Unisynth has shown there is demand for genre-first generation, and the remaining gaps (reference matching, per-voice finishing, mix awareness) are exactly the features an incumbent could bolt on next. So verify Unisynth's current feature set before positioning, and move Phase 3's Reference Match as early as the engine allows.
