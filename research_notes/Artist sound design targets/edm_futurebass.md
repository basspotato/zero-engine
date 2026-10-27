# EDM / Future Bass / UK Dance: Sound Design Targets (Bass, Lead, Chord, Pad, Pluck, FX)

Scope: Marshmello, Whethan, Fred again.., Martin Garrix, Illenium, San Holo, Flume, Odesza, Skrillex-pop crossovers, Swedish House Mafia, Calvin Harris. Focus on parameters a DSP engine can be tuned to. Priority gap: EDM Bass.

Research method note: about 16 web searches. The search tool returned summaries, not full pages, and full-page fetching was not available. Artist-specific patch numbers are rare in public sources. Most numbers below come from generic genre tutorials. Anything not tied to a citation is under Inferences and marked as an estimate. Several sources are commercial (preset packs, courses) and are flagged as such.

## 1. Oscillator / wavetable source per type

### Takeaway
Future bass and big-room sounds are built on stacked, detuned saws. The documented starting point is two saw oscillators, an octave apart, each with 8-16 unison voices and low-to-moderate detune. UK garage/house bass uses a mono sine/triangle sub under an organ, square or reese mid layer. Fred again..'s signature basses are often processed recordings (the "Jungle" bass is a recorded faulty cable), not synth patches.

