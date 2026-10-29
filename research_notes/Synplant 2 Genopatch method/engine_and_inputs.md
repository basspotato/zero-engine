# Synplant 2: synth engine and how Genopatch handles input audio

Scope: Synplant 2 (Oct 2023, through 2.0.x). Note on sources: the official User Guide is only served as a PDF download (https://soniccharge.com/public/Synplant%20User%20Guide.pdf), and the browser could not open it without a file download, which needs the owner's permission. Facts from the guide below come from search-engine excerpts of that PDF (marked "guide excerpt"), not a full read. Reading the PDF in full is the obvious next step and would fill most of the gaps listed.

## What is Synplant 2's sound engine and how are its parameters exposed as genes?

### Takeaway
A small, deliberately mutation-friendly engine: two oscillators with FM (often described as "2-operator FM") that morph between waveforms, two SVF filters with a morphable mix, envelopes and an LFO, per-voice asymmetric saturation, reverb (with its own attack envelope), a 2-band output EQ, a soft clipper and panning. Every parameter is a "gene" on a DNA strand, grouped into three sections: envelope/LFO, oscillators, filter/effects. V2 kept v1's design for backward compatibility but rewrote most of the DSP.

### Cited Findings
- "Essentially a twin-oscillator synth with FM and a flexible system for envelopes and modulation, a rather nice filter section, plus reverb, saturation, EQ and panning effects"; core engine updated in v2 to improve the sound — [Sound On Sound, Sept 2024](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- Described as "2-operator FM" with machine learning turning samples into programs — [Synth and Software review](https://synthandsoftware.com/2023/11/sonic-charge-synplant-2-the-synth-and-software-review/); MusicTech also calls it a two-oscillator and FM synth — [MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/)
- Magnus Lidström (developer): v2 aimed to be fully backward compatible, so the fundamental design was kept similar to v1, but most of the DSP was overhauled. New in v2: a morphable filter type that mixes the outputs of two SVF filters in different ways, "much like how the oscillators morph between different waveforms"; per-voice saturation with asymmetry (odd harmonics); a simple two-band EQ on the output; an adjustable soft clipper (distortion before and after mixdown); a reverb with an attack envelope that fades the reverb in on each note-down — [CDM interview, Oct 2023](https://cdm.link/the-road-to-synplant-2/)
- DNA Editor: each gene (horizontal line in the strand) is a synth parameter, organised top to bottom into three sections: envelope and LFO, oscillators, filter and effects. New in v2: selecting a section shows conventional rotary controls for it, with a text description on hover — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- The DNA editor shows graphical representations for envelopes, oscillator types and filters — [Sonic Charge product page](https://soniccharge.com/synplant)
- Genes can be controlled by MIDI CC; new "bulb modes" (velocity, keyboard range, layering) — [rekkerd.org](https://rekkerd.org/sonic-charge-releases-synplant-2-with-genopatch-technology/)
- A gene named `a_freq` exists; the guide suggests editing it in the DNA Editor for a broader pitch range (guide excerpt via search) — [Synplant User Guide 2.0.2](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=)
- Patch structure: a seed at the centre of the Bulb with 12 branches, one per note of the octave; dragging a branch outward mutates it further from the seed; the mod wheel grows all branches together; Alt-click on the seed clones the selected branch to all branches. One preset can hold 12 different sounds — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2); [Synth and Software](https://synthandsoftware.com/2023/11/sonic-charge-synplant-2-the-synth-and-software-review/)
- Atonality slider: at 0 a seed typically produces a melodic, playable sound; higher settings push toward unpitched effects (guide excerpt) — [User Guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=); also [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- Poly and mono modes with portamento, tempo sync, a layer mode; many layers reduce polyphony (max set in MIDI Settings) — [product page](https://soniccharge.com/synplant); guide excerpt via search
- Lidström: "The synth engine of Synplant is designed to produce interesting results when you mutate its parameters" — [CDM](https://cdm.link/the-road-to-synplant-2/)

### Inferences
- The engine is intentionally small (two oscillators, one filter block, a few effects). A small parameter space is what makes inverse mapping (sound to parameters) learnable and keeps random/mutated settings musical. That matters more than raw engine power.
- Because each of 12 branches holds a full parameter set, Genopatch results can differ per key unless cloned across branches.

### Gaps
- Exact gene count, the full gene list and ranges (e.g. how many envelopes, LFO destinations, waveform set, filter types) were not found; they are in the User Guide PDF, which could not be opened without a download.
- Whether there is noise as an oscillator source or a dedicated noise gene: not confirmed.

## How does Genopatch work, and what does it do with pitch, length, region, mono/poly, loops?

### Takeaway
Genopatch takes up to about two seconds of a loaded WAV/AIFF (chosen with a position slider), converts it to mono, and runs a neural net trained on the synth's own random output to guess parameters, then keeps refining: four strands grow, each node a better match. By default the patches are tuned to the source's pitch. It aims to reproduce the whole selected sound, so a chord comes back as a chord on every key; it does not transcribe notes or loops.

### Cited Findings
- Load a sample, select up to two seconds of it, press start; four strands sprout as it searches for settings — [MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/); "based upon up to two seconds of that sample", four strands each offering multiple versions; each match is a coloured dot to audition, save, or plant as a new seed — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- With long samples only a short segment (a second or two) is used; a slider moves through the file to pick the section — (review excerpt in search results; [Synth and Software](https://synthandsoftware.com/2023/11/sonic-charge-synplant-2-the-synth-and-software-review/) / [Synthtopia](https://www.synthtopia.com/content/2023/10/04/synplant-2-lets-you-create-synth-patches-from-samples-using-artificial-intelligence/), exact source of that sentence not confirmed)
- WAV and AIFF supported; files longer than 10 minutes are truncated (guide excerpt) — [User Guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=)
- Genopatch converts sound to mono before analysing (listed as a con) — [MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/)
- No sampling: the patch is pure synthesis and gets closer to the source over time, not always exactly — [MusicTech](https://musictech.com/reviews/plug-ins/sonic-charges-synplant-2-review/); "Sometimes, you will get a perfect match; other times, something new and unexpected"; runs fully locally — [product page](https://soniccharge.com/synplant)
- Pitch: unless an option is turned off, Genopatch tunes patches to the source sample's pitch; patches can still drift, and lower Atonality helps keep tuning, especially on grown-out branches (guide excerpt) — [User Guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=)
- Tuning slider adjusts pitch within an octave; automatic correction via right-click on the slider or "Correct Tuning" in the main menu; may fail with complex sounds (guide excerpt) — [User Guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=)
- Method (Lidström): a combination of machine-learning algorithms tuned over 10 years, the last five mainly neural nets; trained on semi-random sounds generated by the synth itself "so that it learns how to go backward from output sound to input parameters"; months of training for a small net that runs on the local CPU. "Confidence" parameters kept deliberately low, so it starts with wild guesses — [CDM](https://cdm.link/the-road-to-synplant-2/)
- Heavy CPU use during the search — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- Chords: a user found a chord sample gives a chord on every single key; the reply: Genopatch replicates the sound you give it, so isolate the source to one note to get one-note solutions — [Sonic Charge forum: chord to individual notes](https://soniccharge.com/forum/topic/2961-from-audio-sample-of-a-chord-to-individual-notes)
- Struggles with foley, composited effects and polyphonic instrument sounds (rave chords, orchestra hits) — [Sonic Charge forum user, 2.0.1](https://soniccharge.com/forum/topic/2680-synplant-2-0-1-resynthesizing-complex-sound-effects)
- Best on short sounds: plucks, short bass tones, kicks, snares, hi-hats, claps. Unrealistic to expect a two-second single note (e.g. guitar) to give a playable patch across the keyboard with the original's dynamics — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- Non-synthetic sounds are less accurate because it is limited to its own engine — [MusicRadar](https://www.musicradar.com/reviews/sonic-charge-synplant-2-review)

### Inferences
- Genopatch treats the selection as one sound event: no note transcription, no loop/rhythm handling, no polyphonic separation. A loop or phrase would be matched as a single (probably messy) sound. Zero Engine's Reference Match, which transcribes notes and replays them, goes further here.
- Pitch is detected (it can tune to the source), then the search works around that pitch; the "Correct Tuning" fallback suggests pitch is estimated from the result too, and can fail on inharmonic sounds.
- Drums and noise work reasonably because the engine plus Atonality can make unpitched sounds and the 2 s window covers a whole hit.
- The two-second cap and mono summing keep the fingerprint small and the search fast; stereo width is not matched.

### Gaps
- How the root note is reported or set by the user (manual override of detected pitch), and the name of the option that disables auto-tuning: not found.
- Whether the user can set the region start and length freely or only the start of a fixed window: partly unclear.
- No source on how sustained vs decaying sounds are treated differently, or how the release/tail past 2 s is inferred.
- No source on vocals specifically.

## What controls does the user have over the Genopatch process?

### Takeaway
Start/stop the search, watch four strands grow, click any node to audition, save it, or plant it as a seed and then grow branches/mutate with the usual Synplant tools or edit genes. No explicit locking or text guidance was found for Genopatch itself.

### Cited Findings
- Four strands; each new iteration adds a node that lets you jump between patches; save the selected match or "Plant Seed" to continue evolving — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2); [MusicRadar news](https://www.musicradar.com/news/sonic-charge-synplant2-ai)
- After planting, results can be tweaked in the DNA Editor — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)
- Auto-tuning to source pitch can be switched off (guide excerpt) — [User Guide](https://soniccharge.com/public/Synplant%20User%20Guide.pdf?dl=20240830T095912&inline=)
- A user asked for per-branch gene editing (a feature request, suggesting it is not there) — [Sonic Charge forum](https://cdn.soniccharge.com/forum/topic/2490-feature-request-per-branch-gene-editing?~post12852)
- A user wished for control over tonality amount, the fundamental pitch and the main 2-3 harmonics to guide the search (wish, not a feature) — [Sonic Charge forum](https://soniccharge.com/forum/topic/2680-synplant-2-0-1-resynthesizing-complex-sound-effects)
- Synplant has a script system (puzzle-piece icon; Skin Chooser is a script) — [CDM](https://cdm.link/the-road-to-synplant-2/)
- Sound On Sound suggested a future feature: several samples at different pitches/velocities interpolated — [Sound On Sound](https://www.soundonsound.com/reviews/sonic-charge-synplant-2)

### Inferences
- The UX trades control for discovery: the user's lever is choosing the 2 s region and then picking among many candidates, not steering the search.

### Gaps
- Gene locking during Genopatch, re-seeding a run, or limiting it to some genes: no source found. Check the User Guide PDF.
- Any changes in updates after 2.0.2 (2025-2026): none found.
