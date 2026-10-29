# Newest methods for automatic synth sound generation (2024-2026)

Written 2026-10-29. Problem being solved: Generate's scoring rewarded movement and width, so it picked detuned, pitch-wobbling, smeared candidates ("a jumbled mess, just noise"). Basics already covered elsewhere in this folder (render-and-check, capped traits, novelty, softmax, hill-climb, Bradley-Terry + Thompson, MAP-Elites, PhenoType-style judge, VAEs/flows) are not repeated.

"Inference" marks my own reasoning rather than a source's claim.

---

## 1. Fast psychoacoustic "clean vs mush" metrics

All of these run on one or a few FFT frames of a rendered note. With N = 4096 at 48 kHz (11.7 Hz bins), an FFT plus peak picking costs roughly 20-50 µs per frame on one core, so about 1-3 ms per candidate for 20-50 frames (inference, from typical FFT timings). Peak picking: local maxima more than 50-60 dB below the frame maximum are dropped, keep the top 20-40, and refine frequency and amplitude by parabolic interpolation on log magnitude.

### 1a. Sensory dissonance / roughness (Plomp-Levelt, Sethares)
For partials (f_i, a_i), sum over all pairs:

    d(f1,f2,a1,a2) = a1·a2 · [exp(-b1·s·Δf) - exp(-b2·s·Δf)]
    s = x* / (s1·min(f1,f2) + s2),   Δf = |f2 - f1|
    b1 = 3.51, b2 = 5.75, x* = 0.24, s1 = 0.0207, s2 = 18.96

