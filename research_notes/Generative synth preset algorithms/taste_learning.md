# Learning one user's taste from few interactions (preference learning for a generative synth)

Scope: methods that learn a single user's preferences over generated synth sounds from 20-200 judgments (ratings, picks, favourites, skips), 2010-2026, with an eye to what fits Zero Engine's Generate (24 candidates measured on SoundTraits, history, favourites in `%APPDATA%/Zero Engine/Favourites`) and C++/CPU constraints.

Method note: this was search-only research (abstracts, project pages, summaries); full texts were not read, so quantitative claims are as stated by the authors' abstracts or by secondary summaries where marked.

## Which methods learn useful preferences from 20-200 judgments? (examples in music, sound and creative tools)

### Takeaway
Three families work at this budget: (1) Gaussian-process preference models driving Bayesian optimisation (preferential BO, "pick one from a gallery", sequential line/plane search), which are explicitly designed to minimise human queries; (2) interactive evolution (IEC/IGA, Synplant-style planting), which works but is limited by user fatigue unless a surrogate predictor of the user's ratings is added; (3) pooling prior knowledge (other users, or a base judge) and personalising with a handful of examples. For sound specifically, the strongest published evidence is small: GP-based EQ personalisation beat random search with 12 listeners, GP active learning learned synth "knobs" faster than baselines in simulation, and SocialEQ needed ~25 ratings per user before transfer learning cut that.

