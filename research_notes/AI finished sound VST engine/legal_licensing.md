# Legal and Licensing Landscape: Training/Analyzing AI on Commercial Music, Sample Packs and Synth Presets (US / EU / UK, as of 26 Oct 2026)

Not legal advice. These are research notes as of 2026-10-26. Many sources are secondary (law-firm alerts, trade press, trackers). Date conflicts between sources are flagged.

## 1. Lawsuits, settlements and fair-use rulings (US, Germany, UK)

### Takeaway
No US court has yet ruled on fair use for *audio* training. The label v. Suno/Udio cases have split: Warner settled with Suno, and UMG settled with Udio (both turned into licensing deals). UMG and Sony are still suing Suno, and Sony is still suing Udio, with dispositive motions not expected before 2027. In Germany, GEMA has won at first instance against both OpenAI (Nov 2025) and Suno (Jul 2026). Those rulings hold that memorisation of works in the model is reproduction outside the TDM exception. In the US, the book cases (Bartz, Kadrey) found training on lawfully acquired works fair use, but punished pirated acquisition and flagged "market dilution" as a live theory. That theory is especially dangerous for a generator of competing sounds.

### Cited Findings
**Labels v. Suno / Udio (US)**
- UMG settled with Udio in October 2025. The deal is described as a "walled garden" where created content cannot leave the platform — [Dubspot blog](https://blog.dubspot.com/suno-udio-ai-music-lawsuits-2026); [AI Lawsuit Tracker](https://ailawsuittracker.com/cases/umg-v-suno/)
- Warner settled with Suno in November 2025, allowing Suno to license and train on the WMG catalog (one source reports a ~$500M figure; I could not verify that figure against primary reporting) — [Wikipedia: Suno](https://en.wikipedia.org/wiki/Suno_(platform)); [happycapyguide](https://happycapyguide.com/blog/suno-umg-sony-licensing-stalemate-ai-music-copyright-2026)
- Warner–Udio: a settlement/licence followed in November 2025 per [Music Business Worldwide](https://www.musicbusinessworldwide.com/after-universal-warner-and-merlin-deals-now-udio-inks-licensing-agreement-with-kobalt/). Udio then also licensed with Merlin (Jan 2026) and Kobalt. One aggregator instead dates a Warner–Udio deal to April 2026 — conflicting; MBW is the more reliable source.
- UMG/Suno settlement talks reportedly hit an impasse in April 2026. The labels were reportedly seeking equity and higher per-stream guarantees — [Digital Music News, 9 Apr 2026](https://www.digitalmusicnews.com/2026/04/09/suno-universal-music-lawsuit-settlement-impasse/)
- UMG & Sony v. Suno (D. Mass.): in Aug 2026 the court allowed a DMCA §1201(a) anti-circumvention claim (stream-ripping from YouTube) but refused to add ~61,000 more recordings. The plaintiffs filed a second suit on 18 Sept 2026. Fact discovery runs to 30 Sept 2026 and dispositive motions are due ~April 2027, so no fair-use ruling is expected in 2026 — [AI Lawsuit Tracker](https://ailawsuittracker.com/cases/umg-v-suno/); [Chartlex tracker](https://www.chartlex.com/blog/business/music-industry-ai-lawsuits-tracker-2026) (trackers; verify against the docket)
- Sony v. Udio (S.D.N.Y.): the court refused in June 2026 to add ~30,000 recordings, and Sony filed a second suit on 20 July 2026 — [AI Lawsuit Tracker](https://ailawsuittracker.com/cases/umg-v-suno/) (tracker; verify)

**GEMA cases (Germany, Munich Regional Court I)**
- GEMA v. OpenAI (11 Nov 2025, 42 O 14139/24): the court found that memorised song lyrics in model weights are a reproduction *within the model*, and that memorisation falls outside the CDSM Directive TDM exceptions (Arts. 3/4). It granted injunction, disclosure and damages. The ruling is under appeal — [Taylor Wessing](https://www.taylorwessing.com/de/insights-and-events/insights/2025/11/gema-v-openai); [Bird & Bird](https://cm.twobirds.com/en/insights/2025/landmark-ruling-of-the-munich-regional-court-(gema-v-openai)-on-copyright-and-ai-training); [Medialaws](https://www.medialaws.eu/gema-v-openai-decision-of-the-munich-regional-court/)
- GEMA v. Suno (31 Jul 2026, 42 O 763/25): GEMA largely won, for six compositions. The court enjoined four acts: (1) reproduction for training in the US, (2) memorisation in the model on German servers, (3) communication to the public by offering the model, and (4) reproduction and communication via outputs. It applied US law to the US training and *rejected fair use*. It ordered revenue disclosure and damages. The ruling is not final, and Suno is considering an appeal; the extraterritorial reasoning is the likely appeal target — [Bird & Bird](https://www.twobirds.com/en/insights/2026/germany/munich-district-court-rules-on-ai-generated-music-gema-v-suno); [Bristows](https://inquisitiveminds.bristows.com/post/102ngsl/gema-v-suno-munich-court-finds-ai-music-training-and-outputs-infringe-copyright); [Variety](https://variety.com/2026/digital/news/suno-loses-ai-lawsuit-gema-1236825010/); [CMS tracker](https://cms.law/en/int/publication/artificial-intelligence-and-copyright-case-tracker/germany-gema-v.-suno-31-july-2026)
- The court reportedly found Suno obtained the works by stream-ripping from YouTube, circumventing the "Rolling Cipher" protection measure — [KPW](https://kpw.law/en/gema-also-wins-against-suno/)

**US fair-use rulings (books) and their application to audio**
- Bartz v. Anthropic (N.D. Cal., Alsup J., June 2025): training on lawfully acquired books was fair use, but keeping >7M pirated books in a "central library" was not excused. The case settled for $1.5B covering ~482,000 works, with final approval reported for 20 Jul 2026. The settlement covers past copying only, not outputs or future conduct — [Polsinelli / NatLawReview](https://natlawreview.com/article/ai-vs-authors-update-court-approves-historic-anthropic-settlement-while-meta); [Reuters via Yahoo](https://www.yahoo.com/news/articles/anthropic-tells-us-judge-pay-200101141.html). The approving judge's name differs between sources (the case was originally before Alsup); verify.
- Kadrey v. Meta (N.D. Cal., Chhabria J., 25 Jun 2025): summary judgment for Meta on training, but only because plaintiffs offered no meaningful market-dilution evidence. The judge wrote that "in many circumstances it will be illegal to copy copyright-protected works to train generative AI models without permission," and limited the ruling to "these plaintiffs." A distribution (torrent-seeding) claim continues — [Authors Alliance](https://www.authorsalliance.org/2025/06/26/meta-wins-on-fair-use-for-now-but-court-leaves-door-open-for-market-dilution/); [Ballard Spahr](https://www.ballardspahr.com/insights/alerts-and-articles/2025/07/from-input-to-impact-the-market-harm-standard-emerging-in-ai-fair-use)
- US Copyright Office, Part 3 "Generative AI Training" (pre-publication, 9 May 2025): fair use is fact-specific, and factor one depends on how the model is *used* (not just trained). Pirated or illegitimate sourcing weighs against fair use. Factor four includes "market dilution" by floods of works of the same kind, plus lost licensing markets. No final version has been issued, and its policy weight is uncertain after the Register was fired — [Sidley](https://datamatters.sidley.com/2025/05/28/generative-ai-meets-copyright-scrutiny-highlights-from-the-copyright-offices-part-iii-report/); [Authors Guild](https://authorsguild.org/news/us-copyright-office-ai-report-part-3-what-authors-should-know/); [Baker Botts](https://ourtake.bakerbotts.com/post/102kb85/u-s-copyright-office-issues-part-3-pre-publication-report-on-generative-ai-and-c)

**UK**
- Getty Images v. Stability AI [2025] EWHC 2863 (Ch), Nov 2025: Getty's secondary infringement claim failed because the model weights did not store or contain copies of the works, so they are not an "infringing copy" (though a model can be an "article"). Getty dropped its training and output claims, so those questions remain undecided in the UK. Leave to appeal was reportedly granted 17 Dec 2025 — [Bird & Bird](https://cm.twobirds.com/en/insights/2025/uk/stability-ai-defeats-getty-images-copyright-claims-in-first-of-its-kind-dispute-before-the-high-cour); [Mayer Brown](https://www.mayerbrown.com/insights/publications/2025/11/getty-images-v-stability-ai-what-the-high-courts-decision-means-for-rights-holders-and-ai-developers); [AIPPI](https://www.aippi.org/news/getty-images-v-stability-ai-uk-high-court-finds-no-secondary-copyright-infringement-in-landmark-genai-ruling/)

**EU (CJEU)**
- Like Company v. Google (C-250/25), the first CJEU AI-copyright reference: it asks whether LLM training is reproduction and whether chatbot outputs are communication to the public. The Grand Chamber hearing was held 10 Mar 2026. Google's counsel said AG Szpunar's opinion was scheduled for 3 Sept 2026 — [DDG](https://en.ddg.fr/actualite/first-ai-related-reference-to-the-cjeu-preliminary-questions-in-case-c-250-25-like-company-v-google-ireland); [Hengeler Mueller](https://hengeler-news.com/en/articles/hengeler-mueller-represents-google-in-landmark-aicopyright-case-before-cjeu). One outlet reports the case "facing delays" — [Electron Libre](https://electronlibre.info/articles/107346-ai-and-copyright-the-like-company-v-google-case-is-facing-delays/)

### Inferences
- For a commercial generator of EDM sounds, the Kadrey/USCO "market dilution" theory is the biggest US risk. A plugin that outputs sounds substituting for sample packs and presets competes directly in the training-data owners' market, which is the strongest case for plaintiffs. "Transformative" arguments from the book cases transfer poorly.
- Data provenance matters independently of the fair-use outcome. Pirated or stream-ripped sources drove the Bartz liability, the DMCA §1201 claim against Suno, and the GEMA findings. Licence-violating acquisition (e.g. using Splice content against its ToS) creates contract claims even if copyright claims fail.
- In the EU, the GEMA line means that if a model can regurgitate recognisable portions of training works, the TDM exception does not protect it. Low-memorisation designs, de-duplication and output filtering are mitigations, though not safe harbours. The German court's willingness to apply US law extraterritorially means training location offers little protection when you sell into Germany.
- The trend in industry is licensing (UMG/WMG with Udio and Suno, ElevenLabs with Merlin, Kobalt and UMG), which strengthens the "licensing market exists" argument under factor four against unlicensed trainers.

### Gaps
- I could not confirm whether AG Szpunar's opinion in C-250/25 was actually delivered in Sept 2026, or what it says.
- I could not verify the exact Warner–Suno settlement amount from a primary source; the D. Mass./S.D.N.Y. procedural details come from trackers, not the dockets.
- Appeal status of GEMA v. OpenAI (Higher Regional Court Munich) and of Getty v. Stability was not confirmed as of Oct 2026.
- No court anywhere has ruled specifically on training on sample packs or presets.

## 2. EU AI Act GPAI obligations, EU TDM opt-out, UK TDM outcome

### Takeaway
EU: since 2 Aug 2025, GPAI model providers must have a copyright policy that honours machine-readable TDM opt-outs (CDSM Art. 4(3)). They must also publish a training-data summary using the Commission's mandatory template. Pre-existing models have until 2 Aug 2027. Commission enforcement powers reportedly begin Aug 2026 (unverified). A narrow, task-specific audio model in a plugin may not be a "GPAI model", but the underlying CDSM Art. 4 opt-out applies to *any* commercial TDM in the EU regardless. UK: the government's 18 Mar 2026 report abandoned the proposed commercial TDM exception with opt-out. Commercial training in the UK still requires licences (s.29A CDPA covers non-commercial research only), and there is no legislative timetable.

### Cited Findings
- GPAI transparency and copyright obligations have applied from 2 Aug 2025. Models placed on the market before then have until 2 Aug 2027 — [EU AI Act Newsletter #83](https://artificialintelligenceact.substack.com/p/the-eu-ai-act-newsletter-83-gpai); [Medialaws](https://www.medialaws.eu/eu-ai-obligations-for-gpai-providers-compliance-enforcement-deadlines-2025-2027/)
- The Commission issued a mandatory template for the public training-data summary (sources, curation, etc.), intended to help rightsholders exercise their rights. The copyright obligations include respecting CDSM TDM opt-outs — [CASRAI](https://casrai.org/news/eu-ai-act-gpai-obligations-code-of-practice-august-2025)
- The GPAI Code of Practice is voluntary and includes a copyright chapter. Signatories include Anthropic, Amazon, Google, Microsoft and OpenAI — [CASRAI](https://casrai.org/news/eu-ai-act-gpai-obligations-code-of-practice-august-2025)
- CDSM Directive Art. 3 covers research organisations, and Art. 4 permits commercial TDM unless rightsholders opt out in machine-readable form (e.g. robots.txt) — [arXiv: Generative AI Training and Copyright Law](https://arxiv.org/pdf/2502.15858)
- Per GEMA v. OpenAI, the TDM exceptions may cover training but not memorisation of works — [Taylor Wessing](https://www.taylorwessing.com/de/insights-and-events/insights/2025/11/gema-v-openai)
- UK: the statutory report (18 Mar 2026, under the Data (Use and Access) Act 2025) dropped the opt-out exception, endorsed no alternative, and set no timetable. It proposes removing s.9(3) CDPA computer-generated-works protection, and further work on transparency and licensing is expected — [Reed Smith](https://www.reedsmith.com/articles/uk-copyright-and-ai-report-the-opt-out-is-dead-but-what-comes-next/); [Osborne Clarke](https://www.osborneclarke.com/insights/opted-out-uk-government-moves-away-preferred-position-ai-and-copyright-following-report); [Birketts](https://www.birketts.co.uk/legal-update/copyright-and-artificial-intelligence-uk-government-report-march-2026/); [Two Birds](https://www.twobirds.com/en/insights/2026/uk/copyright-,-a-,-aiin-the-uk-the-debate-rolls-on)
- UK s.29A CDPA (2014) permits TDM copies only for non-commercial research — [Bratby Law](https://bratby.law/copyright-and-ai-training-exception-2026/); [Cranfield library](https://library.cranfield.ac.uk/text-and-data-mining/copyright)

### Inferences
- Major labels, publishers, Splice, NI and Loopmasters have all reserved rights or prohibited AI use. A commercial EU training pipeline therefore cannot rely on Art. 4 for their content: contractual prohibitions in ToS/EULAs plus express reservations likely count as an opt-out.
- A specialised EDM sound model probably falls outside the "general-purpose" definition. Even so, voluntarily publishing a training-data summary is a cheap trust signal for EU sales and helps with distribution partners.
- UK: assume a licence is needed for any commercial training on UK-protected works.

### Gaps
- The exact start date of Commission enforcement (fines) for GPAI providers was not confirmed from official sources; it is reportedly 2 Aug 2026.
- I did not verify whether the Digital Omnibus proposals (late 2025) changed the GPAI timelines.
- I found no authoritative guidance on whether a narrow audio model embedded in a plugin is a GPAI model.

## 3. Sample pack and preset licence terms

### Takeaway
Explicit AI-training prohibitions now appear in the Splice ToS, the Loopmasters/Loopcloud licence and the Native Instruments EULA and subscription terms. I found no explicit AI clause for Cymatics or for Xfer/Serum preset packs, but standard "use only in new musical works / no redistribution / all rights reserved" language would still likely exclude dataset use. Treat all commercial packs as off-limits for training unless you get a written licence.

### Cited Findings
- Splice: "use of content downloaded from Splice for the purposes of training/modeling data for AI is not permitted by our Terms of Use". Use is limited to "New Recordings and Creative Works", neither of which includes AI training data — [Splice Help Center](https://intercom.help/splice-b7a2d5b83f2e/en/articles/8652642). The ToS was reportedly last updated 24 Jul 2026 — [agentman.ai](https://agentman.ai/agentskills/connections/mcp-server/splice) (secondary)
- Loopmasters licence: bars use of the products to train or develop AI-generated content, machine learning models, generative AI, NLP models "or any other AI technologies that may learn from or replicate the products". It may not apply to some third-party labels it represents — [Loopmasters License Agreement](https://help.loopmasters.com/hc/en-us/articles/27012699080348-Loopmasters-License-Agreement); Loopcloud terms incorporate it — [Loopcloud ToS](https://www.loopcloud.com/cloud/terms_of_service)
- Native Instruments EULA: licensees may not reproduce or use the product or its audio and visual content "for data mining or for training artificial intelligence technologies". The subscription terms also prohibit use "for purposes of training artificial intelligence" — [NI EULA](https://www.native-instruments.com/pages/end-user-license-agreement); [NI subscription terms](https://www.native-instruments.com/en/company/legal-information/terms-of-service/)
- Loopmasters/Big Fish Serum preset pack licences: personal, non-transferable use, a ban on use in competing products, content remains the licensor's property, all rights not expressly granted are reserved, and the licence is revocable on breach. These excerpts contain no explicit AI clause — [Big Fish Audio: Revaux DnB Serum Presets](https://www.bigfishaudio.com/Revaux-DnB-Serum-Presets); [Berlin Techno Serum Presets](https://www.bigfishaudio.com/Berlin-Techno-Serum-Presets)
- Cymatics: the excerpts found cover a royalty-free, single-user, non-transferable licence with a redistribution ban. I found no AI clause, and these were not official Cymatics pages — [Avvo Q&A](https://www.avvo.com/legal-answers/i-bought-some-100-royalty-free-loops-from-cymatics-4747322.html); [Freesound forum](https://freesound.org/forum/legal-help-and-attribution-questions/45171/)
- Splice partnered with ElevenLabs (reported May 2025), which suggests Splice licenses content for AI separately rather than allowing it under the standard ToS — [invideo blog](https://invideo.io/blog/elevenlabs-music-ai-generator/) (secondary; terms unknown)

### Inferences
- The "no use in competing products" and "no redistribution" clauses typical of preset and sample licences bite especially hard on a commercial sound-generating plugin. Even analysis-only use that feeds a product competing with the packs is arguably outside the licence grant.
- Contract claims do not depend on copyrightability. So even if an individual one-shot or preset is not protected, using it against the EULA risks breach of contract and licence termination.

### Gaps
- I did not retrieve the official current Cymatics licence or Xfer Records' own Serum EULA or preset terms; you need to check them directly.
- The Splice–ElevenLabs deal terms are unknown.

## 4. Statistical descriptor extraction vs. generative training; sound-alikes; copyrightability of sounds and timbres

### Takeaway
Extracting aggregate descriptors (LUFS, spectral tilt, crest factor, stereo width, estimated effect parameters) is legally much lower-risk than training a generative model on the audio. The outputs are unprotectable facts or ideas and do not recapture the recorded sounds. The legal exposure sits mainly in the *temporary copying* of the audio, which can be justified as non-expressive fair use (US) or TDM (EU Art. 4, subject to opt-out) but falls into a UK gap for commercial use. It also sits in licence/contract breaches. US §114(b) expressly permits sound-alikes: sound recording rights cover only the actual fixed sounds, not imitations. I found no case holding a synth patch or timbre copyrightable.

### Cited Findings
- 17 U.S.C. §114(b): sound recording rights are limited to duplicating "the actual sounds fixed in the recording". The legislative history says imitation is not infringement "even where one performer deliberately sets out to simulate another's performance as exactly as possible" — [17 U.S.C. §114 (Justia)](https://law.justia.com/codes/us/1994/title17/chap1/sec114/); [CounselStack](https://law.counselstack.com/usc/17/114)
- Synth presets: I found no court decision. Practitioner and forum views split between "a list of settings, so not copyrightable" and "a program, possibly protectable" — [KVR forum](https://www.kvraudio.com/forum/viewtopic.php?p=6715309) (non-authoritative)
- Litigants have argued that synthesized sounds and timbres are genre building blocks and not copyrightable; this is an argument, not a holding — [Loyola IP Bytes](https://blogs.luc.edu/ipbytes/wp-json/wp/v2/posts/3232)
- The US "non-expressive use" doctrine (HathiTrust, Google Books) supports copying for computational analysis that does not communicate expression — [Sag, "Copyright is the lock, non-expressive fair use is the key"](https://experts.illinois.edu/en/publications/copyright-is-the-lock-non-expressive-fair-use-is-the-key-research/)
- The USCO Part 3 report says fair use depends on the model's downstream use and on market effects, so analysis tools that do not output substitutes fare better — [Sidley](https://datamatters.sidley.com/2025/05/28/generative-ai-meets-copyright-scrutiny-highlights-from-the-copyright-offices-part-iii-report/)
- UK Getty v. Stability: a model that does not store copies is not an "infringing copy"; this suggests descriptor sets and small parameter models are not infringing articles — [Bird & Bird](https://cm.twobirds.com/en/insights/2025/uk/stability-ai-defeats-getty-images-copyright-claims-in-first-of-its-kind-dispute-before-the-high-cour)
- Germany (GEMA): liability turned on memorisation and reproducible outputs, which aggregate descriptors do not create — [Bristows](https://inquisitiveminds.bristows.com/post/102ngsl/gema-v-suno-munich-court-finds-ai-music-training-and-outputs-infringe-copyright)

### Inferences
- A spectrum of risk, lowest to highest:
  1. Measuring loudness or spectra from tracks the user owns, done locally in the plugin at the user's direction (the user's own act; the developer ships no data).
  2. Developer-side aggregate statistics (genre-level mix and master target curves) computed from lawfully purchased or streamed audio and stored only as numbers.
  3. Training a discriminative or parametric model (e.g. audio to effect parameters) on copyrighted audio.
  4. Training a generative audio model on copyrighted audio or sample packs: highest risk, especially with output similarity.
- Even tiers 2–3 require transient copies. In the UK there is no commercial TDM exception, and the US relies on fair use. Keep tracks transient, avoid stream-ripping (§1201 risk, as in the Suno claim), and document that no audio is retained.
- Generated outputs: the risks are near-verbatim reproduction of a sample (sound recording infringement) or of a melody/riff (composition infringement). Sound-alike timbres are generally safe under §114(b). Avoid artist-name prompting ("sounds like [artist]"), which raises right-of-publicity, passing-off and trademark issues, and may trigger state voice laws such as Tennessee's ELVIS Act (not verified in this research).
- Preset files may carry some protection as a creative selection of parameters or as software data, and licences add contract protection. So do not ship or train on third-party preset parameter dumps, even if the sounds themselves are unprotectable.

### Gaps
- I found no case law specifically on audio feature extraction or music information retrieval under fair use, or on synth-preset copyrightability.
- I did not research EU/UK sound-alike doctrine (EU related rights also protect only the fixation; there is no direct citation here), or US state voice and likeness laws (ELVIS Act and others).

## 5. Ethical and licensed data options

### Takeaway
Viable safe paths are: (a) Fairly Trained "Licensed Model" certification, (b) licensed catalogues and datasets (labels, Merlin, Kobalt, production-music libraries, the AudioSparx model used by Stable Audio, SourceAudio), (c) commissioned sound-design work-for-hire with explicit AI-training grants, (d) data the developer renders itself from open-source or owned synths with self-designed patches, and (e) opt-in contributor programs with royalties. The market trend is licensing.

### Cited Findings
- Fairly Trained (launched Jan 2024, CEO Ed Newton-Rex) certifies a "Licensed Model" status, which excludes reliance on fair use. Music companies certified at launch include Beatoven.AI, Boomy, Endel, LifeScore, Somms.ai (now Musical AI), Soundful, Tuney and Rightsify. Infinite Album was certified later, and Kits.ai and Voicemod were certified for voice — [Fairly Trained](https://fairlytrained.org/blog/fairly-trained-launches-certification-for-generative-ai-models-that-respect-creators-rights); [MBW](https://musicbusinessworldwide.com/ed-newton-rex-launches-certification-for-ai-models-respect-copyrights); [Infinite Album](https://www.infinitealbum.io/news/b2k91fetcj0dlzp3irut10aak655r2-wcbnk)
- Fairly Trained tightened its standards: badges must now specify which parts of the architecture are certified. Jen and Kits AI were the first to receive them — [MBW](https://www.musicbusinessworldwide.com/music-ai-platforms-can-get-a-fairly-trained-certification-for-meeting-ethical-standards-those-standards-just-got-tougher/)
- ElevenLabs Music (Aug 2025) is licensed via Merlin and Kobalt, with members opting in to training for royalties. ElevenLabs partnered with Splice, and with UMG on a multi-year licence announced 10 Sept 2026 — [weraveyou](https://weraveyou.com/2026/09/universal-music-elevenlabs-ai-music-platform-licensing-deal/); [TechTimes](https://www.techtimes.com/articles/327314/20260911/elevenlabs-signs-first-lawsuit-free-ai-music-deal-umg-fans-get-remix-platform.htm)
- Udio has licences with UMG, Warner, Merlin and Kobalt — [MBW](https://www.musicbusinessworldwide.com/after-universal-warner-and-merlin-deals-now-udio-inks-licensing-agreement-with-kobalt/)
- Stable Audio is trained on licensed AudioSparx data. SourceAudio offers a pre-cleared dataset — [invideo blog](https://invideo.io/blog/elevenlabs-music-ai-generator/) (secondary)
- Opt-in uptake can be low: a report claims that of 100+ labels offered an ElevenLabs AI licence, 3 said yes — [aimusicpreneur](https://www.aimusicpreneur.com/ai-music-industry/secretly-elevenlabs-ai-licence-3-labels-7-artists-opted-in/) (low-reliability blog)

### Inferences
- The best fit for a preset and sound-generating VST is to render your own data: an open-source synth (e.g. Vital, Surge XT, Dexed) driven by procedurally or randomly generated and in-house-designed patches, rendered through your own effect chains. This yields unlimited, perfectly labelled (parameter to audio) data with no third-party rights. Check the synth's licence: under GPL, *rendered audio output* is generally not encumbered, but bundled factory presets and wavetables may carry their own terms.
- Commission sound designers under written agreements that assign rights or grant explicit ML-training and derivative-generation rights, with moral-rights waivers (relevant in the EU/UK).
- Approach Splice, Loopmasters or ADSR for an *AI-specific* licence. Splice's ElevenLabs deal shows such licences exist.
- For "pro mix/master style" profiling, use licensed stems or masters, user-supplied reference tracks analysed locally (the user's act), or published loudness norms rather than a scraped corpus of commercial releases.
- Seek Fairly Trained certification, and publish an EU-AI-Act-style training-data summary as marketing and trust signals.

### Gaps
- I did not retrieve a current (2026) list of Fairly Trained certified companies, or its fees and process.
- I did not verify licence terms, including output and derivative rights, for specific open-source synths' factory content.
- I did not find pricing or terms for licensed audio datasets (AudioSparx, SourceAudio, Splice AI licensing).
