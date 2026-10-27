# Reference sounds: licence-safe sources (2026-10-26)

Goal: about 15 finished sounds per genre x type pair (about 1,100 total) to measure with `ZeroAnalyze` and set profile targets. The owner's own sounds come first; the sources below fill gaps.

## Use

- **Freesound, CC0 only.** Freesound's own guidance: CC0 sounds have no copyright-related restrictions for training predictive or generative models. CC-BY is usable only if attribution ships with anything distributed, so skip it for now. [Freesound blog](https://blog.freesound.org/?p=2082)
  - Full-quality downloads need OAuth2 (a personal Freesound login), not just an API key. [APIv2 overview](https://freesound.org/docs/api/overview.html)
  - API terms: the licence is revocable, no full copies of the database, and rate limits are set per application. [API terms](https://freesound.org/help/tos_api) A community report puts full-quality downloads at about 500/day.
  - Quality varies a lot, so measure and audition before anything counts as a reference.
- **Individual CC0 packs**, only after reading the pack's own licence file (some indie designers release CC0; one aggregator claims Producer Space is CC0, but that's unverified).

## Don't use

- Splice, Loopmasters, Native Instruments: licences forbid AI training (see `CLAUDE.md`).
- Goldbaby free packs: licence forbids any AI or machine-learning use.
- Cymatics and similar "royalty-free" packs: terms cover releasing music, not analysis or training. Treat them as no until a licence says otherwise.

## Poor fit

- **NSynth** (CC BY 4.0, 305,979 notes): notes from instrument sample libraries, 16 kHz audio, not finished EDM sounds. Brightness above 8 kHz can't be measured. Possibly useful later for training, not as finished-sound references.

## Expectation

Freesound CC0 should cover the common pairs (dubstep and DnB basses, 808s, supersaw leads and chords, pads) with tens to a few hundred candidates each, and very few for rare pairs (for example Hardstyle Pluck). It won't reach 1,100 release-ready sounds alone; the owner's set fills the priority pairs.