### Cited Findings
**Preferential / human-in-the-loop Bayesian optimisation**
- Chu & Ghahramani (ICML 2005) define the base model most later work uses: a latent utility function with a GP prior and a probit likelihood on pairwise preferences; problem size grows linearly with the number of training instances rather than quadratically — [Chu & Ghahramani 2005](https://mlg.eng.cam.ac.uk/zoubin/papers/icml05chuwei-pl.pdf); [mlanthology entry](https://mlanthology.org/icml/2005/chu2005icml-preference)
- Preferential Bayesian Optimisation (González, Dai, Damianou, Lawrence, ICML 2017) finds the optimum of a latent function queried only through pairwise "duels", modelling win probability with a GP + Bernoulli likelihood; authors conclude that modelling correlations is the key to reducing the number of comparisons needed — [PMLR](https://proceedings.mlr.press/v70/gonzalez17a); [arXiv 1704.03651](https://ar5iv.labs.arxiv.org/html/1704.03651)
- Brochu's line of work applied interactive BO to graphics/animation: "Preference galleries for material design" (SIGGRAPH posters 2007), "A Bayesian Interactive Optimization Approach to Procedural Animation Design" (SCA 2010), PhD thesis "Interactive Bayesian optimization: learning user preferences for graphics and animation" (2010); the BO tutorial (2010) covers "active user modelling with preferences" — [DBLP Brochu](https://dblp.uni-trier.de/pid/50/2804.html); [Brochu, Cora, de Freitas tutorial arXiv 1012.2599](https://arxiv.org/pdf/1012.2599)
- Sequential Line Search (Koyama, Sato, Sakamoto, Igarashi, SIGGRAPH 2017, ACM TOG 36(4)): breaks a high-dimensional tuning problem into a series of 1-D searches, each done by the human moving one slider; BO picks the slider's line; converged faster than comparison-based microtask designs; demonstrated on photo colour enhancement and BRDF material design — [project page](https://koyama.xyz/project/sequential_line_search/); [preprint](https://koyama.xyz/project/sequential_line_search/download/preprint.pdf)
- Koyama ships a C++ implementation of Sequential Line Search (with Python bindings) — [GitHub mirror](https://github.imc.re/yuki-koyama/sequential-line-search); [README](https://rust-digger.code-maven.com/src/rust_sls-0.1.1/sequential-line-search/README.md)
- Sequential Gallery (Koyama, Sato, Goto, SIGGRAPH 2020): "sequential plane search", the user picks the best option from a 2-D plane shown as a zoomable grid; fewer iterations than baselines on synthetic functions; user study was small (5 students + 1 researcher) and concluded novices could complete search tasks — [arXiv 2005.04107](https://arxiv.org/pdf/2005.04107); [project page](https://koyama.xyz/project/sequential_gallery)
- Music application: Zhou, Koyama, Goto, Igarashi (2020) "Generative Melody Composition with Human-in-the-Loop BO": system generates candidate melodies, user picks the best, BO drives candidate generation to reduce rounds; evaluated by simulation and a pilot user study only — [arXiv 2010.03190](https://arxiv.org/pdf/2010.03190.pdf)
- Huang, Duvenaud, Arnold, Partridge, Oberholtzer, Gajos (IUI 2014) "Active Learning of Intuitive Control Knobs for Synthesizers Using Gaussian Processes": learns GP maps from synth settings to perceived qualities ("scary", "steady"), uses GP uncertainty to choose which sounds the user rates, and builds high-level knobs from them; in simulation it learned knobs faster than several baselines for certain classes of target concept — [paper PDF](https://kgajos.seas.harvard.edu/papers/huang14knobs.pdf); [Harvard page](https://www.eecs.harvard.edu/~kgajos/papers/2014/huang14active.shtml)
- "Personalized Audio Systems — a Bayesian Approach" (AES 135th, 2013): GP regression models one listener's objective over a 5-band EQ; with 12 subjects, the settings found were significantly preferred to those from random experimentation — [AES E-Library](https://www.aes.org/e-lib/browse.cfm?elib=17048); [DTU record](https://www2.imm.dtu.dk/pubdb/pubs/6567-full.html)
- Other PBO applications with small expert studies: constrained PBO for banner ad design with 11 professional designers' pairwise preferences — [arXiv 2505.10954](https://arxiv.org/pdf/2505.10954)
- Gap noted by search: no published preferential-BO user study specifically on synthesizer preset design was found — (search result, see Gaps)

**Relevance feedback / rating-based sound search**
- SynthAssist (Cartwright & Pardo, NIME 2014): search synth patch space by vocal imitation, then the user rates presented sounds; ratings refine the query and feature weights via relevance feedback — [NIME 2014](https://nime.org/proc/mcartwright2014); [paper PDF](https://interactiveaudiolab.github.io/assets/papers/cartwright_pardo_acmmm14.pdf)
- SocialEQ (Pardo lab): typically needs ~25 ratings to personalise an EQ setting; Kim & Pardo (ICMLA 2014) proposed three methods to cut this, tested on 1,635 user sessions; collaborative filtering over prior users' ratings "greatly reduce[s]" ratings needed — [Kim & Pardo ICMLA 2014](https://interactiveaudiolab.github.io/assets/papers/Kim_Pardo_ICMLA2014.pdf); [HCOMP paper](https://bongjun.github.io/pages/files/papers/HCOMP_CRC_Kim.pdf)
- A Pardo-group patent describes active + transfer learning reducing ratings from ~30 to 1-3 (patent claim, not peer-reviewed) — [US20140272883A1](https://www.google.com.au/patents/US20140272883)

**Interactive evolution (IEC/IGA)**
- McDermott, O'Neill, Griffith (Evolutionary Computation 18, 2010) "Interactive EC control of synthesized timbre": the biggest limit of IEC is the fitness-evaluation bottleneck (slow human evaluation → small populations, user fatigue); new UI designs that speed evaluation improved performance — [Univ. of Galway record](https://research.universityofgalway.ie/en/publications/interactive-ec-control-of-synthesized-timbre-3/); [PDF](https://typeset.io/pdf/interactive-ec-control-of-synthesized-timbre-1mwnb66ioj.pdf)
- Kaliakatsos-Papakostas et al. (Soft Computing 2012): evolution must surface appealing results quickly to offset fatigue; user control over randomness via modified GP operators drove evolution to more user-preferred sounds — [PDF](https://www.epitropakis.co.uk/sites/default/files/pubs/KaliakatsosEFVevoSIrev.pdf)
- Quiroz et al. (IEEE TEC 2009): keep a large population but show the user a small subset and ask only every few generations; on OneMax this cut user evaluations tenfold vs a standard small-population IGA — [PDF](https://www.cse.unr.edu/~quiroz/inc/docs/trans2009.pdf)
- Wang, Wang, Takagi: predicting the user's ratings is a way to reduce IEC fatigue; a predictor of absolute ratings converged faster than one on relative ratings; predictors using recent data beat ones using all past data — [WCCI 2006](https://catalog.lib.kyushu-u.ac.jp/opac_download_md/1808898/WCCI2006wang.pdf); [2005 paper](https://catalog.lib.kyushu-u.ac.jp/opac_download_md/1808896/WSTST2005.pdf)
- Takagi's 1998 overview of IEC (art, engineering, education; ~90 refs) — [PDF](https://dendrit.tuke.sk/~newalife/kapitola/846/ines98.pdf)
- Engagement-adaptive IGA for MIDI melodies (CEUR Vol-4158): mutation/injection rates adapted to tracked engagement; users rated compositions attractive after ~4.6 iterations on average (small volunteer study) — [CEUR](https://ceur-ws.org/Vol-4158/Short02.pdf)
- Picbreeder (Secretan, Stanley et al., CHI 2008 / Evolutionary Computation 2011): collaborative interactive evolution; branching from others' images is the key mechanism; discoveries often hinged on serendipity — [EC 2011 record](https://stars.library.ucf.edu/facultybib2010/1881); [arXiv 2605.23908 (2026 replication)](https://arxiv.org/html/2605.23908)
- Synplant: plant a seed, branches grow variant patches radially from it (the user "keeps" a branch by choosing it); Synplant 2 adds Genopatch (local AI patch matching from audio) and a DNA editor; branches carry random permutations of the seed — [CDM hands-on](https://cdm.link/2023/10/synplant-2-hands-on/); [rekkerd](https://rekkerd.org/sonic-charge-releases-synplant/); [Sonic Charge forum](https://cdn.soniccharge.com/forum/topic/2500-synplant-2-scripting)

**Interactive ML by example**
- Wekinator (Fiebrink, Trueman, Cook, NIME 2009): users train models by demonstration, test, and fix mistakes by adding a few examples and retraining — the loop of small, user-curated training sets — [NIME 2009](https://nime.org/proc/nime2009_fiebrink/); [Fiebrink & Caramiaux arXiv 1611.00379](https://arxiv.org/pdf/1611.00379)

**Few-shot personalisation of a judge/generator**
- PPD (Dang et al., CVPR 2025): a VLM turns a few pairwise preferences into a user embedding; with as few as 4 preference examples from a new user, 76% average win rate over the base Stable Cascade — [arXiv 2501.06655](https://arxiv.org/abs/2501.06655v1); [CVPR paper](https://openaccess.thecvf.com/content/CVPR2025/papers/Dang_Personalized_Preference_Fine-tuning_of_Diffusion_Models_CVPR_2025_paper.pdf)
- PIPE (Google Research): models a user's reward as a linear combination of K base "user types", fitting user weights by logistic regression on pairwise data — [Google Research](https://research.google/pubs/pipe-personalized-image-generation-via-preference-encoding/)

### Inferences
- The recurring recipe that fits 20-200 judgments is: a fixed, informative feature space (for Zero Engine: SoundTraits + key parameters) + a low-capacity utility model (GP or linear) + a query strategy that uses model uncertainty. Each paper above that reports success uses tens, not thousands, of judgments, but all their user studies are small (6-12 people), so effect sizes are not well established.
- PIPE's "linear combination of base types" maps directly onto Zero Engine: the existing per-genre/type targets from the owner's 1,187 sounds can be the prior ("base taste"), and a few weights learned from the owner's picks shift it. This is the transfer-learning idea behind SocialEQ too.
- Synplant's design is effectively IEC with a population of one parent and n children chosen by the user; the "keep" is the selection signal. Zero Engine's Generate + history already resembles this; logging which candidate was kept supplies preference data for free.
- Sequential Line Search's single-slider idea could map onto a "more like this ↔ less" slider between the current sound and a BO-proposed direction, but no audio study of it was found.

### Gaps
- No published preferential-BO or sequential-line-search user study on synthesizer presets was found; evidence is transferred from images, EQ, melody and simulated synth knobs.
- Sequential Gallery's quantitative iteration counts and Huang 2014's exact numbers of ratings were not in the abstracts read.
- The Takagi 2001 Proceedings of the IEEE IEC survey (often cited for fatigue limits, e.g. ~10-20 generations) was not retrieved, so specific fatigue thresholds are unverified.
- No public data on how many keeps/branches Synplant users make before settling.

## Explicit vs implicit feedback: which signals are reliable (favourites, quick re-generate as dislike, play time)?

### Takeaway
Explicit relative judgments (pick A over B, pick best of a set) are more reliable than absolute ratings; implicit signals are useful but biased and noisy, and should be treated as relative preferences, weighted by confidence, and combined rather than taken as labels. A favourite save is the strongest implicit positive; a quick re-generate or skip is a weak negative; listening time needs thresholds and repeats to mean much. No study was found that validates "regenerate quickly = dislike" for generative tools.

### Cited Findings
- Joachims et al. (SIGIR 2005), using eye tracking vs manual relevance judgments: clicks are informative but biased, should not be treated as absolute relevance labels, while relative preferences derived from them are reasonably accurate on average — [SIGIR PDF](https://www.sigir.org/wp-content/uploads/2017/06/p003.pdf); [PSU record](https://pure.psu.edu/en/publications/accurately-interpreting-clickthrough-data-as-implicit-feedback/)
- Music streaming: in lean-back listening, recommenders rely mostly on implicit cues such as skips; skips are treated as dissatisfaction — [arXiv 2301.03881](https://arxiv.org/pdf/2301.03881)
- Hanna et al.: many tracks that score well on offline metrics still led to skips; filtering by listening duration and replays improved recommendations — [arXiv 1711.05237](https://export.arxiv.org/abs/1711.05237); [DeepAI](https://deepai.org/publication/considering-durations-and-replays-to-improve-music-recommender-systems)
- Deezer (UMAP 2025): implicit feedback is noisy; a skip is conventionally a track dropped before ~30 s, a convention not a precise measure; repeated listens are used to estimate each interaction's reliability with a Bayesian beta prior, beating traditional weighting on Deezer and Last.fm (via secondary summary) — [Deezer newsroom](https://newsroom-deezer.com/2025/06/umap-sguerra-html/); [Moonlight review](https://www.themoonlight.io/review/uncertainty-in-repeated-implicit-feedback-as-a-measure-of-reliability)
- Pairwise vs absolute ratings in audio: direct numerical ratings suffer from scale-calibration inconsistencies between assessors; pairwise comparisons give lower measurement error and less fatigue but scale quadratically — [Webb & Picinali, Forum Acusticum 2025](https://dael.euracoustics.org/confs/fa2025/data/articles/000660.pdf); pairwise generally more sensitive and less noisy, Bayesian sampling of pairs most robust — [arXiv 2508.17840](https://arxiv.org/pdf/2508.17840); MOS and scaled pairwise results correlate strongly linearly but differ by content — [Zerman et al. 2018](https://www.cl.cam.ac.uk/~rkm38/pdfs/zerman2018cross_content.pdf); paired comparisons were very stable across loudspeaker assessment procedures — [Applied Acoustics 2013 record](https://lib.hcmut.edu.vn/a-comparative-study-on-different-assessment-procedures-applied-to-loudspeaker-sound-quali-748)
- Contrary datapoint: in IEC, a predictor trained on absolute ratings converged faster than one trained on relative ratings — [Wang, Wang, Takagi WCCI 2006](https://catalog.lib.kyushu-u.ac.jp/opac_download_md/1808898/WCCI2006wang.pdf)
- Generative-AI tools: practitioner guidance lists copy/use-without-edit as positive implicit signals and regenerate/heavy-edit as negative, warns no single signal is reliable (not peer-reviewed) — [user-satisfaction-signals guide](https://www.skills.sh/owl-listener/ai-design-skills/user-satisfaction-signals); an opinion piece argues silent repeated regeneration may predict churn better than ratings (not empirical) — [tianpan.co blog](https://tianpan.co/blog/2026/04/20/behavioral-signals-ai-user-satisfaction); a 16-person study found users gaze longer at image regions they want regenerated — [Oldenburg publications](https://scalableinteractionparadigms.uni-oldenburg.de/blog/publications/?tgid=104)
- UI pattern evidence: speeding up evaluation (UI design) was the main IEC improvement for timbre — [McDermott et al. 2010](https://research.universityofgalway.ie/en/publications/interactive-ec-control-of-synthesized-timbre-3/); asking for a judgment only every few generations and showing a subset cut evaluations tenfold — [Quiroz et al. 2009](https://www.cse.unr.edu/~quiroz/inc/docs/trans2009.pdf); "pick the best from a gallery/grid" as the single interaction — [Sequential Gallery](https://arxiv.org/pdf/2005.04107), [Zhou et al. 2020](https://arxiv.org/pdf/2010.03190.pdf)

### Inferences
- For Zero Engine, a ranked signal ladder (strongest first): favourite saved > sound kept/exported/used in a session (if detectable, e.g. Generate result not replaced before the host saves or the plugin is closed) > picked from history (going back to it) > played for a while > replaced by Generate within a few seconds (weak negative). Each should be stored as a relative preference ("kept X over the candidates/previous Y"), following Joachims, not as an absolute label.
- "Quick re-generate" confounds dislike with "just browsing"; weighting it low (or only counting it as "the next one beat this one" when the user then keeps the next) is safer. Repetition (coming back to a favourite or history item several times) is a reliability multiplier, as in the Deezer beta-prior approach.
- Time-listened in a plugin is dominated by whether the host is playing; it is likely the least reliable signal and should be capped or ignored unless the user is auditioning notes.
- Favourites are positive-only; negatives must come from the alternatives the user saw and didn't keep (implicit pairwise: favourite > the other candidates shown at that moment).
- Low-annoyance UI options supported by the literature: one gesture per round (pick/keep), no mandatory rating scales, optional thumbs on history items, and asking at most occasionally.

### Gaps
- No peer-reviewed study validates regenerate-speed or dwell time as dislike/like signals in generative creative tools (search found only practitioner guides and opinion).
- No study found on implicit feedback inside DAW plugins specifically.

## How to combine learned taste with exploration so outputs don't collapse to one style

### Takeaway
Use the learned taste as a soft term, not a filter: acquisition functions (expected improvement/UCB/Thompson sampling) naturally trade off exploitation and exploration; sets of candidates should be chosen for relevance plus diversity (DPP-style or quality-diversity archives); and giving the user a visible exploration/exploitation control improved engagement in a melody study. Recency-weighting the taste model also stops it from locking onto early choices.

### Cited Findings
- Koyama group IUI 2021 "Interactive Exploration-Exploitation Balancing for Generative Melody Composition": users adjust, per iteration, the balance between diverse (exploration) and focused (exploitation) candidates in a preferential BO loop; authors report this could improve engagement and optimisation performance — [project page](https://koyama.xyz/project/interactive-exploration-exploitation-balancing-for-generative-melody-composition); [author version](https://www.koyama.xyz/project/interactive-exploration-exploitation-balancing-for-generative-melody-composition/author-version.pdf)
- PBO acquisition functions extend standard BO policies to duels, balancing exploration and exploitation through the GP's uncertainty — [González et al. 2017](https://proceedings.mlr.press/v70/gonzalez17a)
- Bayesian active learning for classification and preference learning (BALD, Houlsby et al. 2011) picks queries that maximise information about the model — [arXiv 1112.5745](https://arxiv.org/pdf/1112.5745)
- DC2B (AAAI 2020): a determinantal point process selects item sets that are both relevant and diverse in interactive recommendation with implicit feedback, with a Thompson-sampling-style variational Bayesian learner; reports balancing accuracy and diversity — [AAAI](https://ojs.aaai.org/index.php/AAAI/article/view/5931); [arXiv 1907.01647](https://arxiv.org/pdf/1907.01647)
- D2RL keeps a personalised DPP kernel per user (fixed item-similarity matrix + learned relevance) — [ar5iv 1903.07826](https://ar5iv.arxiv.org/html/1903.07826)
- Quality-diversity for sound: CPPN-DSP generators + MAP-Elites + a classifier produce a wide range of sounds; lineages reach elites by indirect "goal switches" — [Jónsson et al., Sound Innovation Engine](https://uio.no/ritmo/english/people/phd-fellows/bthj/publications/sie1/sound_innovation_engine_v1.pdf); [arXiv 2606.09780](https://arxiv.org/pdf/2606.09780); unsupervised behaviour spaces for sonic MAP-Elites (300K evaluations per run) — [arXiv 2512.02783](https://arxiv.org/pdf/2512.02783)
- Picbreeder: serendipity and branching from others' results drove notable discoveries, an argument against pure exploitation — [arXiv 2605.23908](https://arxiv.org/html/2605.23908)
- IEC predictors trained on recent data beat ones on all past data (taste drifts) — [Wang, Wang, Takagi 2005](https://catalog.lib.kyushu-u.ac.jp/opac_download_md/1808896/WSTST2005.pdf)
- User control of randomness in GP operators led to more preferred sounds — [Kaliakatsos-Papakostas et al. 2012](https://www.epitropakis.co.uk/sites/default/files/pubs/KaliakatsosEFVevoSIrev.pdf)

### Inferences
- Zero Engine's Generate already does softmax over a score with a novelty term; the taste model can be one more capped additive term in that score (like the owner-sound trait guide, "soft, capped"), with softmax temperature acting as the exploration knob. Thompson sampling (sample a utility function from the posterior, score candidates with it) gives exploration for free and is cheap with a linear/Laplace model.
- To avoid collapse, pick the shown/loaded set greedily for score minus similarity to already-chosen ones (a cheap DPP approximation) in SoundTraits space, and reserve a fixed share of draws (e.g. 1 in 4) as pure in-genre exploration that ignores taste. This mirrors Koyama's explicit balance control; a single "Familiar ↔ Surprise" control would be the simple UI form (consistent with the owner's keep-it-simple preference).
- Learn taste per type (bass vs lead vs pad) or with type as a feature; a single global taste would push pads toward lead traits.
- Exponential forgetting (recent judgments weighted more) follows the Takagi-lab result and keeps the model responsive to changing projects.

### Gaps
- No study measured "style collapse" from taste learning in a sound tool directly; the remedies above are borrowed from recommender systems and BO.
- The Koyama IUI 2021 paper's quantitative engagement results were not retrieved.

## Simple models that work in C++ on CPU (logistic regression on features, Gaussian processes, small nets)

### Takeaway
At 20-200 judgments, a Bradley-Terry / logistic model on a few dozen hand-made audio features, or a GP preference model (Laplace approximation) on the same features, is the right size; both are a few hundred lines of C++ and train in milliseconds. Small nets or embedding-based judges need prior data (other users or a pretrained model) and only add value via transfer. Koyama's Sequential Line Search library is existing C++ code for GP preference BO.

### Cited Findings
- GP preference learning (Chu & Ghahramani): latent utility with GP prior, probit pairwise likelihood, cost linear in number of instances — [ICML 2005](https://mlg.eng.cam.ac.uk/zoubin/papers/icml05chuwei-pl.pdf)
- A tutorial on learning from preferences and choices with GPs (2024) covers pairwise and choice (pick-one-of-k) likelihoods — [arXiv 2403.11782](https://arxiv.org/pdf/2403.11782v3.pdf)
- Skew-GP variant for preferential BO (exact posterior rather than Laplace) — [arXiv 2008.06677](https://arxiv.org/pdf/2008.06677)
- Sequential Line Search is implemented in C++ (Eigen-based library with demos and Python bindings) — [GitHub mirror](https://github.imc.re/yuki-koyama/sequential-line-search)
- PIPE learns user weights by plain logistic regression on pairwise data over K base types — [Google Research](https://research.google/pubs/pipe-personalized-image-generation-via-preference-encoding/)
- Wekinator showed general-purpose small supervised learners (classification/regression) trained on few user examples suffice for interactive music use — [NIME 2009](https://nime.org/proc/nime2009_fiebrink/)
- GP regression over a 5-band EQ found significantly preferred settings with 12 listeners — [AES 2013](https://www.aes.org/e-lib/browse.cfm?elib=17048)
- GP active learning over synth parameters learned perceptual knobs faster than baselines — [Huang et al. IUI 2014](https://kgajos.seas.harvard.edu/papers/huang14knobs.pdf)
- PPD's few-shot personalisation (4 examples, 76% win rate) depends on a large pretrained VLM — [arXiv 2501.06655](https://arxiv.org/abs/2501.06655v1)

### Inferences
- Recommended starting model for Zero Engine: Bradley-Terry over a feature vector x (SoundTraits: width, body, density, movement, richness; plus brightness/centroid, % low end, attack/decay times, LUFS-normalised, one-hot type): P(A beats B) = sigmoid(w·(x_A − x_B)), L2 prior pulling w toward 0 (or toward weights derived from the owner's 1,187 sounds). Fit by Newton/IRLS on each new judgment; with ~20 features this is microseconds. A Laplace approximation gives a posterior covariance for Thompson sampling at no extra cost. Pick-one-of-k ("kept this of 24") becomes k−1 pairwise rows or a softmax (Plackett-Luce) likelihood.
- Upgrade path if linear underfits after ~100+ judgments: GP preference model with an RBF kernel on the same standardised features (n ≤ 200 → 200×200 matrix, trivial on CPU), or Koyama's library if a slider/gallery UI is wanted.
- Keep all fitting off the audio thread (Generate's worker pool), store judgments as plain rows next to favourites so the model can be rebuilt, and fall back to the current scoring when there are fewer than ~10 judgments.
- Small neural nets offer little at this data size unless pretrained on engine renders; a tiny MLP over the same features would need regularisation and would lose interpretability (no "you seem to like wide, dark sounds" readout).

### Gaps
- No benchmark found comparing Bradley-Terry vs GP vs small nets on a single user's sound preferences at 20-200 judgments; the choice above is inferred from general small-data practice and the cited audio GP studies.
- No reported CPU timing for Koyama's library at these sizes was found (expected trivial, but unverified).
