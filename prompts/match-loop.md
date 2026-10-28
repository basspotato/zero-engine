# Reference Match improvement loop

Run with: `/loop` followed by the prompt below (no interval: Claude paces itself).

---

Keep improving Zero Engine's Reference Match until it is good on real-world files, working on your own in rounds. I review results; you do the work.

**Goal (measurable):**
- Owner's references in `F:\tests` (every `file to be matched*.wav`, plus any added later): each comes back CLOSE or VERY CLOSE, judged by `ZeroAnalyze matchbench F:/tests`.
- Engine benchmark (`ZeroAnalyze matchbench`): mean mel difference at or under 4.0 dB without getting worse on timbre or brightness.
- A match in the plugin takes 20 s or less on this PC.
- `ZeroAnalyze test` passes (the 5 Hz filter-wobble case included).

**Each round:**
1. Read `analysis/match_log.md` (create it if missing) for what has been tried, and `CLAUDE.md`.
2. Pick the single biggest remaining error on the owner's files: run `ZeroAnalyze match <file> Synthwave Pluck -v`, read the loss breakdown and the "with OTT / reverb / spectral off" lines, and state one hypothesis.
3. Make one focused change. Measure it: `matchbench F:/tests` before and after, plus `test` if engine or matcher code changed.
4. Keep it only if the owner's files or the benchmark improve without the other getting worse; otherwise revert it. Never keep a change because it "should" help.
5. Commit kept changes locally with the numbers in the message. Append one entry to `analysis/match_log.md`: hypothesis, change, before/after numbers, kept or reverted.
6. Schedule the next round.

**Allowed on your own:** changes to SoundPrint, ReferenceMatch, Cmaes, the analysis tool and tests; building ZeroAnalyze, the VST3 and the exe; local commits; web research for methods (permissive licences only: MIT, BSD, Apache, ISC, zlib, public domain).

**Never without asking first (stop and ask instead):**
- Pushing to GitHub.
- Changes that alter how existing presets sound (engine DSP, profiles, macro maps, envelopes, finishing chain defaults), new user-facing controls, or anything the owner approved by ear.
- GPL/LGPL/AGPL code in the plugin, or training data whose licence forbids it.
- Allocation, locks, file I/O or inference on the audio thread.

**Stop the loop and report when:**
- The goal is met; or
- three rounds in a row are reverted (stuck: report what blocks it and propose the next bigger step, such as a transient layer or a learned starting point with CMA-ES); or
- a fix needs one of the "ask first" changes; or
- eight rounds have run (check in either way).

**Report:** a few bullets: owner-file results (word per file), benchmark numbers, what changed, what's next. Keep it short; the log holds the detail.
