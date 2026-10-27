# Tech House and Techno sound design targets (Bass, Lead, Chord, Pad, Pluck, FX)

Scope note: research was done with web search only (no page fetch tool was available), so findings come from search-result extracts of the cited pages. No artist in the brief has published patch-level settings (synth, cutoff, envelope ms) for their own tracks; every numeric value below comes from genre tutorials or from engineering convention, not from the artists. Items in "Inferences" that carry numbers are **estimates** meant as starting targets for the Zero Engine tuner. Check them against reference renders (`ZeroAnalyze compare`) before trusting them.

## Oscillator / source per type

### Takeaway
Tech house bass is simple: sine, square or saw, mono, with a short filter-envelope pluck in a rolling or offbeat pattern. Techno bass is either a 303-style acid line (saw or square, high resonance, glide) or a rumble made from a distorted, filtered reverb tail of the kick. Techno stabs and chords start from one square or a few slightly detuned saws. Hoover leads are a PWM, octave-stacked, chorused patch (an old-school sound, about 1991).

### Cited Findings
- Tech house bass: a Vital build uses a square on one oscillator and a saw on the second, with a 24 dB filter slope to keep the focus on the sub — [SoundBridge, tech house bass sound design](https://soundbridge.io/tech-house-bass-sound-design/)
- Tech house bass: two voices with about 5% unison for a slightly stereo bass, plus an envelope on the filter cutoff for movement — [SoundBridge, tech house bassline in Vital](https://www.soundbridge.io/create-a-tech-house-bassline-using-soundbridge-and-vital)
- Modern rolling tech house bass (Chris Stussy style): a sine-based Serum patch in **mono mode**, because polyphony muddies the low end; a 3-note pattern; change one note slot per bar so it doesn't sound looped — [The Producer School, Chris Stussy rolling bassline](https://theproducerschool.com/blogs/featured-blogs/building-a-rolling-bassline-like-chris-stussy-the-3-note-pattern-that-defines-modern-tech-house)
- Offbeat tech house bass: notes sit between kicks, and velocity is mapped to cutoff so harder notes are brighter — [The Producer School, modern tech house bass lines](https://theproducerschool.com/blogs/featured-blogs/master-modern-tech-house-bass-lines-complete-tutorial-guide)
- A guide modelled on Fisher, Chris Lake, Cloonee and Michael Bibi: a simple sine or square bass, light saturation, sidechained to the kick — [Beatportal step-by-step guide](https://www.beatportal.com/articles/692607-step-by-step-guide-to-producing-tech-house-like-fisher-chris-lake-cloonee-and-michael-bibi)
- Mau P, "Drugs From Amsterdam" (Beatport #1, 2023): he landed on a "hypnotic sine wave" sound for the drop lead, called it eerie, ravey and dirty, then layered a contrasting second synth (not named in the extract). The track also uses sampled Amsterdam Metro sounds — [Beatportal](https://www.beatportal.com/news/mau-p-delivers-a-beatport-number-1-with-drugs-from-amsterdam)
- Donk bass is an established tech house bass category: Serum 2 courses list "Donk Bass" next to tech house bass. No parameters were found — [Production Music Live, 5 basslines (Serum 2)](https://www.productionmusiclive.com/blogs/news/5-basslines-every-producer-should-know-max-styler-serum-2); [Groove3 Serum 2 Tech House listing via search](https://www.loopmasters.com/genres/66-Tech-House/products/16724-Tech-House-Production-Course)
- Rolling techno bassline: an arpeggiated sequence with filter-cutoff modulation, built in Serum from a square-type wave — [SoundBridge, deep analog techno bass](https://www.soundbridge.io/deep-analog-techno-bass-tutorial)
- Rumble bass (techno, especially hard techno): send the kick to a parallel return (not an insert) with a **3–5 s reverb at 100% wet**, distort it hard, keep only about **30 Hz to 150–200 Hz**, and sidechain it to the kick so it pumps on the offbeat — [The Producer School, techno rumble kicks 2025](https://theproducerschool.com/blogs/featured-blogs/how-to-create-techno-rumble-kicks-for-2025); [MusicRadar, rumbling techno kick](https://musicradar.com/how-to/rumbling-techno-kick); [Attack, adding reverb to a techno kick](https://www.attackmagazine.com/technique/tutorials/adding-reverb-techno-kick/)
- Rumble variant: an Auto Filter on the distorted reverb set to **96.6 Hz** — [Attack Magazine](https://www.attackmagazine.com/?p=63028); Bonedo starts the low-pass at about **250 Hz** and notes that more delay makes it more rhythmic, more reverb makes it darker — [Bonedo, rumble & rattle](https://www.bonedo.de/artikel/bass-drum-rumble-rattle-for-techno)
- Acid (303): a saw or square through a resonant low-pass. Resonance is what makes the "acid" sound. Start the cutoff at about the 10 o'clock position, raise resonance, use glide for slides, add overdrive (one walkthrough uses an MT-style overdrive) — [SoundBridge, design your own 303](https://www.soundbridge.io/design-your-own-303-sound-from-scratch); [Attack, acid synth in Diva](https://www.attackmagazine.com/technique/synth-secrets/acid-synth-uhe-diva/)
- Charlotte de Witte: reporting on her gear describes a 909 + 303 setup, with the 303 driving tension. Cutoff sweeps are used sparingly and stretched over long passages. Recent tracks are at 135+ BPM — [gearnews, Techno legends' gear: Charlotte de Witte](https://www.gearnews.com/techno-legends-gear-charlotte-de-witte/); [The Producer School, Charlotte de Witte style](https://theproducerschool.com/blogs/featured-blogs/how-to-create-techno-music-like-charlotte-de-witte)
- Techno stab: "essentially all you need is a square wave and some envelope modulation" — [Attack, techno synth stabs](https://www.attackmagazine.com/technique/synth-secrets/techno-synth-stabs/). A Juno-style stab guess: slightly detuned saws with a second oscillator at a fixed interval (a 4th, +5 semitones), minor chords — [Gearspace, techno stabs](https://gearspace.com/threads/techno-stabs.672979/) (forum, informal)
- Drumcode-style stabs (Adam Beyer's label) built in Thorn CM — [MusicRadar, Drumcode-esque techno stabs](https://musicradar.com/how-to/how-to-synthesize-drumcode-esque-techno-stabs)
- Hoover: Roland Alpha Juno "What the?" preset, about 1991. Three oscillators an octave apart, heavy PWM, thick chorus. Classic, not current mainstream — [Wikipedia, Hoover sound](https://en.wikipedia.org/wiki/Hoover_sound); Attack has a Diva "liquid hoover" reese-bass version — [Attack, liquid hoover bass](https://www.attackmagazine.com/technique/synth-secrets/liquid-hoover-bass/)
- I Hate Models: a Sequential Prophet-6 (6-voice poly) made "all the melodic parts, including the pads, the bassline, and the arpeggios" on his album opener. On early trance tracks he ran it through distortion pedals to build pads. Also uses a Jomox Xbase 999 for drums — [XLR8R, Studio Essentials: I Hate Models](https://xlr8r.com/features/studio-essentials-i-hate-models/)
- Melodic techno pluck/synth: short attack, fast decay, filter envelope on cutoff (Serum) — [MusicRadar, melodic techno synth sound](https://musicradar.com/how-to/how-to-program-a-melodic-techno-synth-sound)
- Dub techno chords: chord sample pitched to D# minor, then Redux with bit reduction at 6, then a band-pass Auto Filter (Basic Channel-style beat at 145 BPM) — [Attack, dub techno / Basic Channel articles](https://attackmagazine.com/?p=86844); minor chords plus a Synth > Delay > Reverb chain, "a lot of it is the chord rather than the process" — [Gearspace](https://gearspace.com/threads/chord-sounds-basic-channel.521312/)
- Sara Landry: inspired by early techno and dubstep, now a "high-definition" hard techno style with an aggressive, industrial palette. She learned synthesis from Francis Preve and was a beta tester for Live 11. No patch details — [Ableton, Sara Landry: High-End Hard Techno](https://www.ableton.com/fr/blog/sara-landry-high-end-hard-techno/)
- Indira Paganotto: known for blending psytrance and techno, which implies rolling 16th-note psy-style basslines and acid. Her 2025 remix of Landry's "Pressure" was their first official collaboration — [DJ Mag](https://djmag.com/news/sara-landrys-pressure-remixed-indira-paganotto-listen)

### Inferences
- Engine sources (estimates):
  - **TH Bass**: sine, plus 0–30% square/saw for mid harmonics; mono voice; unison off or 2 voices at ≤5% detune above the mono crossover only.
  - **TH Donk**: sine or triangle with a fast pitch envelope (+12 to +24 st decaying in about 20–40 ms) and a short band-pass "knock". This is a commonly used recipe, not verified in a source.
  - **Techno Bass**, two flavours: (a) acid: saw or square, 24 dB resonant LP, glide about 60–100 ms (estimate); (b) rumble: a low sine/noise tail, low-passed at about 100–250 Hz and ducked by the kick.
  - **Techno Lead**: hard/peak time favours distorted detuned saws or a hoover (PWM + octave stack + chorus). Melodic techno (Afterlife) favours saws or squares through a warm analog-style LP. I found no Anyma patch source.
- The Producer School mono-sine statement and the Beatportal guide agree: a tech house bass should be close to a pure sine below about 100 Hz. Grit belongs in an upper (mid) layer, which matches Zero Engine's existing "clean sub, dirty mids" split.

### Gaps
- No artist-confirmed patches for Dom Dolla, Fisher, John Summit, Chris Lake, Cloonee, Michael Bibi, Amelie Lens, KlangKuenstler, Anyma, Sara Landry or Indira Paganotto. Searches for interviews with plugin and setting detail returned nothing specific.
- No numeric donk recipe from a reputable source.
- Mau P's second (contrast) synth is not named in the available extract.

## Filter, envelopes (ms) and filter-envelope amount

### Takeaway
Sources mostly give settings as knob positions rather than ms. The consistent shapes are: tech house bass = LP with cutoff near 150 Hz, small envelope amount, short decay; acid = high resonance, swept cutoff; stab = 0 attack, sustain 0, envelope to cutoff about 3 o'clock; pad = long attack and release. Most ms numbers below are estimates.

### Cited Findings
- Tech house bass in Vital: resonance at 0, cutoff around **150 Hz**, a plucky envelope (quick release, a slight decay increase) mapped to cutoff by a small amount — [SoundBridge, tech house bassline in Vital](https://soundbridge.io/en/create-a-tech-house-bassline-using-soundbridge-and-vital)
- Low-pass tech house bass content above about **200–300 Hz**; the LP is modulated by a short-decay envelope — [SamplesFocus, Chris Lake-style track](https://samplefocus.com/blog/make-tech-house-track-chris-lake/); [Loopmasters tech house beat & bass](https://www.loopmasters.com/articles/4300-How-To-Make-A-Tech-House-Beat-Bass-Tutorial)
- Velocity to cutoff on the offbeat bass — [The Producer School](https://theproducerschool.com/blogs/featured-blogs/master-modern-tech-house-bass-lines-complete-tutorial-guide)
- Techno stab: mod envelope to cutoff at about **3 o'clock** (roughly 65–75% of range); amp attack **0**, long decay, sustain **0**, release high — [Attack, techno synth stabs](https://www.attackmagazine.com/technique/synth-secrets/techno-synth-stabs/). Sustain at 0% for a "stabby" feel; a slow attack gives a "womp" swell, short times give a pluck — [MusicRadar, Drumcode-esque stabs](https://musicradar.com/how-to/how-to-synthesize-drumcode-esque-techno-stabs)
- Acid: cutoff starts low (about 10 o'clock) with high resonance, and sweeps are automated over long passages (de Witte) — [SoundBridge 303](https://www.soundbridge.io/design-your-own-303-sound-from-scratch); [gearnews](https://www.gearnews.com/techno-legends-gear-charlotte-de-witte/)
- Dub chord EQ: about **−7 dB at 600 Hz**, wide Q (hollow sound), HP at about **100 Hz**, LP at about **4.5 kHz**; a low-wet plate reverb; RE-201-style tape delay — [Attack, dub techno synth chords](https://www.attackmagazine.com/technique/synth-secrets/dub-techno-synth-chords/)
- Atmospheric pad: the filter acts on the oscillator, with extended sustain and release, then LFO modulation — [SoundBridge, atmospheric pad](https://soundbridge.io/design-your-own-atmospheric-pad-sound)
- Synthwave/101 laser bass (not TH/techno, but a related shape): filter env attack 0, decay and sustain high, resonance raised for squelch — [Attack, laser-gun bassline](https://www.attackmagazine.com/technique/synth-secrets/how-to-make-a-synthwave-laser-gun-bassline/)

### Inferences (all ms values are estimates translated from knob descriptions and common practice)
| Profile | Filter | Cutoff base | Res | Filt env A/D/S/R | Env amt | Amp A/D/S/R |
|---|---|---|---|---|---|---|
| TH Bass (rolling) | LP 24 dB | 120–250 Hz | 0–15% | 0 / 80–180 ms / 0–20% / 50 ms | +1 to +2 oct | 1–3 ms / 150–250 ms / 60–80% / 30–60 ms |
| TH Donk | BP or LP 12 dB | 300–800 Hz | 30–50% | 0 / 30–60 ms / 0 / 30 ms | +2 to +3 oct | 0–1 ms / 100–150 ms / 0 / 40 ms |
| Techno Bass, acid | LP 24 dB (diode-ish) | 200–600 Hz | 70–90% | 0 / 150–300 ms / 0 / 50 ms (accent shortens decay) | +2 to +4 oct | 1 ms / 200 ms / 70% / 20 ms, glide 60–100 ms |
| Techno Bass, rumble/sub | LP 12–24 dB | 90–250 Hz | 0–10% | none | 0 | attack 0, decay to the next kick (about 300–450 ms at 135–145 BPM), ducked |
| Techno Stab/Chord | LP 24 dB | 400–1000 Hz | 15–30% | 0 / 150–300 ms / 0 / long | +2 to +3 oct | 0 / 200–400 ms / 0 / 300–800 ms (+ reverb send) |
| Dub Chord | LP + BP; EQ −7 dB at 600 Hz, HP 100 Hz, LP 4.5 kHz | 1–2 kHz | 20% | 0 / 300 ms / 0 | +1 oct | 0 / 300 ms / 0 / 500 ms, with tape delay feedback 40–60% |
| Techno Lead (hard) | LP 12 dB or none | 3–8 kHz | 10% | slow env or LFO | small | 5 ms / – / 100% / 150 ms |
| Melodic techno lead/arp | LP 24 dB (Diva-like) | 600–2000 Hz | 20–40% | 0 / 200–400 ms / 30% | +2 oct | 2 ms / 300 ms / 50% / 200 ms |
| Pluck (both) | LP 24 dB | 300–800 Hz | 15–25% | 0 / 80–200 ms / 0 | +3 oct | 0 / 150–300 ms / 0 / 150 ms |
| Pad (both) | LP 12 dB | 1–3 kHz | 10% | slow LFO on cutoff | 0 | 300–1500 ms / – / 100% / 1.5–4 s |

### Gaps
- No source gives tech house or techno filter envelopes in ms; the table is extrapolated.
- No tech house pad-specific tutorial was found. The tech house "pad" is often a filtered chord stab loop or vocal pad in sample packs ([Mind Flux / rekkerd listing of a free pack inspired by Fisher, Chris Lake, Dom Dolla, Mau P, John Summit with stab, chord and pad loops](https://rekkerd.org/?p=267535)).

## Modulation (LFO rates, targets) and tempo

### Takeaway
Tech house sits at about 124–128 BPM (packs label 125 BPM). Melodic techno runs about 120–128, peak-time and Charlotte de Witte-style techno 135+, hard techno 140–155+. Movement comes mostly from sidechain pumping at 1/4 notes, cutoff automation over 8–32 bars, velocity-to-cutoff and rhythmic gating, rather than fast free LFOs.

### Cited Findings
- Tech house chord loops are labelled at 125 BPM — [Loopmasters House Chords 3](https://www.loopmasters.com/products/8980)
- Charlotte de Witte tracks mostly at 135 BPM and above — [The Producer School](https://theproducerschool.com/blogs/featured-blogs/how-to-create-techno-music-like-charlotte-de-witte)
- A Basic Channel-style dub beat recreated at 145 BPM — [Attack](https://attackmagazine.com/?p=86844)
- Tech house bass sidechained to the kick for pumping — [Beatportal](https://www.beatportal.com/articles/692607-step-by-step-guide-to-producing-tech-house-like-fisher-chris-lake-cloonee-and-michael-bibi); the rumble is sidechained to pump on the offbeat — [The Producer School rumble](https://theproducerschool.com/blogs/featured-blogs/how-to-create-techno-rumble-kicks-for-2025)
- Stab tutorials recommend automating cutoff and envelope attack/decay through the track; a Massive stab uses a rhythmic "performer" (step-LFO) pulse — [MusicRadar stabs](https://musicradar.com/how-to/how-to-synthesize-drumcode-esque-techno-stabs); [SoundBridge mau5 stabs](https://www.soundbridge.io/mau5-stabs)
- Rolling techno bass = arpeggiated sequence plus cutoff modulation — [SoundBridge deep techno bass](https://www.soundbridge.io/deep-analog-techno-bass-tutorial)
- Techno noise FX: a "filtered, pumping noise" preset with reverb and delay (Producertech course) — [Loopmasters, Essential Techno Sound Design](https://www.loopmasters.com/products/7292-Essential-Techno-Sound-Design)

### Inferences (estimates)
- Motion macro mapping:
  - TH bass: an LFO on cutoff at 1/4 or 1/8 dotted, depth ±20–40% (about ±0.5 oct), or a 1/16 step gate.
  - Techno lead/stab: 1/16 step-gate or 1/8 dotted delay.
  - Acid: a slow cutoff LFO at 4–8 bars (very slow, sweep-like, which mimics de Witte's long sweeps).
  - Pad: cutoff LFO at 1–4 bars, ±10–20%, plus slow pan/chorus at 0.1–0.5 Hz.
  - Rumble: 1/4 duck shape (gain 0 at the kick, recovering in about 150–250 ms).
- At 126 BPM, 1/4 = 476 ms and 1/16 = 119 ms. At 140 BPM, 1/4 = 429 ms and 1/16 = 107 ms.

### Gaps
- No artist-specific LFO settings were found.

## Distortion / saturation / compression / OTT

### Takeaway
Tech house: light saturation for warmth on bass, with grit kept above the sub. Techno: much heavier distortion, especially on rumble (distorted reverb return) and hard techno leads. Distortion often sits before the filter or reverb (reverb first, then driven). There's little evidence of OTT as a genre habit in these sources.

### Cited Findings
- Tech house bass: "light saturation for warmth" — [Beatportal guide](https://www.beatportal.com/articles/692607-step-by-step-guide-to-producing-tech-house-like-fisher-chris-lake-cloonee-and-michael-bibi)
- Rumble: aggressive distortion of the 100%-wet reverb return. Plugins cited across guides include Ableton Hybrid Reverb, Decapitator, Trash, Saturn. The kick transient "has to survive the saturation" — [The Producer School rumble](https://theproducerschool.com/blogs/featured-blogs/how-to-create-techno-rumble-kicks-for-2025)
- 303: overdrive after the filter (MT-style) — [SoundBridge 303](https://www.soundbridge.io/design-your-own-303-sound-from-scratch)
- I Hate Models: a Prophet-6 through distortion pedals for pads — [XLR8R](https://xlr8r.com/features/studio-essentials-i-hate-models/)
- Stabs: reverb first in the chain, so later effects drive and saturate it — [MusicRadar stabs](https://musicradar.com/how-to/how-to-synthesize-drumcode-esque-techno-stabs)
- Dub chords: Redux bit reduction to 6 bits, then a band-pass to tame the noise — [Attack](https://attackmagazine.com/?p=86844)
- White-noise riser: distortion for fullness, plate reverb, EQ cutting the lows — [SoundBridge noise riser](https://www.soundbridge.io/create-a-perfect-white-noise-riser)

### Inferences (estimates)
- Grit macro defaults:
  - TH Bass: 10–25% soft clip, only above about 120 Hz (mid layer).
  - Techno Bass, acid: 30–60% overdrive after the filter.
  - Techno rumble: 60–90% drive on the tail, then LP.
  - Hard techno lead: 50–80% (hard clip or fold), with a post-LP at about 8–10 kHz to remove fizz.
  - Melodic techno lead: 10–20% tape-style.
  - Pads: 0–15% (I Hate Models-style pads up to 40%).
- OTT: suggest low depth (0–25%) for tech house and techno. Their aesthetic leans on transient punch (the club-master note below), unlike dubstep/EDM.

### Gaps
- No sources found on OTT or multiband-compression habits for these genres.

## Stereo: mono crossover and width

### Takeaway
Keep bass mono below about 100 Hz (vinyl and club convention). Sub-only content can be high-passed at about 30 Hz. Stabs, pads and leads get width from detune, chorus, unison, delay and reverb.

### Cited Findings
- Elliptic EQ (vinyl-cutting heritage) sums stereo to mono below a set frequency. Bass is traditionally mono for vinyl and so all clubbers feel the same low end. A user keeps "approx 100hz and below mono". Bass compensation after mono-summing is 0.5–2 dB — [Audacity forum, elliptical EQ](https://forum.audacityteam.org/t/elliptical-equalizer-mono-crossfeed-filter/18349); [KVR, why bass mono](https://www.kvraudio.com/forum/viewtopic.php?p=8971452); [HoRNet ElliptiQ](https://www.hornetplugins.com/plugins/hornet-elliptiq)
- Tech house sub on club systems: clubs reproduce 30–100 Hz with huge headroom; high-pass bass at about 30 Hz; use absolute mono compatibility and aggressive sidechaining — [The Producer School, sub bass for tech house](https://theproducerschool.com/blogs/featured-blogs/how-to-mix-sub-bass-for-tech-house-without-it-disappearing-on-club-systems); [Midnight Rebels](https://midnightrebels.com/mixing-sub-bass-club-sound-systems/)
- Stabs: panning spread for width — [SoundBridge mau5 stabs](https://www.soundbridge.io/mau5-stabs); unison keeps punch where heavy reverb would smear — [MusicRadar stabs](https://musicradar.com/how-to/how-to-synthesize-drumcode-esque-techno-stabs)
- TH bass: 2 voices, 5% unison (stereo bass) — [SoundBridge](https://www.soundbridge.io/create-a-tech-house-bassline-using-soundbridge-and-vital)

### Inferences (estimates)
- Mono below: Bass 100–150 Hz (TH and Techno; rumble fully mono). Lead/Chord/Pluck 150–200 Hz (they are high-passed anyway). Pad 200 Hz.
- Side share targets (estimate): Bass < 5%; TH Pluck/Chord 15–30%; Techno Stab 15–25% dry, wider with the reverb tail; Pad 30–45%; Hard techno Lead 10–25% (often fairly central, mono-ish for club punch); Melodic techno Lead 20–35%; FX 30–50%.

### Gaps
- No measured width data for releases by these artists. Use `ZeroAnalyze analyze` on reference folders.

## Brightness/tilt, loudness, transient character

### Takeaway
Club-oriented tech house and techno masters land around −6 to −8 LUFS integrated with about −0.7 to −1 dBTP. Club versions emphasise punch and transient clarity over maximum LUFS. Dub/melodic chords are darker (LP about 4.5 kHz); hard techno is the brightest and most saturated.

### Cited Findings
- Club masters around −6 LUFS; drops near −6 LUFS while keeping transients intact — [Sweetwater, loudness standards](https://www.sweetwater.com/insync/loudness-standards-lufs-peaks-and-streaming-limits); [Mastering The Mix, mastering for mediums](https://masteringthemix.com/blogs/learn/mastering-for-different-mediums-streaming-club-cd-and-vinyl)
- A techno producer's universal masters: about −8 LUFS integrated, −0.7 dBTP; a melodic techno engineer's club master prioritises punch and transient clarity over raw LUFS — [SoundBetter profile (arkeØn)](https://soundbetter.com/profiles/684331-arkeØn) (single-producer claims, low authority)
- Dub chords low-passed at about 4.5 kHz and high-passed at about 100 Hz (darker tilt) — [Attack](https://www.attackmagazine.com/technique/synth-secrets/dub-techno-synth-chords/)
- Sara Landry's style is described as "high-definition" hard techno with an aggressive, industrial palette — [Ableton](https://www.ableton.com/fr/blog/sara-landry-high-end-hard-techno/)

### Inferences (estimates for finished one-shots/loops of individual sounds, not full mixes)
- One-shot loudness targets (short-term LUFS over the note): Bass −8 to −10, Lead −9 to −11, Chord/Stab −10 to −12, Pluck −11 to −13, Pad −13 to −16, FX −10 to −14. Peak ≤ −0.5 dBFS.
- PLR: TH bass and plucks 8–12 dB (punchy); hard techno lead 6–9 dB (dense, clipped); pads 6–10 dB.
- Brightness (% energy above 4 kHz): TH Bass < 2%; Techno acid 5–12%; Hard techno lead 15–30%; Melodic techno lead 8–15%; Dub chord < 5%; Pad 5–12%; FX (noise riser) 25–45%.
- Transients: TH bass has a fast 1–3 ms attack with a short pluck. Stabs and plucks have 0 ms attack and a click-free knock. Pads have slow swells. The rumble attack is deliberately softened and ducked.

### Gaps
- No measured spectral data of reference stems found; use `ZeroAnalyze compare` on owner-supplied references.

## Register (MIDI note range)

### Takeaway
No source gives explicit MIDI ranges. Ranges below are inferred from the stated filter and cutoff and the sub-focus targets.

### Cited Findings
- TH bass focus: cutoff around 150 Hz with energy below 200–300 Hz, which places fundamentals at about 40–100 Hz — [SoundBridge](https://soundbridge.io/en/create-a-tech-house-bassline-using-soundbridge-and-vital); [SamplesFocus](https://samplefocus.com/blog/make-tech-house-track-chris-lake/)
- The rumble lives at about 30–200 Hz (fundamental tied to the kick, often in F–A key range) — [The Producer School rumble](https://theproducerschool.com/blogs/featured-blogs/how-to-create-techno-rumble-kicks-for-2025)
- A Basic Channel chord pitched to D# minor — [Attack](https://attackmagazine.com/?p=86844)

### Inferences (estimates, MIDI note numbers with C4 = 60; C1 = 24, C2 = 36, C3 = 48, C4 = 60)
- TH Bass: E1–G2 (28–43), roots mostly F1–A1 (about 44–55 Hz).
- TH Donk: C2–C3 (36–48).
- Techno Bass, acid: C2–C4 (36–60), mainly C2–C3.
- Rumble: A0–F1 (21–29).
- Techno Lead: C3–C5 (48–72). Hoover: two octaves lower too.
- Chord/Stab: C3–C5 (48–72), minor triads/7ths/9ths.
- Pluck: C4–C6 (60–84).
- Pad: C3–C5 (48–72).
- FX: unpitched or C2–C6.

### Gaps
- No artist-specific key or register data.

## Which artists exemplify each type

### Takeaway
Artist-to-type mapping is mostly drawn from genre descriptions. Concrete production evidence exists only for Mau P (sine lead), Charlotte de Witte (303/909 acid), I Hate Models (Prophet-6 pads, bass, arps, distorted pads) and Indira Paganotto (psytrance-techno blend).

### Cited Findings
- Fisher "Losing It" is cited as a defining tech house track: four-to-the-floor kicks, offbeat hats, rolling basslines — [SoundBridge](https://soundbridge.io/tech-house-bass-sound-design/)
- Fisher, Chris Lake, Cloonee, Michael Bibi named as models for the sine/square, sidechained bass style — [Beatportal](https://www.beatportal.com/articles/692607-step-by-step-guide-to-producing-tech-house-like-fisher-chris-lake-cloonee-and-michael-bibi)
- Mau P: hypnotic sine lead, layered synth contrast, sampled metro sounds — [Beatportal](https://www.beatportal.com/news/mau-p-delivers-a-beatport-number-1-with-drugs-from-amsterdam)
- Charlotte de Witte: 303 acid lines, long sparing cutoff sweeps, 135+ BPM — [gearnews](https://www.gearnews.com/techno-legends-gear-charlotte-de-witte/)
- I Hate Models: Prophet-6 for pads, bass and arps; distortion pedals on pads — [XLR8R](https://xlr8r.com/features/studio-essentials-i-hate-models/)
- Adam Beyer / Drumcode: the square-ish, filtered stab archetype — [MusicRadar Drumcode-esque stabs](https://musicradar.com/how-to/how-to-synthesize-drumcode-esque-techno-stabs)
- Anyma / Afterlife: melodic techno and melodic house with ambient, electronica and classical influences. No patch detail — [Wikipedia, Anyma](https://en.wikipedia.org/wiki/Anyma)
- Sara Landry: industrial, high-definition hard techno — [Ableton](https://www.ableton.com/fr/blog/sara-landry-high-end-hard-techno/)
- Indira Paganotto: psytrance + techno — [DJ Mag](https://djmag.com/news/sara-landrys-pressure-remixed-indira-paganotto-listen)

### Inferences
- Suggested exemplar pairings (editorial, not sourced):
  - TH Bass: Fisher, Cloonee, Michael Bibi (rolling, offbeat); Chris Lake (groovy, sometimes donk-adjacent).
  - TH Lead: Mau P (sine/ravey), John Summit and Dom Dolla (vocal-led, plucky synth hooks).
  - TH Pad/Chord: Dom Dolla, John Summit (filtered house chords).
  - Techno Bass: de Witte and Amelie Lens (acid), hard techno (rumble), Paganotto (psy rolling 16ths).
  - Techno Lead: I Hate Models (distorted trance-like), KlangKuenstler and Sara Landry (hard, hoover/rave), Anyma (melodic arps).
  - Techno Chord: Adam Beyer / Drumcode (stabs), dub techno (classic).
  - FX: hard techno noise risers and rumbles.

### Gaps
- No verifiable sources were found for Amelie Lens, KlangKuenstler, Dom Dolla, John Summit, Cloonee or Michael Bibi sound design. A search for Bibi/Cloonee plugin interviews returned nothing relevant.
