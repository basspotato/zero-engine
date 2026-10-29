# Evolutionary, search-based and quality-diversity methods for generating synth patches

Scope note: 15 web searches; WebFetch was not available, so most findings come from abstracts, search excerpts, project pages and secondary summaries rather than full-text reads. Numbers quoted below are what those excerpts state; where only an abstract was seen, that is flagged.

## 1. Genetic algorithms and interactive evolution for synth sound design (research and commercial randomizers)

### Takeaway
GA sound design splits into (a) target matching, where fitness is distance to a reference (MFCC/spectral), and (b) interactive evolution (IEC), where the user is the fitness function and the system just proposes mutated/bred variants (MutaSynth, Synplant). Commercial "randomize" features are mostly crude (uniform per-parameter randomness with an amount knob) and the community's main complaint is broken, silent or out-of-tune results; the best-reviewed tools constrain randomness by learning from existing presets.

### Cited Findings
**Classic foundations (pre-2018)**
- Horner et al. (1993) used a GA over FM synthesis, optimising modulation indices and carrier/modulator frequencies for various numbers of carriers ("Machine Tongues XVI"); best results came from invariant modulation indices and a multiple-carrier formant FM model — [ResearchGate: Machine Tongues XVI](https://www.researchgate.net/publication/269879200_Machine_Tongues_XVI_Genetic_Algorithms_and_Their_Application_to_FM_Matching_Synthesis)
- Johnson (1995, UCT thesis), GA settings for FM: population 50, crossover 60 %, mutation 0.1 % worked best for matching real sounds, fitness levelling off after ~100 generations; for *new-sound* searches (exploration rather than matching) crossover near 100 % and mutation as high as 10 % worked well — [Johnson 1995 thesis](https://people.cs.uct.ac.za/~djohnson/Files/thesis1995.pdf) (figures via search excerpt)
- Dahlstedt's MutaSynth (2001): interactive evolution where an external sound engine is remote-controlled through a genetic representation of its parameter set; variants of genomes are generated, auditioned and selected by the user's aesthetic preference; motivation is that synth parameter spaces of 30-500+ parameters are too big to explore by hand — [Chalmers record](https://research.chalmers.se/en/publication/59129); [Organised Sound 6(2):121-124 / DMU PDF](https://www.dmu.ac.uk/documents/technology-documents/research/mtirc/nowalls/mww-dahlstedt.pdf). The specific mating/mutation operators were not visible in the excerpts.
- Yee-King & Roth: Java system (SynthBot) that programs any VSTi with a GA using MFCC-based fitness — [core.ac.uk (secondary citation)](https://core.ac.uk/download/16235476.pdf); PresetGen/SynthBot notes crossover probability and mutation operators can be tuned for closer matches — [ResearchGate: PresetGen](https://www.researchgate.net/publication/301775101_Automatic_Synthesizer_Preset_Generation_with_PresetGen)
- Macret & Pasquier (2013): multi-objective GA to tune the OP-1 synth automatically — [ATIAM/IRCAM report](http://www.atiam.ircam.fr/Archives/Stages1112/DROGUET.pdf) (via search summary)
- Mitchell: advanced evolutionary algorithms for difficult FM sound-matching problems — [UWE repository](https://uwe-repository.worktribe.com/output/946187/automated-evolutionary-synthesis-matching-advanced-evolutionary-algorithms-for-difficult-sound-matching-problems-advanced-evolutionary-algorithms-for-difficult-sound-matching-problems)
- Smith, "Play it Again": GA run time ranged from seconds to hours across software synths and effects — [IU ScholarWorks](https://scholarworks.indianapolis.iu.edu/server/api/core/bitstreams/eacd59fd-359a-406d-b55d-e43a23595287/content)
- A comparison of parametric optimisation techniques for instrument tone matching identifies optimizer limitations (search-space difficulty) as a main source of match inaccuracy — (search summary of "A Comparison of Parametric Optimisation Techniques for Musical Instrument Tone Matching"; no direct URL captured)

**2018+ research**
- Yee-King, Fedden & d'Inverno (2018, IEEE TETCI): compared hill climber, GA and three deep nets for sound matching on Dexed (DX7); a bidirectional LSTM with highway layers was best, matching closely in 25 % of test cases, and runs near real-time once trained, unlike GA search — [Goldsmiths repository](https://research.gold.ac.uk/22516); [IEEE](https://ieeexplore.ieee.org/abstract/document/8323327). GA per-render timings not in the excerpt.

**Synplant (Sonic Charge) – IEC with genes**
- A patch is drawn as a plant; the original (2008) had a seed in the centre with 12 branches; growing a branch gradually mutates the sound away from the seed; planting a branch makes it the new seed (iterative breeding/hill-climbing by ear) — [CDM](https://cdm.link/synplant-2-hands-on/); [sonicfield.org](https://sonicfield.org/phenotype-synplant-2-text-to-patch); [SonicState 2008](https://sonicstate.com/news/2008/11/10/sow-sonic-seeds/)
- Mutation is seed-based: a forum feature request notes you cannot tweak genes per branch — [Sonic Charge forum](https://cdn.soniccharge.com/forum/topic/2490-feature-request-per-branch-gene-editing?~post12852)
- Synplant 2 (Oct 2023): rebuilt engine, more genes, DNA editor (spiral overview of the gene set, coloured lines show how genes move settings), MIDI CC automation of every gene, JavaScript scripting — [Synthanatomy](https://synthanatomy.com/2023/10/sonic-charge-synplant-2-popular-synth-plugin-gets-a-genetic-sound-design-boost.html); [Rekkerd](https://rekkerd.org/sonic-charge-releases-synplant-2-with-genopatch-technology/)
- Genopatch: ML that takes ≤2 s of a sample, "makes educated guesses about optimal synth settings, listening to the outcomes, and iteratively refining them" (render-in-the-loop); UI shows four strands growing as it searches; results range from close matches to unexpected surprises; runs locally, no internet — [Synplant 2.0.2 user guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=); [MusicTech review](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/); [Synthtopia](https://www.synthtopia.com/content/2023/10/04/synplant-2-lets-you-create-synth-patches-from-samples-using-artificial-intelligence/)

**Commercial / community randomizers**
- Dexed: no dedicated randomizer; the developer made the sysex loader accept files with a bad checksum, with every parameter filtered into range, "so there is some kind of randomize patch feature" — [Dexed GitHub issue 389](https://www.github.com/asb2m10/dexed/issues/389) (via search excerpt)
- "This DX7 Cartridge Does Not Exist" (neuralDX7): a VAE squeezes the DX7's 155 parameters to 8 latent dims and samples there, giving random-but-plausible patches — [MusicTech](https://musictech.com/news/ai-generated-fm-patches-free); [PyPI neuralDX7](https://libraries.io/pypi/neuralDX7)
- u-he presets (Diva, Hive etc.) — community tool by Fannon (open source, npm `u-he-preset-randomizer`): three modes: (1) fully random presets drawn from the *real value distributions in your preset library*, (2) randomize an existing preset by `--randomness` 0-100 %, (3) merge several presets. A "stable mode" that keeps unstable parameters (e.g. pitch) fixed is listed only as a future idea. An early user reported patches that went silent, odd pitch variations and "nonsense variations" and asked for stability controls — [unpkg README](https://unpkg.com/u-he-preset-randomizer@1.2.0/README.md); [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?p=8898478)
- Vital: community Mod Matrix Randomizer app writes randomized presets from an input folder (output count multiplies with inputs; use 1-2 inputs); `vitfov` Python package to split presets into components and randomize sections (described as unfinished) — [Vital forum](https://forum.vital.audio/t/mod-matrix-randomizer-by-me/6463); [Vital forum: fully randomized preset](https://forum.vital.audio/t/fully-randomized-vital-preset/11213). Preset Mutator (Kreativ Sound, web, beta) generates Vital variants; Serum 2 and Pigments 7 planned — [kreativsound.com](https://kreativsound.com/preset-mutator/)
- Pigments: randomness lives in modulation (purple "Randomizers" category, S&H and Turing-style random), granular random pitch/start/size/volume, analog drift, and per-track Randomize amount (0-1) + Regen in the sequencer — [Pigments 5 manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf); [IDM tutorial](https://idmmag.com/tech/tutorials/arturia-pigments-2-creating-melodies-leads/)
- Serum 2: the manual excerpt (v2.0.18, manual 1.0.3, April 2025) and launch coverage did not show a whole-patch randomize button; KVR users achieve per-voice randomness with note-on discrete random or a retriggered S&H LFO at speed 0 — [Serum 2 manual](https://xferrecords.com/manual/serum-2); [KVR](https://www.kvraudio.com/forum/viewtopic.php?p=9059297)

### Inferences
- Exploration vs. matching want different operator settings: Johnson's numbers suggest roughly 10x-100x higher mutation and near-always crossover when the goal is new sounds, vs. low mutation for matching.
- The most robust "randomizers" don't sample uniformly: they sample from the distribution of existing good presets (u-he tool mode 1, neuralDX7 VAE) or mutate around a known-good seed (Synplant). For Zero Engine this matches the current design (variations around designed profiles, checked against the owner's sounds); a library-distribution prior per parameter is a cheap next step.
- Locking pitch/tuning-like parameters ("stable mode") is the commonly requested guard against unusable results.

### Gaps
- No primary documentation found for built-in randomizers in Vital, Phase Plant, Serum 2 or Pigments' preset-level features; I could not confirm whether they exist or how they constrain results.
- Synplant's actual mutation maths (per-gene step sizes, which genes mutate per branch) is not public in the sources found.
- MutaSynth's exact operators (Dahlstedt describes mating/insemination/morphing in the full paper) not verified from text.

## 2. Novelty search and quality-diversity (MAP-Elites etc.) for sound

### Takeaway
The main body of QD-for-sound work is Björn Þór Jónsson et al. (RITMO/Oslo, kromosynth/synth.is), evolving CPPN + DSP-graph genomes with MAP-Elites over behaviour spaces first defined by a classifier (YAMNet), then by unsupervised PCA/autoencoder projections that are retrained during the run, which gave significantly more diversity than hand-made descriptor spaces. Masuda & Saito showed novelty search (centroid + flatness descriptors) improves GA sound matching on real synths (Dexed, Diva) and yields several distinct good matches.

### Cited Findings
**Foundations**
- MAP-Elites (Mouret & Clune 2015): keep one elite per cell of a user-defined feature (behaviour) grid, so diversity is held regardless of fitness — [arXiv 1504.04909](https://arxiv.org/pdf/1504.04909); overview claims it consistently beats traditional EAs, MOEAs and pure novelty search on coverage and QD-score — [emergentmind overview (aggregator)](https://www.emergentmind.com/topics/map-elites-algorithm)

**Masuda & Saito – QD for synth sound matching (DAFx 2020in21; extended J. Inf. Process. 31, 2023)**
- Add a novelty objective to GA sound matching; it keeps the population phenotypically diverse, which *improves* match quality, and the QD version returns a varied set of good matches differing in specified audio features — [J-STAGE](https://www.jstage.jst.go.jp/article/ipsjjip/31/0/31_220/_article/-char/en); [DAFx paper](https://dafx2020.mdw.ac.at/proceedings/papers/DAFx20in21_paper_46.pdf)
- Variants: Novelty Search with Global Competition (NS-GC) and Local Competition (NS-LC); behaviour descriptors spectral centroid and spectral flatness; tested on Dexed and u-he Diva with in-domain presets and out-of-domain NSynth sounds; novelty variants cover a wider descriptor region than plain GA — [accompanying site](https://hyakuchiki.github.io/QDSynthWebpage); [Grouwels 2024 novelty paper (secondary summary)](https://zenodo.org/records/11396082/files/grouwels24_novelty.pdf)

**Jónsson, Erdem, Fasciani, Glette – kromosynth / synth.is**
- Genome = CPPN + DSP audio graph, both evolved with NEAT (add/mutate nodes and connections); system paper in J. Audio Eng. Soc. 72(4), 2024 — [JAES paper PDF](https://www.uio.no/ritmo/english/people/postdoctoral-fellows/bthj/publications/a_system_for_sonic_explorations_with_evolutionary_algorithms-oa.pdf); EvoMUSART 2024 "Towards Sound Innovation Engines Using Pattern-Producing Networks and Audio Graphs" — [Springer](https://link.springer.com/chapter/10.1007/978-3-031-56992-0_14)
- ICCC 2024 "Cultivating Open-Earedness…": behaviour space defined by a pre-trained classifier; sounds released on the kromo.synth.is label where artists use only evolved sounds — [ICCC24 paper](https://computationalcreativity.net/iccc24/papers/ICCC24_paper_175.pdf); [kromo.synth.is](https://kromo.synth.is/); [RITMO project page](https://www.uio.no/ritmo/english/people/postdoctoral-fellows/bthj/kromosynth.html)
- arXiv 2512.02783 (Dec 2025) "Exploring Definitions of Quality and Diversity in Sonic Measurement Spaces": hand-crafted descriptors or supervised classifiers bias discovery; they build the MAP-Elites behaviour space with unsupervised PCA and autoencoders, *retrained on sounds found during the search* to avoid stagnation; unsupervised spaces give significantly higher diversity than the manual baseline (a 2-D space from paired instantaneous audio descriptors); quality kept separate, via reference-based similarity (to NSynth) and reference-free audio quality assessment; k = 15 configuration reached diversity 0.801 ± 0.049; used only NEAT mutation (no speciation or crossover), CPPN activation functions restricted to sine/square/saw/triangle — [arXiv PDF](https://arxiv.org/pdf/2512.02783); [Oslo dataverse data](https://dataverse.no/dataset.xhtml?persistentId=doi%3A10.18710%2FGS4RT0)
- arXiv 2606.09780 (8 June 2026) "Quality-Diversity Search in Sound Generation: Investigating Innovation Engines for Audio Exploration": Innovation Engine = QD + supervised discriminative model; MAP-Elites cells defined by YAMNet classes; 10 runs × 300 k iterations, batch 32, 0.5 s sounds; multiple specialised CPPNs per frequency range give simpler networks with near single-CPPN performance; analyses "goal switches" (stepping stones between musical and non-musical classes); behaviour space extended across durations shows temporal-niche specialisation — [arXiv abs](https://arxiv.org/abs/2606.09780); [HTML](https://arxiv.org/html/2606.09780) (abstract/early sections only seen)
- Code: kromosynth and kromosynth-cli on GitHub/npm/Zenodo — [GitHub synth-is/kromosynth](https://github.com/synth-is/kromosynth); [Zenodo kromosynth-cli](https://zenodo.org/records/10228846). Licence not confirmed in the excerpts (check before reuse).

### Inferences
- "Goal switching" is the key QD benefit for "interesting": a candidate that is mediocre in its own niche becomes the stepping stone to an elite in another. For Zero Engine, a small per-type archive keyed on 2-3 SoundTraits (e.g. brightness × width × movement) with one elite per cell would give Generate a pool that stays varied across clicks, rather than picking the same high scorers.
- Budgets in kromosynth (300 k evaluations per run, offline) are far beyond a plugin click; an archive would have to be built offline (or in the background) and Generate would sample/mutate from it.
- Masuda & Saito's result that novelty also helps matching suggests a novelty term could help Reference Match escape local optima.

### Gaps
- No full-text numbers seen for 2606.09780's comparisons, nor render speeds per evaluation.
- No 2018-2026 paper found applying MAP-Elites directly to a *conventional parametric* synth's presets for generation (as opposed to matching); searches for this were swamped by the GForce "MAP" synth.
- CVT-MAP-Elites and NSLC applied to commercial synths: not found in this search.

## 3. Preset interpolation, blending and morphing

### Takeaway
Linear interpolation in raw parameter space often sounds uneven; the research answer is to interpolate in a learned latent space aligned with audio (Le Vaillant & Dutoit transformer AE; Esling's FlowSynth VAE + normalizing flows), which gives smoother perceptual paths and learned macro controls. A cheap non-ML alternative is mixing within existing good presets (u-he randomizer merge mode).

### Cited Findings
- Le Vaillant & Dutoit, "Synthesizer Preset Interpolation using Transformer Auto-Encoders" (ICASSP 2023, arXiv 2210.16984): bimodal AE (presets via multi-head attention, audio via convolutions), tested on an FM synth (100+ parameters, i.e. Dexed), gives smoother interpolations than related architectures; authors suggest integrating into synths for live interpolation — [arXiv](https://arxiv.org/pdf/2210.16984v2.pdf); code link listed at [sotaverified](https://sotaverified.org/papers/synthesizer-preset-interpolation-using) (availability unconfirmed)
- Esling et al., "Universal Audio Synthesizer Control with Normalizing Flows" (DAFx 2019): VAE + normalizing flows map an audio latent space to synth parameters; latent dims separate main factors of variation and serve directly as macro-parameters; supports parameter inference from audio, preset visualisation and interpolation, real-time in Ableton Live; IJCAI 2020 demo (FlowSynth) controlled hundreds of parameters with one hand via LeapMotion — [arXiv 1907.00971](https://arxiv.org/abs/1907.00971v1); [IJCAI 2020](https://www.ijcai.org/proceedings/2020/767); [ACIDS FlowSynth](https://acids.ircam.fr/flowsynth)
- neuralDX7: 8-D VAE latent for DX7 patches, sample = new patch — [MusicTech](https://musictech.com/news/ai-generated-fm-patches-free)
- u-he-preset-randomizer "merge" mode combines multiple presets, optionally adding randomness — [README](https://unpkg.com/u-he-preset-randomizer@1.2.0/README.md)

### Inferences
- In parameter space, blends behave better when discrete/structural parameters (wavetable choice, filter type, routing) are taken whole from one parent and only continuous ones are interpolated, and when log-scaled values (frequencies, times) are interpolated in log domain; this is standard practice but was not sourced in this search. Zero Engine's any-genre blend (25-75 % mix of searched settings, loaded as the dominant parent's genre) is consistent with this; rendering and checking each blend (as it does) is the safeguard the research approaches replace with a learned latent.

### Gaps
- Didn't find sourced, quantitative listening studies on raw-parameter vs latent interpolation beyond the "smoother" claim.
- Did not verify Dahlstedt's morphing/mating operators or any commercial synth's morph feature (e.g. Phase Plant, Pigments).

## 4. Practical tricks: musical constraints, guarding against silence/clipping/noise, playability

### Takeaway
Sources consistently report the same failure modes (silence, bad pitch, nonsense) for unconstrained randomization; the documented fixes are: sample from distributions of known-good presets, mutate around a good seed, lock pitch-like parameters, clamp every parameter to valid ranges, separate "quality" from "diversity" checks, and render-in-the-loop with an objective check before showing a result.

### Cited Findings
- Silence/nonsense from naive randomizing and request for stability locks — [KVR u-he randomizer thread](https://www.kvraudio.com/forum/viewtopic.php?p=8898478); planned "stable mode" excluding pitch etc. — [README](https://unpkg.com/u-he-preset-randomizer@1.2.0/README.md)
- Range-filtering every parameter on random sysex load (Dexed) — [Dexed issue 389](https://www.github.com/asb2m10/dexed/issues/389)
- Library-distribution sampling as the default random mode — [README](https://unpkg.com/u-he-preset-randomizer@1.2.0/README.md); latent-space sampling (neuralDX7) — [MusicTech](https://musictech.com/news/ai-generated-fm-patches-free)
- Quality judged separately from behaviour: reference similarity + reference-free audio-quality assessment, with diversity in the archive dimensions — [arXiv 2512.02783](https://arxiv.org/pdf/2512.02783); classifier (YAMNet) confidence as quality in Innovation Engines — [arXiv 2606.09780](https://arxiv.org/abs/2606.09780)
- Synplant's growth metaphor keeps variants near the seed (small steps outward), so most branches stay usable — [CDM](https://cdm.link/synplant-2-hands-on/)
- Iterative render-listen-refine (Genopatch) with up to ~2 s of reference audio — [Synplant user guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=); [MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/)

### Inferences
- For a plugin-side Generate: (1) bound mutation step by distance from a designed seed; (2) lock tuning/octave unless explicitly varied; (3) render a short note (and ideally a low and high note for keyboard playability, which none of the sources tested) and reject on silence, clipping, tail not dying, or noise between harmonics; (4) keep a novelty term vs. recent history so successive clicks differ. Zero Engine already does (3) and partly (4); keyboard-span checks appear absent from the literature found.

### Gaps
- No source found that explicitly tests playability across the keyboard (multi-pitch fitness) in preset generation.
- No sourced render-budget/speed figures for real-time-in-plugin evolutionary generation (Synplant/Genopatch timing not published in what was found).