### Cited Findings
- Future bass drop (Serum): two 16-voice unison detuned saw oscillators an octave apart, plus a short pitch envelope that bends the pitch up at note start "for character". — [Loopmasters/Loopcloud: How To Make A Future Bass Drop](https://www.loopmasters.com/articles/4372-How-To-Make-A-Future-Bass-Drop-A-Future-Bass-Production-Tutorial)
- A Reason/Thor future bass chord patch is "fairly simple": detuned saw plus square waves. — [Pyramind: future bass chords Reason 9](https://pyramind.com/future-bass-chords-reason-9/)
- Community Serum notes (unverified, Scribd): future bass saw chord = 2 saws, one an octave up, 8 voices each with slight detune; a pad recipe with 16 unison and detune 0.04. — [Scribd: Sound Design Notes](https://www.scribd.com/document/1029074669/Sound-Design-Notes)
- Future Music chord patch: unison 2 voices, detune pulled back to 0.04, unison stereo width at 0 "for a more analogue sound" (2017, older practice). — [Future Music via PressReader](https://www.pressreader.com/australia/future-music-9629/20170727/282291025290883)
- Chord stabs: detune kept "relatively low"; other tutorials push unison up to 16 voices and raise detune for width. — [PML: chord stab in Serum](https://www.productionmusiclive.com/blogs/news/1-2-5-how-to-make-a-chord-stab-in-serum-sound-design-tutorial); [Soundbridge: bigger synths using unison](https://www.soundbridge.io/make-bigger-synths-using-unison)
- Big-room / electro chord supersaw (Afrojack "Take Over Control", fan analysis, older classic around 2010): Sylenth1 "classic supersaw, voices maxed out on all 4 osc's" (Sylenth1 maximum is 8 voices per oscillator). — [Image-Line forum](https://forum.image-line.com/viewtopic.php?p=1949228)
- Flume ("Tennis Court" remix lead, 2013, older classic): Sylenth1 detuned saw plus pulse wave through 24 dB lowpass filters, with an LFO and a mod envelope on the cutoff. Sylenth1 is cited as his go-to synth in interviews. — [PML: Flume Synth with Sylenth1](https://www.productionmusiclive.com/blogs/news/flume-synth-with-sylenth1-designing-the-sound-from-lorde-tennis-court-flume-remix)
- Flume-style lead layered as six parts, including a sub, a monophonic top lead and a polyphonic main lead. — [PML: Flume sound Massive](https://www.productionmusiclive.com/blogs/news/53960517-flume-sound-massive-synth-designing-the-sound-from-lorde-tennis-court-flume-remix)
- Odesza "Line of Sight" (2017): "a lot of saw and square-based synths"; no "crazy sound design", the focus is on cohesive sounds. — [Splice: Hits decoded, Line of Sight](https://splice.com/blog/hits-decoded-odesza-line-of-sight/)
- UK garage reese: detuned oscillators over a stable sub. The sub is a triangle on 1 unison voice, an octave down, stereo width 0, with slight filter drive. — [The Producer School: UKG Reese in Serum 2](https://theproducerschool.com/blogs/featured-blogs/master-uk-garage-reese-bass-synthesis-in-serum-2-complete-sound-design-tutorial)
- UKG bass palette (commercial pack descriptions): square bass, organ bass, wubs, subs, 808, acid; "classic organ basses to deep, gritty Reeses and plucky subs" as the 90s UKG low end. — [Loopmasters product 12026](https://www.loopmasters.com/products/12026); [Loopmasters product 19653](https://www.loopmasters.com/products/19653)
- Fred again.. "Jungle" (2022) bass: an iPhone recording of a "dodgy cable", chopped, processed and repitched in Logic. — [MusicRadar](https://musicradar.com/news/fred-again-jungle-bass-sound-dodgy-audio-cable)
- Fred again.. works mainly in the box (laptop, Logic). — [MusicRadar: "If you're bored of laptop..."](https://www.musicradar.com/news/laptop-fred-again-howa)
- Skrillex/Fred again../Flowdan "Rumble" (2023): reviewers describe a "lean, mean and minimal" sub-bass riff, compared with The Bug's "Skeng". No technical breakdown was found. — [The Fader](https://thefader.com/2023/01/04/skrillex-rumble-feat-fred-again-flowdan); [MusicRadar](https://musicradar.com/news/skrillex-fred-again-rumble)
- Calvin Harris: TB-303 acid bass on "Over Now" (2020); Prophet 08, Juno-106, MS-20 and ARP Odyssey in his gear. — [EDM.com: Calvin Harris deconstructs song with The Weeknd](https://edm.com/features/watch-calvin-harris-deconstruct-his-song-with-the-weeknd/); [Equipboard](https://equipboard.com/calvin-harris)
- Pluck source: a saw or bright wavetable; a square gives a "hollower, more retro" pluck. The Faithless "Insomnia" house pluck (1995 classic) uses squares. — [Violet Recording: pluck](https://violetrecording.com/how-to-make-a-pluck-sound/); [Audiotool wiki: Faithless pluck](https://audiotool.fandom.com/wiki/Famous_sound_tutorial/Faithless%27_pluck)
- Flume-style chords and plucks are also made by heavily saturating vocal recordings (Saturn), then adding reverb and chorus. — [Warp Academy: saturation](https://warpacademy.com/?p=89809) (via MusicTech/search summary)

### Inferences
- Engine target, future bass chord: 2 saw oscillators (0 and +12 semitones), 7-9 voices each, detune of about 15-25 cents spread (estimate; Serum 0.04 on the 0-1 scale is subtle, a "big" future bass stack is usually 0.15-0.30). Add an optional square layer at -12.
- Engine target, festival/big-room supersaw lead: 2-3 saw layers × 7-16 voices, wider detune (estimate 25-40 cents), plus an octave-up layer. Sylenth1 caps at 8 voices per oscillator, so classic big-room is 4 × 8.
- EDM bass sub-types for the priority gap, all estimates:
  - **Future bass bass (Illenium, San Holo, Marshmello):** a sine sub at 1 voice plus a saw/reese mid layer that copies the chord root, often resampled. Mid layer: 2-4 saws, detune 10-20 cents, low-passed around 1-3 kHz.
  - **Big-room/festival bass (Garrix, SHM-era big room):** short sine sub "kick-bass" plus a distorted saw/square mid layer, gated to the offbeat 1/8s.
  - **UKG/house bass (Fred again.., Calvin Harris house):** mono sine/triangle sub, an organ (square plus sine harmonics) or 2-saw reese mid, plus volume pump.
  - **Skrillex-pop crossover ("Rumble"):** a near-pure sine sub riff with light saturation. Minimalism is the point.
- Fred again..'s character comes from resampled audio rather than a synth patch. For an engine, approximate it with a noise/texture layer plus heavy pump. Treat it as a style, not an oscillator recipe.

### Gaps
- No primary-source patch numbers (voices, detune) for Illenium, Marshmello, Garrix, SHM, Whethan or San Holo. Searches returned only preset-pack marketing.
- No technical breakdown of the "Rumble" bass. A Musicians Institute masterclass article exists, but its contents could not be read: [MI](https://www.mi.edu/in-the-know/sound-design-tips-make-skrillexs-rumble-bass-line/).
- Odesza's synth gear: a 2025 Synth History interview exists but its details could not be read: [Synth History](https://www.synthhistory.com/post/interview-with-odesza).

## 2. Filter and envelope shapes (A/D/S/R in ms)

### Takeaway
Plucks have fast attack, short decay, sustain at or near zero, and a short release, with a filter envelope that closes on decay. Future bass chords have a short pitch-up envelope at note start. Sources give shapes, not millisecond values, so the ms numbers below are estimates.

### Cited Findings
- Pluck amp envelope: near-instant attack, short-to-medium decay, sustain 0 or very low, short release. Sustain at 0 makes the note die while held. — [Violet Recording](https://violetrecording.com/how-to-make-a-pluck-sound/)
- Pluck filter envelope: start open and close as the note decays, giving the "ptew" snap. — [Violet Recording](https://violetrecording.com/how-to-make-a-pluck-sound/)
- Decay "quick enough to give a tight, snappy character… but not so short that the sound feels too thin"; add "a touch of release". — [Avid: trance plucks (2024)](https://www.avid.com/resource-center/how-to-make-trance-plucks)
- Big-room pluck (Logic ES2): decay about a quarter of the range. — [Ask.audio: Big Room Plucks](https://ask.audio/articles/how-to-create-big-room-synth-plucks-in-logic-pro-x)
- Faithless pluck: sustain 0%, short decay, tempo-synced envelope times. — [Audiotool wiki](https://audiotool.fandom.com/wiki/Famous_sound_tutorial/Faithless%27_pluck)
- Future bass chord: a quick pitch envelope bending up at note start. — [Loopmasters](https://www.loopmasters.com/articles/4372-How-To-Make-A-Future-Bass-Drop-A-Future-Bass-Production-Tutorial)
- Flume lead: 24 dB lowpass, cutoff driven by a mod envelope plus an LFO. — [PML](https://www.productionmusiclive.com/blogs/news/flume-synth-with-sylenth1-designing-the-sound-from-lorde-tennis-court-flume-remix)
- Future bass chords: LP filter cutoff modulated by an LFO for "swelling"; filters "opened or closed quickly". — [Unison: how to make future bass](https://unison.audio/how-to-make-future-bass/); [FeelYourSound](https://feelyoursound.com/articles/future-bass-tutorial/)

### Inferences (all ms values are estimates from common practice, not sourced)
| Type | Attack | Decay | Sustain | Release | Filter |
|---|---|---|---|---|---|
| Future bass chord | 2-10 ms | 300-600 ms | 60-80% | 150-400 ms | LP 24 dB, cutoff 3-8 kHz, env amount moderate; pitch env +2 to +12 st decaying in 30-80 ms (estimate) |
| Festival supersaw lead | 1-5 ms | n/a | 100% | 100-300 ms (plus reverb) | Mostly open LP or none, light HP at 150-250 Hz |
| Pluck (house/future) | 0-2 ms | 150-350 ms | 0% | 50-150 ms | LP env from about 8-10 kHz down to about 500 Hz-1 kHz over the decay |
| Pad | 200-1500 ms | n/a | 100% | 1-3 s | LP 2-6 kHz, slow LFO |
| Bass sub (UKG/house) | 1-5 ms | 200-400 ms (organ bass) | 50-100% | 30-80 ms | Mid layer LP 1-3 kHz; sub unfiltered |
| Big-room bass | 0-2 ms | 80-200 ms | 0-30% | 20-60 ms | Gate-like |

### Gaps
- No source gave explicit ms values for any of these artists' patches.

## 3. Modulation: LFO rates and sidechain pumping

### Takeaway
Chords get rhythmic volume/filter modulation at 1/8 (Odesza "Line of Sight") or 1/16-1/8 triplets. Future bass uses swung or shaped LFOs. Fred again.. style pumping uses kick-keyed sidechain or a tremolo/volume shaper offset in time, applied to bass, percussion, risers and noise.

### Cited Findings
- Odesza "Line of Sight": volume LFO on the main saw chords at 1/8, "bouncy feel". — [Splice](https://splice.com/blog/hits-decoded-odesza-line-of-sight/)
- Future bass chords with a "swung LFO" (Serum preset plus Ableton session). — [Dylan Bowes (Gumroad)](https://dylanbowes.gumroad.com/l/futurebasschords)
- Rhythmic chord volume plus filter shaping with LFO Tool in future bass. — [Sleepfreaks: LFO Tool](https://sleepfreaks-dtm.com/en/softsynth/xfer-lfo-tool)
- Wobble filter LFO between 1/16 and 1/8, often triplet (forum answer). — [DJTechTools forum](https://forum.djtechtools.com/t/anybody-know-how-to-make-a-repetitive-wobble-bass/69195)
- Massive future bass filter LFO at 2/4 (slower sweep). — [Loopmasters](https://www.loopmasters.com/articles/4372-How-To-Make-A-Future-Bass-Drop-A-Future-Bass-Production-Tutorial)
- Future bass chords are often gated (Reason Alligator) as well as filter-swept; "bass playing long notes, chord instrument playing short, rhythmic notes". — [Reason Studios](https://www.reasonstudios.com/news/post/how-make-future-bass-tracks-in-reason-12); [FeelYourSound](https://feelyoursound.com/articles/future-bass-tutorial/)
- San Holo-style remake uses a "silent sidechain track" (ghost kick to trigger pumping). — [PML (future bass tag)](https://www.productionmusiclive.com/blogs/news/tagged/future-bass?page=2)
- Fred again../DJ Seinfeld style: percussion ducked to the 4/4 kick; bass sidechained to the kick; riser, downer and white-noise loop also sidechained. — [MusicRadar](https://musicradar.com/news/make-a-fred-again-style-track-free-samples)
- Fred again.. tricks: tremolo with stereo panning removed and timing offset as a pump instead of a compressor. — [MusicRadar](https://www.musicradar.com/artists/the-best-feeling-is-when-you-play-back-the-thing-youve-made-that-day-and-you-actually-feel-good-about-it-thats-the-drug-im-chasing-every-day-unlocking-the-production-tricks-that-fred-again-swears-by)
- "Jungle" bass: volume modulation is a key feature and gets the bass out of the four-on-the-floor kick's way; start from Flux Pro "Simple – Sidechain" and shape the curve to the bassline. — [MusicTech](https://musictech.com/tutorials/tips/how-to-recreate-fred-again-s-jungle-bass-sound)

### Inferences (estimates)
- Sync divisions at typical tempos: future bass 140-160 BPM (half-time feel), house/UKG 124-134, big room 126-128. A 1/8 at 150 BPM is 200 ms per cycle (5 Hz); 1/16 at 128 is about 117 ms (8.5 Hz).
- Pump shape for an engine: duck to about -12 to -24 dB at the kick (estimate; Fred again.. style is deeper, near full duck on bass, -6 to -10 dB on pads/chords), recovering over 60-80% of a quarter note with a concave curve. A volume-shaper LFO synced to 1/4 is the RT-safe way to implement this without a kick input.
- Future bass "wobble" chords: volume LFO 1/8 or 1/16, plus a slight pitch wobble (estimate ±10-30 cents) or vibrato. The pitch wobble comes from typical practice and was not confirmed by sources.

### Gaps
- No sourced numbers for sidechain depth (dB) or release (ms) for any named artist.
- No source confirmed a pitch-LFO "wobble" depth on future bass chords.

## 4. Distortion, saturation and OTT

### Takeaway
OTT (upward plus downward 3-band compression) is standard in EDM sound design. Depth is the main control, and high depth trades dynamics for "power". Saturation is used on chords and vocal-derived plucks (Flume), on drums (Flume), and lightly on subs (filter drive).

### Cited Findings
- OTT: 3-band up/down compression, "common in EDM, Dubstep, and most genres within electronic music"; lower depth keeps dynamics, higher depth adds power at the cost of musicality; faster time = tighter, slower = more transients. — [SampleFocus OTT guide](https://blog.samplefocus.com/blog/ott-compression-guide-what-it-is-and-how-to-use-multiband-compression)
- OTT on synths and pads adds clarity and power but easily flattens dynamics and adds noise. — [Unison: OTT](https://unison.audio/ott-plugin/)
- Flume: Saturn heavy saturation on vocal recordings to make lead and pluck chords. — [Warp Academy](https://warpacademy.com/?p=89809)
- Flume "Sirens" drums: VSM-3 crunch on the kick, Decapitator in parallel on a snare. — [MusicTech: Flume](https://musictech.com/guides/essential-guide/how-to-create-heavy-beats-gritty-synths-like-flume)
- Odesza: Decapitator and Devil-Loc for "vintage grit" on vocal chops. — [Soundtoys: Odesza](https://www.soundtoys.com/artist/odesza/)
- UKG sub: slight filter drive only. — [The Producer School](https://theproducerschool.com/blogs/featured-blogs/master-uk-garage-reese-bass-synthesis-in-serum-2-complete-sound-design-tutorial)

### Inferences (estimates)
- OTT depth by type: future bass chords 30-60%; festival leads 40-70%; plucks 20-40%; pads 10-30%; bass mid layer 50-100%; sub layer 0% (keep clean, consistent with the project's existing "clean sub, dirty mids" principle).
- Saturation: chords and leads mild soft-clip/tape (2-6 dB drive); big-room bass mid heavy (waveshaper or clipper); UKG organ bass light. Fred again.. style: lo-fi/bit-crushed textures on top layers, clean sub.

### Gaps
- No sourced OTT depth values for any named artist.

## 5. Stereo: mono crossover and width

### Takeaway
Keep the sub mono below about 100 Hz (some say 80-150 Hz) and put width in a separate high-passed layer above it. Most usable width sits between 150 Hz and 3 kHz.

### Cited Findings
- "Keep everything below 100 Hz mono" exists because of phase issues; Haas widening collapses in mono. — [Mastering The Mix: stereo width](https://www.masteringthemix.com/pages/how-to-get-the-right-stereo-width)
- Widen a layer high-passed above 100 Hz while the sub stays centred; controlled width at 60-150 Hz is acceptable if it passes a mono check. — [Mastering The Mix: width to bass](https://www.masteringthemix.com/blogs/learn/how-to-add-width-to-bass-without-losing-mono-compatibility)
- Band guide: 20-60 Hz highest collapse risk; 60-150 Hz widen with care; 150 Hz-3 kHz is where usable width lives. — [Mastering The Mix](https://www.masteringthemix.com/pages/how-to-get-the-right-stereo-width) (per search summary)
- Width in the bass is a "rule to break" when done carefully. — [MusicRadar: stereo bass](https://musicradar.com/how-to/how-to-break-the-rules-and-work-with-stereo-bass)
- UKG reese sub: stereo width 0. — [The Producer School](https://theproducerschool.com/blogs/featured-blogs/master-uk-garage-reese-bass-synthesis-in-serum-2-complete-sound-design-tutorial)
- Some chord patches set unison stereo width to 0 for an "analogue" feel. — [Future Music](https://www.pressreader.com/australia/future-music-9629/20170727/282291025290883)

### Inferences (estimates)
- Mono crossover per type: bass 100-120 Hz; leads, chords and plucks high-passed at 120-250 Hz anyway (consistent with commit 1728ed7). Pads mono below about 200 Hz.
- Width (side share): supersaw chords and leads very wide (correlation about 0.2-0.5); plucks moderately wide (0.5-0.7); bass mid layer moderate (0.6-0.8), sub fully mono; Fred again.. style pads wide but lo-fi.

## 6. Brightness, loudness and transients of a finished one-shot

### Takeaway
Masters land at about -6 to -9 LUFS integrated for club/festival, with big-room and festival trap loudest (-6 to -7). No source gives one-shot LUFS. Big-room and future bass are bright and dense; UKG/Fred again.. are darker and more dynamic.

### Cited Findings
- Club/festival releases around -6 to -9 LUFS; big-room/festival trap -6 to -7; deep house/techno -8 to -10; true peak ceiling -1 dBTP. — [Violet Recording: how to master EDM](https://violetrecording.com/how-to-master-edm/)
- No official club loudness standard; many aim near -6 LUFS. — [Sweetwater](https://www.sweetwater.com/insync/loudness-standards-lufs-peaks-and-streaming-limits)
- Beatport/club -9 to -6 LUFS vs streaming -14. — [AirGigs mastering listing](https://www.airgigs.com/mastering-engineers-for-hire/105024/Professional-Mastering-for-House-EDM--Electronic-Music-|-All-Genres-Welcome) (commercial)
- Spotify reference -14 LUFS, -1 dBTP (-2 if louder than -14). — [Soundcamps](https://soundcamps.com/de/blog/spotify-lufs/)

### Inferences (estimates)
- One-shot short-term loudness targets for the engine (sustained section of a held note, 0 dBFS true peak or below): festival lead/chord -8 to -10 LUFS-S; future bass chord -9 to -11; pluck -10 to -13 (lower because of decay; peak-to-loudness ratio 10-14 dB); pad -14 to -18; big-room bass -7 to -9; UKG/house bass -9 to -12; FX risers ramping to about -8.
- Brightness: big-room lead and future bass chord centroid high (estimate 2.5-4.5 kHz, with lots of energy above 4 kHz after OTT); UKG organ bass and Fred again.. pads darker (centroid under 1.5 kHz).
- Transients: future bass chords have a pitch-blip attack; plucks have the sharpest transients; pads have none; big-room bass is punchy (PLR 8-10 dB).

### Gaps
- No published LUFS/centroid figures for individual synth one-shots by genre. These need measuring from reference audio (the project's `ZeroAnalyze compare/tune` workflow).

## 7. Register (MIDI note range) per type

### Takeaway
No sources gave note ranges. Everything below is an estimate based on common arrangement practice and the tunings implied above.

### Cited Findings
- UKG sub one octave below the reese mid layer. — [The Producer School](https://theproducerschool.com/blogs/featured-blogs/master-uk-garage-reese-bass-synthesis-in-serum-2-complete-sound-design-tutorial)
- Future bass chord stack uses an octave-up oscillator. — [Loopmasters](https://www.loopmasters.com/articles/4372-How-To-Make-A-Future-Bass-Drop-A-Future-Bass-Production-Tutorial)
- Future bass voicings favour maj7/min7 chords. — [FeelYourSound](https://feelyoursound.com/articles/future-bass-tutorial/)

### Inferences (estimates; C3 = MIDI 60 here)
- Bass: root notes E0-G1 (MIDI 28-43; fundamentals 41-98 Hz), sub layer an octave down where relevant.
- Chords: 7th-chord voicings across C3-C5 (MIDI 48-72), plus octave layer.
- Leads: C4-C6 (MIDI 60-84), with festival leads often layered an octave up.
- Plucks: C4-C6 (MIDI 60-84).
- Pads: C3-C5 (MIDI 48-72).
- FX: noise risers have no pitch; tonal FX span the full range.

## 8. Artists who exemplify each type

### Takeaway
Artist-to-type mapping comes mostly from preset-pack marketing and track breakdowns. It is not from artists' own statements.

### Cited Findings
- Future bass genre canon per Loopmasters: Illenium, Flume, Odesza, Martin Garrix, NGHTMRE, The Chainsmokers, San Holo, Marshmello. — [Loopmasters Ultimate Future Bass for Serum Vol 3](https://www.loopmasters.com/genres/142-Future-Bass/products/12856-Ultimate-Future-Bass-for-Serum-Vol-3) (commercial)
- An Illenium-inspired pack (Aubit Sound "Awake") claims to study his entire catalogue. — [rekkerd.org](https://rekkerd.org/?p=126511) (commercial)
- SHM/Afrojack/Prydz big-room house references in Sylenth1 packs. — [ADSR Mainstage Sylenth1 presets](https://www.adsrsounds.com/product/presets/mainstage-sylenth1-presets/) (commercial)
- Fred again..: pump and sidechain processing, resampled audio basses. — [MusicRadar](https://musicradar.com/news/fred-again-jungle-bass-sound-dodgy-audio-cable); [MusicTech](https://musictech.com/tutorials/tips/how-to-recreate-fred-again-s-jungle-bass-sound)
- Odesza: 1/8 volume-LFO saw chords. — [Splice](https://splice.com/blog/hits-decoded-odesza-line-of-sight/)
- Flume: Sylenth1 detuned-saw leads, saturated vocal chord-plucks. — [PML](https://www.productionmusiclive.com/blogs/news/flume-synth-with-sylenth1-designing-the-sound-from-lorde-tennis-court-flume-remix); [Warp Academy](https://warpacademy.com/?p=89809)
- San Holo: warm guitar-like textures, bright clean drops (low-quality source). — [tilnote.io](https://tilnote.io/pages/69fc199855c6dfabca2647d9)
- Calvin Harris: 303 acid bass, Prophet/Juno/MS-20 analogue palette. — [EDM.com](https://edm.com/features/watch-calvin-harris-deconstruct-his-song-with-the-weeknd/); [Equipboard](https://equipboard.com/calvin-harris)

### Inferences
- Bass: Fred again.. / Skrillex "Rumble" (minimal sub plus pump, UK); Garrix / SHM (big-room offbeat bass); Illenium / Marshmello (future bass sub plus saw).
- Lead: Garrix, SHM (supersaw festival lead); Flume (detuned saw plus pulse, filtered).
- Chord: Illenium, San Holo, Marshmello (wobbly supersaw chords); Odesza (1/8 pumped saw/square chords).
- Pad: Odesza, Fred again.. (warm, pumping, lo-fi).
- Pluck: Marshmello, Whethan (bright, short, vocal-chop-like); Flume (saturated vocal plucks).
- FX: SHM/Garrix risers and white-noise sweeps; Fred again.. sidechained noise loops.

### Gaps
- Whethan: no sources found at all.
- Marshmello and Garrix: no artist-confirmed synth details found. A follow-up targeting YouTube studio videos (Garrix "Studio Time", Marshmello breakdowns) or Splice "Hits decoded" articles could fill this.