These are the constants used in MIRtoolbox `mirroughness` (Sethares model) ([MIRtoolbox source](https://code.soundsoftware.ac.uk/hg/camir-ismir2012/annotate/tip/toolboxes/MIRtoolbox1.3.2/MIRToolbox/mirroughness.m); [Pd dissonance toolbox](https://www.uni-weimar.de/kunst-und-gestaltung/wiki/images/Dissonance_Model_Toolbox_in_Pure_Data.pdf)). Cost: O(P²) for P peaks, so 40 peaks give 780 pairs, which is microseconds.

**Vassilakis (2001)** variant (the formula as usually quoted, e.g. in MIRtoolbox's 'Vassilakis' option; check it against Vassilakis 2001 Eq. 6.23 before relying on the constants):

    r = (a_min·a_max)^0.1 · 0.5 · (2·a_min/(a_min+a_max))^3.11 · [exp(-3.5·s·Δf) - exp(-5.75·s·Δf)]

This weights equal-amplitude pairs (deep beating) much more than unequal ones. Implementations exist in the R `dycon` package (`roughness_seth`, `roughness_vass`) ([dycon](https://rdrr.io/github/pmcharrison/dycon/f/README.Rmd)) and the Java tagtraum `RoughnessProcessor` ([docs](https://www.beatunes.com/apidocs/com/tagtraum/audiokern/audioprocessor/RoughnessProcessor.html)).

**Relevance to the detune problem (inference).** Roughness peaks at Δf of about 25% of a critical band (20-50 Hz in the mids). Fine unison detune of 5-15 cents gives beats under about 10 Hz on low harmonics, which reads as chorus and not as rough. On high harmonics (2-5 kHz) the same cents give 30-80 Hz splits, which are rough. Sethares therefore penalises wide detune on bright sounds automatically and leaves gentle supersaws alone, which is the right behaviour. Normalise by (Σa²) so the score does not depend on level. An FFT this size cannot split partials closer than about 12 Hz, so roughness alone misses slow smear; 1c covers that.

### 1b. Harmonicity / inharmonicity
- **Inharmonicity index** (Peeters 2004, CUIDADO feature set; formula from memory): given f0 and harmonic peaks f_k,
  `INH = (2/f0) · Σ_k |f_k - k·f0| · a_k² / Σ_k a_k²`. Clean tones score about 0; FM/ring-mod clang scores high. Note that it is defined against a single f0.
- **Harmonic template / harmonicity (Harrison & Pearce 2020).** Consonance is modelled as roughness + harmonicity (+ familiarity). Harmonicity is the match between the spectrum and idealised harmonic templates, found by sliding a template over a pitch-class spectrum and taking how peaked the best match is ([Harrison & Pearce 2020 summary](https://synapsesocial.com/papers/W2988017779); [ISMIR 2018 version](https://export.arxiv.org/pdf/1807.00790); R package `incon`). Eerola & Lahdelma (2021) got better fits with Stolzenburg's periodicity model in its place, 73% against 62% ([Durham repo](https://durham-repository.worktribe.com/OutputFile/1273671)). For single synth notes the simpler per-note harmonic energy ratio below is enough (inference).
- **Harmonic energy concentration** (this is what `SoundTraits::clarityOf` already does). Per frame, `C = Σ_k E(k·f0 ± w) / E_total`, with w a fixed window in cents (e.g. ±30 cents) and not in bins. A cents window makes upper harmonics tolerate the same relative detune as lower ones (inference). Two refinements: (i) take the median over frames rather than the mean, so one clean attack frame cannot save a smeared sustain; (ii) also score **peak sharpness**, the share of each harmonic's ±50-cent energy that sits inside ±10 cents. Detune smear spreads energy within the harmonic window, so C stays high while sharpness falls, and that is exactly the failure being reported.

### 1c. Pitch salience and pitch stability (tuning clarity)
- **Subharmonic summation (Hermes 1988):** `S(f) = Σ_{n=1..N} h^(n-1) · P(n·f)` on a log-frequency, auditorily weighted spectrum, with h ≈ 0.84 and N ≈ 15. Salience is `max S / mean S` (inference: a cheap normalisation). It is computed per frame on the same FFT.
- **Terhardt virtual pitch (1982) / Parncutt (1989)** do the same with spectral- and virtual-pitch weights and masking. They are better for chords and more work; for single notes SHS is the practical one (inference).
- **Pitch-stability test.** Track f0 per frame with SHS or YIN near the known MIDI pitch. Report (i) the median offset in cents from the note (out of tune), (ii) the IQR in cents over the sustain (wobble), (iii) the share of frames whose salience falls below a floor (no clear pitch). Suggested thresholds (inference, calibrate as in §3): offset ≤ 15 cents, IQR ≤ 10 cents for leads, pads and basses (a deliberate wobble bass modulates the filter, not pitch), and salience frames ≥ 80%. Vibrato a profile asks for gets an allowance from the profile's LFO depth.

### 1d. Noisiness
- **Spectral flatness:** `SFM = exp(mean ln P_k) / mean P_k` over bins (Wiener entropy). It is 0 for pure tones and 1 for white noise.
- **Johnston (1988) tonality coefficient:** `α = min(SFM_dB / -60, 1)`, with SFM_dB = 10·log10(SFM). α ≈ 1 is tonal and α ≈ 0 is noise. Computed per band (for example 150 Hz to 5 kHz), it ignores intended air or hiss above 8 kHz.
- **Spectral entropy:** `H = -Σ p_k log p_k / log K`, with p_k = P_k / ΣP. It is close to SFM and either one is enough.
- **Harmonic-to-noise ratio** = 10·log10(harmonic-window energy / remaining energy), which is clarity in dB. The repo's `noise` command already measures roughly this.
- Rough thresholds (inference): for pitched types, α over 150 Hz-5 kHz below about 0.5, or HNR below about 6 dB, means "mostly noise". FX and noise-based types skip this test.

### Use them as gates, not as rewards
SCORE (2025) shows that maximising any single verifier gets hacked ("verifier hacking"), and that combining several standardised rewards reduces it ([SCORE arXiv 2509.19831](https://www.arxiv.org/abs/2509.19831)). TuneJury (2026) found that fine-tuning on a scalar preference metric raised the reward but hurt fidelity ([arXiv 2606.17006](https://arxiv.org/pdf/2606.17006)). Our own failure is the same pattern: movement and width were rewarded, and detune is the cheapest way to get both. Recommended shape (inference):

    pass   = clarity ≥ c_min(type) AND |pitch offset| ≤ 15 c AND pitch IQR ≤ τ(type) AND roughness ≤ r_max(type) AND tonality ≥ α_min(type)
    score  = Σ capped traits (movement, width, ...)   among passers only
    movement_credit = movement measured on a pitch-normalised, harmonic-only spectrum

The last line is the key one: credit movement from **filter, envelope and wavetable change** (change in the harmonic amplitudes a_k(t)), not from frequency smear. Track a_k per frame from the harmonic peaks and score `mean |Δ log a_k|`. Detune and wobble then earn nothing (inference). Width can be handled the same way: measure side energy only inside the harmonic windows, or require mono-sum correlation ≥ about 0.3 and clarity of the M channel ≥ the gate.

---

## 2. Newest generation and search ideas

### 2a. Test-time compute scaling with verifiers (best-of-N, evolutionary)
- **SCORE (KAIST + ByteDance, Sep 2025):** training-free inference scaling for text-to-audio. It uses best-of-N and evolutionary search guided by several rewards (CLAP alignment, Audiobox PQ, an audio-LM alignment score). Each reward is standardised (z-scored over the candidate pool) and the scores are summed with weights. It reports about +10.8% alignment and about +10.7% quality over naive sampling ([arXiv](https://www.arxiv.org/abs/2509.19831); [project](https://mm.kaist.ac.kr/projects/score/)). **Directly transferable:** z-score each trait over the click's 24-100 candidates before the weighted sum, so no single trait (movement) dominates through its raw scale.
- **Inference-time scaling for joint audio-video (Jun 2026):** several verifiers combined with adaptive reward weighting ([HF 2606.03183](https://huggingface.co/papers/2606.03183)). Adaptive weighting is not needed here.
- **Verifier available off the shelf:** Meta **Audiobox Aesthetics** (Feb 2025) is a no-reference predictor of Production Quality, Production Complexity, Content Enjoyment and Content Usefulness. It is a 12-layer WavLM transformer trained on about 97k human ratings, and it is open source ([arXiv 2502.05139](https://www.emergentmind.com/papers/2502.05139); [GitHub/HF](https://huggingface.co/facebook/audiobox-aesthetics)). It is too heavy for 100 candidates per click inside the plugin (inference: about 95M parameters). Use it **offline** as a teacher: score thousands of engine renders, then distil PQ into the cheap surrogate in 2d. Check the licence before use (the release states its own terms, so read them).

### 2b. Reward-guided / RL fine-tuning of generators
- **DDSynth-RL (ISMIR 2026, arXiv 2608.03032):** synthesizer inversion framed as masked discrete diffusion over tokenised parameters and MIDI. It is fine-tuned with GRPO on rewards computed from rendered audio (a multi-metric audio reward, then CLAP + CREPE). It works with black-box synths (Dexed), and RL improves out-of-domain matching. Apache-2.0 code, CC-BY checkpoints ([arXiv PDF](https://arxiv.org/pdf/2608.03032); [HF](https://huggingface.co/MINNE-WU/DDSynth-RL)). Notable: **CREPE pitch reward**. They needed an explicit pitch-correctness reward to stop matches drifting off pitch, which is the same lesson as our out-of-tune problem.
- **SynthRL (IJCAI 2025):** RL sound matching with an audio-similarity reward, no parameter labels, reward-prioritised replay, fine-tunes on out-of-domain sounds ([IJCAI](https://www.ijcai.org/proceedings/2025/1129)).
- **NIME 2026 (Madaghiele et al.):** continuous-action RL agent controlling any synth to match a live player. It compares five reward functions ([NIME](https://nime.org/proc/nime2026_89/)).
- **MR-FlowDPO (Dec 2025):** multi-reward DPO for text-to-music, with alignment, production quality and semantic consistency rewards ([arXiv 2512.10264](https://arxiv.org/abs/2512.10264v2)). **GenSR-Pref (2025):** a preference pair counts only when a suite of metrics agrees *unanimously* (a Pareto-dominance rule against reward hacking) ([arXiv 2508.17229](https://arxiv.org/html/2508.17229v1)).
- Fit for us (inference): these all train a neural generator, which is weeks of work and gives a model we would have to ship or distil. Generate already samples from profiles, so the cheaper path is to use their *reward designs* (pitch reward, multi-metric, unanimity) in our selector. Training only pays off if Generate later needs to produce from scratch rather than vary a profile.

### 2c. LLM / model-guided evolution
- **LLM4FM (KAIST, ICML 2026):** LLMs generate DX7 patches from text or audio, with the DX7Caps dataset ([ICML](https://icml.cc/virtual/2026/77367)). An ADC Japan 2026 talk gave an LLM signal-flow knowledge and live spectral analysis so it could diagnose and fix patches ([sched](https://adcjapan26.sched.com/event/2KCdo/)).
- I found no published FunSearch/AlphaEvolve-style work on synth sound design. Such systems evolve *programs*. Here the analogue would be evolving the `generateProfile` mutation rules or the scoring function offline, with renders + metrics as the evaluator (inference). It breaks the "no cloud" rule only if run at build time, not in the plugin. Low priority: high effort and uncertain payoff.

### 2d. Surrogate-assisted evolution (screen before rendering)
Standard SAEA practice (general EC literature, not audio-specific): train a cheap regressor on (parameters → measured traits / pass-fail) from past renders, then generate many more candidates than you render (for example 2000), predict, and render only the top-K plus a few random ones (to keep the surrogate honest). **Neural Proxies for Sound Synthesizers** (JAES 73(9), 2025) trains networks that map presets to an audio-embedding space, so black-box synths can be used in neural preset systems. That is a learned surrogate of the renderer ([arXiv 2509.07635](https://arxiv.org/abs/2509.07635v1); [code](https://github.com/DBraun/synth-proxy)). For us (inference): a gradient-boosted tree or small MLP on the ~150 normalised parameters, predicting pass/fail for the gates in §1 plus the capped traits. Training data comes free from `gencheck`, with 72 profiles × hundreds of candidates rendered offline. At inference it is a few µs per candidate and needs no allocation. Gain: 10-50x more candidates screened per click at the same render budget, which matters most for profiles where few of the 6 candidates pass.

### 2e. Quality-diversity (newer than plain MAP-Elites)
- **Learned behaviour spaces for sound QD (Dec 2025):** PCA or autoencoder projections of audio features as the MAP-Elites grid, retrained during the run, gave significantly more diversity than hand-chosen descriptors ([arXiv 2512.02783](https://arxiv.org/abs/2512.02783v1)).
- **QD innovation engines for sound (Jun 2026):** QD with a deep classifier as the quality/novelty signal, using specialised CPPNs per frequency range ([arXiv 2606.09780](https://arxiv.org/pdf/2606.09780)).
- **CMA-MAE** (Fontaine & Nikolaidis) fixes CMA-ME's early abandonment of the objective and flat-objective problems by using a soft archive threshold (learning rate α). I found no audio use. Within 24-100 renders per click, QD does not have the budget to fill an archive (inference). QD fits better as an **offline** step that builds a per-profile archive of diverse, gate-passing "seed" patches shipped as data. Generate then mutates around archive cells and gets diversity for free.
- **Shier et al. (NIME 2025):** a GA builds preset datasets for black-box VSTs toward target timbres ([NIME](https://nime.org/proc/nime2025_66/)).

### 2f. Multi-objective (Pareto) selection
No audio paper applies Pareto selection to presets directly. GenSR-Pref's unanimity rule (2b) is the closest. Practical recipe (inference): objectives = {clarity, −roughness, pitch stability, trait fit to targets, novelty}. Keep the non-dominated front among gate-passers, then softmax-pick within the front using the Bradley-Terry taste score. This stops any single trait from buying its way past mush, and it needs no weight tuning.

### 2g. Other 2025-26 preset work (context)
SynthCloner (ICASSP 2026) factorises envelope, timbre and content for preset conversion ([arXiv 2509.24286](https://arxiv.org/abs/2509.24286v1)). Le Vaillant's thesis (UMONS 2025) covers synth-agnostic sound matching and VAE preset interpolation ([ORBi](https://orbi.umons.ac.be/handle/20.500.12907/52109)). Commercial: MicroMusic Replicate (audio → Vital preset), and Synplant 2 Genopatch/PhenoType (already known).

---

## 3. Ranked: biggest gain for least work in Zero Engine

| # | Change | Work | Why it gives the most | How to verify |
|---|---|---|---|---|
| 1 | **Hard gates before scoring**: clarity (median over frames) + pitch offset/IQR (SHS on the known note) + tonality α; candidates that fail are never picked | ~1 day; `clarityOf` exists, add SHS f0 track | Removes the reported failure outright; SCORE/TuneJury show scalar rewards get gamed | Label ~100 Generate outputs good/mush by ear (owner). Gate ROC/AUC ≥ 0.85; pick thresholds where the 72 designed profiles all pass (they are known-good) |
| 2 | **Movement and width credited only on harmonic amplitudes** (a_k(t) change; side energy inside harmonic windows; mono-corr floor) | ~1 day | Takes away detune and wobble as ways to earn the reward | Sweep unison detune 0→50 c and pitch-LFO depth on one profile; movement credit must stay flat while a filter-LFO sweep raises it |
| 3 | **Sethares roughness from top-40 peaks**, per-type cap | half a day | Catches bright wide detune and FM clang that clarity windows tolerate | Same detune sweep: roughness rises monotonically past about 15 c on bright leads; designed profiles sit under the cap |
| 4 | **z-score traits across the click's candidates** (SCORE) before the capped sum | an hour | Stops one large-scale trait dominating | Log the per-trait contribution share of the winner over 500 clicks; no trait > 40% on average |
| 5 | **Pareto front among passers, then taste softmax** | half a day | Removes weight tuning; no single objective buys a win | `gencheck` pass rate unchanged or better; owner A/B blind on 20 clicks old vs new |
| 6 | **Surrogate screen** (GBT/MLP on params → pass + traits, trained from `gencheck` renders) | 3-5 days incl. data | 10-50x more candidates per click at the same CPU | Holdout precision of "pass" ≥ 0.8; per-profile pass-rate of rendered candidates rises; time per click unchanged |
| 7 | **Offline QD archive per profile** (CMA-MAE or MAP-Elites, learned PCA descriptors, gates as feasibility) shipped as seeds | 1-2 weeks | Diversity without spending click budget on exploration | Archive coverage; owner rates variety; novelty of outputs vs. the current Generate |
| 8 | Audiobox PQ as offline teacher distilled into the surrogate | 1 week + licence check | Adds a human-trained quality signal | Correlation of distilled PQ vs. teacher ≥ 0.7; owner preference agreement |
| 9 | RL/diffusion generator (DDSynth-RL-style), LLM-evolved mutation rules | weeks | Heavy and uncertain for a profile-variation tool | Only if 1-6 plateau |

Items 1-4 together are about 3 days and target exactly the "out-of-tune, smeared, just noise" outputs. Keep-scope note: 1-5 are scoring changes only, with no new models and no UI. Item 6 is the first one that adds a learned component.

### Calibration protocol (applies to 1-3)
1. Render all 72 profiles: these define the "must pass" floor per type.
2. Render around 20 Generate candidates per profile under the current scorer and have the owner tag the worst ones (or use the ones already reported).
3. Per type, set each threshold midway between the 5th percentile of designed profiles and the median of tagged-bad ones. Report false-reject rate on designed profiles (target 0) and catch rate on tagged-bad (target ≥ 80%).
4. Add a `ZeroAnalyze test` case: a deliberately detuned (±40 c, 7 voices) and pitch-wobbled variant of a lead must fail the gate, and its undetuned version must pass.
