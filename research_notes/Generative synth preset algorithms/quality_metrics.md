# Measuring whether a synth sound is "good", "big", "interesting" or "finished"

Scope: perceptual and learned metrics usable as a fitness or judge for Generate / Reference Match in Zero Engine (CPU, short renders, milliseconds). Searched Oct 2026. Most sources were seen as abstracts, model cards or summaries, not full papers; figures marked "secondhand" are not checked against the original.

## Which audio descriptors predict perceived bigness / fullness / width / punch / aggression?

### Takeaway
Three semantic dimensions recur across timbre studies: luminance (brightness/sharpness), texture (roughness/harshness) and mass (full/thick/dense). Brightness and roughness have solid acoustic correlates; "mass/fullness" is the weakest-linked to simple descriptors. Punch and heaviness/"hardness" have validated, computable models (Fenton's PM95; Czedik-Eysenberg's regression, r = 0.86), and width has a frequency-dependent decorrelation model. Nothing found is specific to EDM synth patches.

### Cited Findings
**Timbre semantics (classic framework)**
- Greek and English listeners rated 23 instrument tones on 30 adjectives; three shared semantic dimensions emerged: luminance, texture and mass ("dense, rich, full, thick" vs "light") — [Zacharakis, Pastiadis, Reiss 2014, Music Perception](https://joshreiss.github.io/documents/2014/Zacharakis%20Pastiadis%20Reiss%20-%20Music%20Perception.pdf)
- Acoustic correlates named in that work: energy distribution of harmonic partials, spectral detail, temporal/spectrotemporal characteristics (full per-dimension list not confirmed, abstract truncated) — [Zacharakis et al. 2014](https://sonicfield.org/library/an-interlanguage-study-of-musical-timbre-semantic-dimensions-and-their-acoustic-)
- Comparing semantic and perceptual (MDS) spaces: texture and luminance matched perceptual dimensions well; mass showed no strong correlations — [Zacharakis, Pastiadis, Reiss 2015, Music Perception](https://joshreiss.github.io/documents/2015/Zacharakis%20Pastiadis%20Reiss%20-%20Music%20Perception%202015.pdf)
- Inharmonicity relates to the luminance and mass dimensions — [Zacharakis & Pastiadis 2021 (core.ac.uk)](https://core.ac.uk/download/440379996.pdf)
- Background noise can rearrange timbre spaces (judgements are context-dependent) — [Zacharakis et al. 2017, Acta Acustica (core.ac.uk)](https://core.ac.uk/download/80685327.pdf)
- Synthesizer-specific (FM): with adjectives taken from synth forums, luminance and texture merged into a single "sharpness" factor, and mass was a separate factor. Participants tuning for brightness and roughness moved the modulator parameters similarly, while thickness was pursued through amplitude envelopes — [Hayes et al. 2021/2022, QMUL](https://comma.eecs.qmul.ac.uk/assets/pdf/hayes_perceptual_2021.pdf); [Hayes et al. "disembodied" 2022](https://comma.eecs.qmul.ac.uk/assets/pdf/hayes_disembodied_2022.pdf)
- Wide, flat spectral envelope peaks go with "full/rich/rough", narrow peaks with "thin/smooth" (French horn study) — [MIT thesis, dspace 1721.1/54498](https://dspace.mit.edu/handle/1721.1/54498?show=full)
- Greater bandwidth (spectral spread) maps to "full"/"rich" in a compiled, non-peer-reviewed summary table — [Vassilakis 2009](https://acousticslab.org/RECA293/293Files/293Downloads/Vassilakis2009_SpectrumTimbre.pdf)
- A "fullness" timbre dimension appeared only for musician listeners — [Jyväskylä dissertation](https://jyx.jyu.fi/bitstream/123456789/38121/1/9789513946548.pdf)
- Caution: most timbre research is correlational; little evidence features causally drive the perceived dimensions — [Frontiers in Psychology 2022](https://www.frontiersin.org/articles/10.3389/fpsyg.2022.835401)

**Psychoacoustic metrics (Zwicker/Fastl)**
- Zwicker–Fastl psychoacoustic annoyance: PA = N5 (1 + sqrt(wS^2 + wFR^2)), N5 = loudness exceeded 5% of the time, S sharpness (term active only above 1.75 acum), F fluctuation strength, R roughness, with F and R weighted 0.4/0.6 — [metricgate docs](https://metricgate.com/docs/psychoacoustic-annoyance/); [arXiv 2410.22208](https://arxiv.org/pdf/2410.22208); book: [Psychoacoustics: Facts and Models (TUM catalogue)](https://search.ub.tum.de/vufind/Record/DE-604.BV022007777?lng=en)
- The PA model is referenced in ISO 12913 soundscape work — [Internoise 2019](https://acusvirt.upv.es/wp-content/uploads/2021/02/Internoise2019-Zwicker.pdf)

**Heaviness / hardness / aggression**
- 40 listeners rated 62 excerpts for "hardness"; a multiple linear regression on extracted features reached r = 0.86 — [Czedik-Eysenberg, Knauf, Reuter 2017 (GI)](https://dl.gi.de/items/5581898c-a10d-4603-9d4e-9138eaecc459/full)
- Strong single correlates reported: sensory dissonance r = 0.814, spectral skewness r = -0.650, plus event density and onset tempo; a poster lists spectral flux r = 0.80 and roughness r = 0.75. Percussive elements, flat envelope, high loudness and strong treble raise heaviness regardless of genre — [DGM 2017 poster](https://homepage.univie.ac.at/christoph.reuter/unterwegs/DGM2017_Czedik_Knauf_Reuter.pdf); [DAGA 2017](https://homepage.univie.ac.at/christoph.reuter/unterwegs/DAGA2017_Czedik_Knauf_Reuter.pdf)
- Berger & Fales (2005), as cited by Herbst: treble dominance, high loudness and harmonic dissonance constitute heaviness — [Herbst 2018, accepted version](https://eprints.hud.ac.uk/33381/1/Herbst_Heaviness_accepted%20version.pdf)
- Distortion level and harmonic complexity both reduce sensory consonance, more so together; acoustically distortion has the slightly stronger effect, perceptually the ratio depends on the listener — [Herbst 2018](https://pure.hud.ac.uk/en/publications/heaviness-and-the-electric-guitar-considering-the-interaction-bet/)
- In 270 distorted guitar chords with 171 listeners, roughness was the least reliable variable for explaining reduced sonority — [Herbst 2017](https://eprints.hud.ac.uk/id/eprint/33379/)
- With distortion, power and major chords were rated significantly more pleasant than minor and altered-dominant chords — [Herbst 2019, Music Perception](https://pure.hud.ac.uk/en/publications/distortion-and-rock-guitar-harmony-the-influence-of-distortion-le/)

**Punch**
- Fenton/Lee/Wakefield: perceptual punch model = signal separation + low-level measures into a weighted score; punch correlated with onset times, octave band, signal duration and dynamic range — [AES 139 paper](https://eprints.hud.ac.uk/id/eprint/26288/9/AES%20139_Perceptual_Punch_Model_Fenton.pdf); [Fenton thesis](https://eprints.hud.ac.uk/id/eprint/32629/)
- PM95 (95th-percentile weighted transient loudness) outperformed other models: Pearson r 0.849, Spearman 0.833 — [JAES 2019, weighted transient loudness](https://pure.hud.ac.uk/en/publications/a-perceptual-model-of-punch-based-on-weighted-transient-loudness/)
- Real-time meter (P95, mean punch): Pearson r 0.840, Spearman 0.937 with listeners; a VST/AU Perceptual Punch Tool was released on Zenodo (2021) — [AES 2018 real-time system](https://pure.hud.ac.uk/en/publications/real-time-system-for-the-measurement-of-perceived-punch/); [eprints 20141](https://eprints.hud.ac.uk/20141)

**Width**
- The IACC-to-perceived-width relation for narrow-band sounds depends on frequency; Mason, Brookes & Rumsey (JASA 2005) model it (rectification + low-pass above phase-locking, extra low-frequency dependence) and predict the data well — [JASA 10.1121/1.1853113](https://zendy.io/pdf-viewer/10.1121/1.1853113); [Surrey record](https://openresearch.surrey.ac.uk/permalink/44SUR_INST/15d8lgh/alma99512180402346)
- Lower-frequency sounds seem wider than high-frequency sounds at the same interaural coherence, and width grows with SPL — [DTU / Käsbach et al. (core.ac.uk)](https://core.ac.uk/download/20608606.pdf)
- Headphones: decreasing IACC broadens width most around 400–800 Hz; loudness widens most near 200 Hz, least near 1.6 kHz — [UTS, Applied Acoustics 2020](https://opus.lib.uts.edu.au/handle/10453/137853)
- ITLD (interaural thresholded level distribution) as an alternative width metric; decorrelation effect strongest 125 Hz–2 kHz anechoic, 500 Hz–2 kHz in rooms — [DAFx 2024 paper 38](https://dafx.de/paper-archive/2024/papers/DAFx24_paper_38.pdf)
- Frequency-dependent decorrelation can split a source spatially by band — [Potard & Burnett, DAFx 2004](https://dafx.de/paper-archive/2004/P_280.PDF)

### Inferences
- Cheap proxies the literature supports: brightness = spectral centroid / % energy above ~4 kHz (Zero's analyzer already has these); texture/aggression = roughness or sensory dissonance, spectral flux, (negative) spectral skewness, loudness; mass/fullness = spectral spread/bandwidth plus sustained envelope (Hayes: thickness via amplitude envelope), but this is the least reliable dimension, so the owner's references matter most here.
- Width should be measured per band (side share or 1 − correlation in ~200 Hz–2 kHz), not broadband: decorrelation there buys perceived width; below ~150 Hz it costs mono-compatibility and above a few kHz it buys little.
- Punch for plucks/stabs/basses: a PM95-style "95th percentile of onset-weighted transient loudness" is computable from an STFT in milliseconds; Zero's existing "Punch" macro could be judged by it.
- The annoyance formula (loudness × (1 + sharpness/roughness/fluctuation)) is useful the other way round: as a penalty term that grows when a candidate is loud, sharp and rough at once.
- Background knowledge, not re-sourced this session: McAdams-style timbre spaces (attack time, centroid, spectral flux) and BS.1770 loudness underlie these descriptors.

### Gaps
- No study found on perceived bigness/width/punch of EDM synth presets specifically (supersaws, reese basses, wobbles).
- No direct study found tying unison detune amount to perceived "thickness"; only adjacent blend work ([McGill](https://escholarship.mcgill.ca/downloads/47429g585)).
- Fluctuation strength vs "movement"/"interesting" for synth sounds: no study found.

## Learned aesthetic / quality predictors (Audiobox Aesthetics, CLAP scoring, FAD/KAD, PAM): licences, CPU cost, usefulness on single synth notes

### Takeaway
Audiobox Aesthetics (Meta, 2025) is the most relevant: four axes (Production Quality, Production Complexity, Content Enjoyment, Content Usefulness), CC-BY 4.0, but a ~WavLM-base transformer in PyTorch, i.e. tens to hundreds of ms per clip on CPU and not real-time-thread material. PAM (CLAP + two opposing text prompts) is a zero-shot quality score. FAD/KAD are distribution distances (a set of renders vs a reference set), not per-sound scores, and their usefulness depends heavily on the embedding. None is validated on single synth notes.

### Cited Findings
- Audiobox Aesthetics axes: PQ (clarity, fidelity, dynamics, frequency balance, spatialisation), PC (number of audio components), CE (emotional impact, artistic expression, pleasure), CU (usefulness as source material for creation) — [arXiv 2502.05139](https://arxiv.org/pdf/2502.05139); [Meta publication page](https://ai.meta.com/research/publications/meta-audiobox-aesthetics-unified-automatic-quality-assessment-for-speech-music-and-sound/)
- Licence: mainly CC-BY 4.0; WavLM-derived code under MIT (Microsoft unilm). Code at facebookresearch/audiobox-aesthetics, weights at HF facebook/audiobox-aesthetics; Python 3.9+, PyTorch 2.2+ — [HF model card](https://huggingface.co/facebook/audiobox-aesthetics); [PyPI](https://pypi.org/project/audiobox-aesthetics/)
- Architecture: WavLM-based encoder, 12 transformer layers, hidden 768, learnable weighted sum over layers/time, MLP heads per axis, MAE+MSE losses (summary) — [Emergent Mind summary](https://www.emergentmind.com/papers/2502.05139)
- Axis correlations (secondhand): PQ–CU r = 0.81; PC weakly correlated (0.26 with PQ, 0.35 with CE); critic argues pooling speech/sound/music hides music-specific behaviour — [Substack critique](https://wewillnotbeflattened.substack.com/p/can-ai-measure-beauty-a-deep-dive); a practical test on real music by [Cyanite](https://cyanite.ai/tag/music-industry/page/2) (results not retrieved)
- Follow-up work improves aesthetic assessment with triplet loss and self-supervised embeddings — [arXiv 2509.03292](https://arxiv.org/html/2509.03292v1)
- PAM: no-reference; embeds audio with CLAP and scores softmax between "the sound is clear and clean" and "the sound is noisy and with artifacts"; a single prompt does not correlate with MOS but two opposing prompts do; tested on text-to-audio, text-to-music, TTS and noise suppression; authors say the best prompting strategy is unresolved — [arXiv 2402.00282](https://arxiv.org/pdf/2402.00282); [Microsoft Research](https://www.microsoft.com/en-us/research/?p=1136799)
- FAD depends on the embedding: on DCASE 2023 environmental audio, PANNs-WGM-LogMel gave Spearman > 0.5 with ratings, VGGish (original FAD) < 0.1, music-specific embeddings worse on that domain — [Tailleur et al. 2024, arXiv 2403.17508](https://arxiv.org/pdf/2403.17508)
- CLAP / LAION-CLAP embeddings reported best for music FAD vs MOS (attributed to Gui et al. 2023, secondhand, not verified) — [Emergent Mind FAD topic](https://www.emergentmind.com/topics/frechet-audio-distance-fad)
- KAD (MMD-based): distribution-free, unbiased, converges with fewer samples, cheaper; toolkit kadtk; authors' own claims of better human alignment — [arXiv 2502.15602](https://arxiv.org/html/2502.15602v2); [ICML 2025](https://icml.cc/virtual/2025/52889)
- LAION-CLAP has a music-specific checkpoint (laion/larger_clap_music) runnable on CPU via Transformers; a user reported CPU memory exhaustion after ~150–300 iterations of clap-htsat-unfused on Windows; the HTSAT audio encoder runs on CPU in a CoreML port — [HF larger_clap_music](https://huggingface.co/another-blue/larger_clap_music); [HF forum](https://discuss.huggingface.co/t/clap-model-crash-after-several-inference-iterations/60630); [CoreML port](https://huggingface.co/atan2f/larger_clap_general_coreml)

### Inferences
- For Zero Engine (C++, no Python, local): Audiobox Aesthetics would need an ONNX export of a ~95M-param WavLM-base-size model (estimate from 12 layers × 768) and runtime of order 0.1–1 s per clip on CPU (estimate, not measured). Usable offline in ZeroAnalyze to label or validate the hand-made score, not inside Generate's 6–24-candidate loop at interactive speed.
- PQ and CU are the axes most aligned with "release-ready"; PC rewards many components, which for a single synth note mostly means busy/noisy, so it should not be maximised. CE on a 2 s single note is probably weakly informative.
- FAD/KAD answer "does this batch of Generate outputs sit in the same distribution as the owner's 1,187 sounds?", which suits periodic offline evaluation of Generate (e.g. per genre/type), not per-candidate scoring. KAD is better for small sets (tens of renders per type).
- PAM-style prompt pairs could be adapted ("a big, wide, polished synth sound" vs "a thin, harsh, noisy sound") but prompt sensitivity is an acknowledged open problem; treat as an offline experiment.

### Gaps
- No measured CPU latency found for Audiobox Aesthetics or LAION-CLAP; no confirmation of the predictor's window length.
- LAION-CLAP licence not confirmed in these searches (check the GitHub repo).
- No study found validating any of these models on isolated synth notes or one-shots.

## How to combine metrics into one score without rewarding extremes

### Takeaway
Use standardised composites of several judges (z-scores vs a reference distribution) with caps and "in-character" gates, plus a diversity term or Pareto/MAP-Elites archive; single-metric optimisation is consistently exploited.

### Cited Findings
- SCORE: optimising one reward model inflates that metric far more than others (CLAP-only: +10.8% CLAP, +1.4% PQ; reverse for PQ-only), called "verifier hacking"; standardised composite rewards across several reward models reduce it — [arXiv 2509.19831](https://arxiv.org/pdf/2509.19831)
- DRAGON optimises distributional rewards built from reference exemplar sets (incl. a music aesthetics model): 81.45% average win rate over 20 reward functions and 60.95% human-voted music-quality win rate without human preference labels — [arXiv 2504.15217](https://arxiv.org/pdf/2504.15217v2)
- Quality-diversity for sound: MAP-Elites with CPPN/DSP graphs plus a classifier yields a wide range of sounds — [arXiv 2606.09780](https://arxiv.org/abs/2606.09780); [Sound Innovation Engine v1](https://uio.no/ritmo/english/people/phd-fellows/bthj/publications/sie1/sound_innovation_engine_v1.pdf)
- Learned behaviour spaces (PCA/autoencoder on audio features) give more diversity than hand-made descriptor grids; retraining projections during search avoids stagnation — [arXiv 2512.02783](https://arxiv.org/pdf/2512.02783)
- A synth-preset evolution system uses a CLAP-embedding MAP-Elites grid for diversity plus a per-target champion archive; dual archive improved Kernel Audio Distance alignment to a held-out set (authors/venue unverified) — [Princeton talk listing](https://lists.cs.princeton.edu/hyperkitty/list/talks@lists.cs.princeton.edu/thread/WUWLI7Z4YGUH6CXODQEGJ33W2CQFBAHT/)

### Inferences
- Pattern that fits Zero's current Generate (SoundTraits scored against the owner's sounds, softmax, capped): (1) hard gates first: true-peak/clipping, LUFS window, sub-150 Hz mono, tail decays, harmonic-to-noise floor (ZeroAnalyze `noise`); (2) each trait as a z-score vs the owner's per-type distribution, rewarded up to e.g. +1 SD and then flat or falling (a tent/target band, not monotone "more"); (3) penalty from an annoyance-like term (loudness × sharpness/roughness together); (4) novelty vs history as a separate objective, combined by softmax or by picking from a Pareto front.
- Distribution-matching (DRAGON-style: be like the reference set) is safer than "maximise bigness", because the owner's library already encodes where "enough" stops.

### Gaps
- No study found on combination weights for synth-sound fitness specifically; weights must be fitted to owner A/B choices (e.g. the existing `genab` blind packs).

## Known pitfalls (Goodhart effects)

### Takeaway
Documented failures: diversity collapse under aesthetics-reward optimisation, single-verifier hacking, and loudness/compression being a weak or contradictory preference signal. Brightness, roughness and loudness all correlate with "heavy/aggressive", so pushing them gives harsh sounds that score as "big".

### Cited Findings
- SMART: GRPO fine-tuning of a piano model on Audiobox Aesthetics ratings improved ratings in a 14-person listening study, but over-optimisation sharply reduced output diversity — [arXiv 2504.16839 (listing)](https://awesomepapers.io/speech-audio/papers/2504.16839)
- RL post-training for melody-to-chord collapsed to trivial low-diversity outputs; an adversarial discriminator added to the reward prevented it — [arXiv 2511.17879](https://arxiv.org/abs/2511.17879v2)
- Single-reward optimisation exploits that reward ("verifier hacking") — [SCORE, arXiv 2509.19831](https://arxiv.org/pdf/2509.19831)
- Listeners showed no preference for less-compressed remasters, with low response consistency — [Hjortkjær & Walther-Hansen, JAES 2014](https://vbn.aau.dk/da/publications/perceptual-effects-of-dynamic-range-compression-in-popular-music-); contradicted by a 20-listener test preferring large dynamic range — [Ferguson, TCD 2011](https://publications.scss.tcd.ie/theses/diss/2011/TCD-SCSS-DISSERTATION-2011-021-ABSTRACT.pdf); configuration of compression studied with 130 listeners — [Campbell et al., JAES 2017](https://repository.uwl.ac.uk/id/eprint/3324/); [Limerick paper](https://pure.ul.ie/en/publications/factors-influencing-listener-preference-for-dynamic-range-compres/)
- Hardness rises with loudness, treble and dissonance — [Czedik-Eysenberg et al. 2017](https://dl.gi.de/items/5581898c-a10d-4603-9d4e-9138eaecc459/full)

### Inferences
- Always compare loudness-matched (Zero already trims to LUFS); otherwise "louder = better" bias dominates any metric.
- Width rewards invite phase tricks that collapse in mono; score width with a mono-compatibility floor (low-band correlation > ~0.8).
- "Interesting/movement" scored by spectral flux or modulation energy is gamed by noise and random jitter; require movement that is periodic or tempo-related, or measured on harmonic bins only.
- Keep a human loop: periodic blind A/B (genab) to re-fit weights and detect drift.

### Gaps
- No published study on Goodhart effects in synth preset generation specifically; the evidence is from music/audio generative models.
