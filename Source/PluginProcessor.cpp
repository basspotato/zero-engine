#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "SynthVoice.h"
#include "Wavetable.h"
#include "Profiles.h"
#include "GenerateTargets.h"
#include "ArpGrooves.h"
#include "GenerateNet.h"
#include "Moods.h"
#include <thread>


ZeroEngineProcessor::ZeroEngineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ZeroEngine", createParameterLayout()),
      macroMap (state),
      synth (state),
      grit (*state.getRawParameterValue (ParamIDs::grit)),
      air (*state.getRawParameterValue (ParamIDs::air)),
      width (*state.getRawParameterValue (ParamIDs::width)),
      output (*state.getRawParameterValue (ParamIDs::output)),
      levelTrim (*state.getRawParameterValue (ParamIDs::levelTrim)),
      subSplit (*state.getRawParameterValue (ParamIDs::subSplit)),
      subOn (*state.getRawParameterValue (ParamIDs::subOn)),
      midHighPass (*state.getRawParameterValue (ParamIDs::midHighPass)),
      ottDepth (*state.getRawParameterValue (ParamIDs::ottDepth)),
      spectral (*state.getRawParameterValue (ParamIDs::spectral)),
      tilt (*state.getRawParameterValue (ParamIDs::tilt)),
      monoFreq (*state.getRawParameterValue (ParamIDs::monoFreq)),
      stereoWidth (*state.getRawParameterValue (ParamIDs::stereoWidth)),
      delayMix (*state.getRawParameterValue (ParamIDs::delayMix)),
      delayTime (*state.getRawParameterValue (ParamIDs::delayTime)),
      delayFeedback (*state.getRawParameterValue (ParamIDs::delayFeedback)),
      reverbMix (*state.getRawParameterValue (ParamIDs::reverbMix)),
      reverbSize (*state.getRawParameterValue (ParamIDs::reverbSize))
{
    WavetableBank::get(); // build tables here, never on the audio thread

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new ZeroVoice (state, macroMap, hostBpm, sampleTable, livePosition, i));

    synth.addSound (new ZeroSound());

    for (auto [slot, id] : { std::pair { 0, ParamIDs::arpOn }, { 1, ParamIDs::arpMode }, { 2, ParamIDs::arpRate }, { 3, ParamIDs::arpOctaves }, { 4, ParamIDs::arpGate } })
        arpParams[slot] = state.getRawParameterValue (id);
    seqLengthParam = state.getRawParameterValue (ParamIDs::seqLength);
    arpLatchParam = state.getRawParameterValue (ParamIDs::arpLatch);
    for (auto [m, id] : { std::pair { 0, ParamIDs::weight }, { 1, ParamIDs::grit }, { 2, ParamIDs::punch }, { 3, ParamIDs::width }, { 4, ParamIDs::motion }, { 5, ParamIDs::air } })
        macroParams[m] = state.getParameter (id);
    for (int step = 0; step < Arpeggiator::maxSteps; ++step)
    {
        seqParams[0][step] = state.getRawParameterValue (SeqIDs::on (step));
        seqParams[1][step] = state.getRawParameterValue (SeqIDs::pitch (step));
        seqParams[2][step] = state.getRawParameterValue (SeqIDs::accent (step));
    }

    syncProfile(); // start on the default genre/type's profile
    startTimerHz (10);
}

void ZeroEngineProcessor::timerCallback()
{
    syncProfile();

    // Taste: time the last Generate result spends being played.
    if (showingLatest && ! shown.empty())
        for (const auto& word : soundingNotes)
            if (word.load (std::memory_order_relaxed) != 0)
            {
                shown.back().playedSeconds += 0.1f; // the timer runs at 10 Hz
                break;
            }

    // A finished Reference Match loads here, on the message thread.
    if (matcher != nullptr && (matcher->getState() == ReferenceMatch::State::done || matcher->getState() == ReferenceMatch::State::failed))
    {
        const auto result = matcher->take();
        if (result.ok)
        {
            if (result.target.harmonics.has_value()) // the matched sound may use the reference's own wavetable
                setSampleHarmonics (*result.target.harmonics);
            loadProfile (result.genre, result.type, result.profile);
        }
        matchSummary = result.summary;
        matchSummaryUntil = juce::Time::getMillisecondCounterHiRes() + 6000.0;
    }

    if (! juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
        checkpoint(); // after syncProfile, so a Genre/Type change and its new sound are one step
}

void ZeroEngineProcessor::syncProfile()
{
    const auto genre = (int) state.getRawParameterValue (ParamIDs::genre)->load();
    const auto type = (int) state.getRawParameterValue (ParamIDs::type)->load();

    if (genre == appliedGenre && type == appliedType)
        return;

    loadProfile (genre, type, makeProfile (genre, type));
    showingLatest = false;
    currentHeart = -1; // a designed sound now, not the last Generate result
}

void ZeroEngineProcessor::loadProfile (int genre, int type, const Profile& profile)
{
    auto setChoice = [this] (const char* id, int index)
    {
        auto* param = state.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
    };

    appliedGenre = genre;
    appliedType = type;
    setChoice (ParamIDs::genre, genre);
    setChoice (ParamIDs::type, type);
    applyProfile (state, profile);
    loadMacroMap (macroMap, profile);
}

ZeroEngineProcessor::GeneratePlan ZeroEngineProcessor::prepareGenerate (juce::int64 seed, bool moodMode)
{
    syncProfile(); // a Genre/Type change not yet picked up by the timer
    // The seed scrambled first (splitmix64): clicks a moment apart give nearby
    // seeds, and nearby seeds give a plain random generator near-identical
    // first draws (the same genres and variations click after click).
    auto mixed = (juce::uint64) seed + 0x9e3779b97f4a7c15ull;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ull;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebull;
    GeneratePlan plan { juce::Random ((juce::int64) (mixed ^ (mixed >> 31))), appliedGenre, appliedType.load(), -1 };

    // Mood mode: a mood (never the last one twice running) picks the type; the
    // genre box keeps the genre, and the display shows the mood's name.
    if (moodMode)
    {
        plan.mood = (lastMood + 1 + plan.random.nextInt (Moods::count - 1)) % Moods::count;
        if (lastMood < 0)
            plan.mood = plan.random.nextInt (Moods::count);
        lastMood = plan.mood;
        plan.type = Moods::typeOf (plan.mood);
    }

    // Taste: what became of the last result decides a judgment or two.
    loadTasteAndHearts();
    judgeLastShown();
    lastPickTraits.reset();
    return plan;
}

void ZeroEngineProcessor::applyGenerate (GeneratePlan& plan, const Profile& profile, int source)
{
    // History, like a browser: going back and generating drops what was ahead;
    // the sound before the first click (or any tweak since) is kept as a step too.
    history.erase (history.begin() + juce::jmin ((int) history.size(), historyPos + 1), history.end());
    if (history.empty() || ! history.back().isEquivalentTo (state.state))
        history.push_back (state.copyState());

    // In Genre mode the result loads as its genre; a mood keeps the genre box.
    loadProfile (plan.mood >= 0 ? plan.genre : source, plan.type, profile);
    if (plan.mood >= 0)
    {
        matchSummary = Moods::nameOf (plan.mood);
        matchSummaryUntil = juce::Time::getMillisecondCounterHiRes() + 3000.0;
    }
    currentHeart = -1;
    if (lastPickTraits.has_value()) // a measured pick (not a fallback)
    {
        shown.push_back ({ TasteModel::featuresOf (*lastPickTraits, plan.type), juce::Time::getMillisecondCounterHiRes() / 1000.0 });
        if (shown.size() > 12)
            shown.erase (shown.begin());
        showingLatest = true;
    }

    generateArp (plan.type, plan.random);

    history.push_back (state.copyState());
    constexpr size_t maxHistory = 50;
    if (history.size() > maxHistory)
        history.erase (history.begin(), history.begin() + (std::ptrdiff_t) (history.size() - maxHistory));
    historyPos = (int) history.size() - 1;
}

void ZeroEngineProcessor::generate (juce::int64 seed, bool moodMode)
{
    auto plan = prepareGenerate (seed, moodMode);
    const auto [profile, source] = pickGeneratedProfile (plan.genre, plan.type, plan.random, plan.mood);
    applyGenerate (plan, profile, source);
}

void ZeroEngineProcessor::generateAsync (bool moodMode, std::function<void()> done)
{
    // The renders and checks (about 0.4 s) run on a worker, so the tesseract
    // folds at once and the sound arrives mid-fold; the result is applied back
    // on the message thread. A click while one is running is ignored.
    if (generating)
        return;
    generating = true;
    auto plan = prepareGenerate (juce::Time::currentTimeMillis(), moodMode);
    generateWorker.addJob ([this, plan, done, alive = alive]() mutable
    {
        const auto [profile, source] = pickGeneratedProfile (plan.genre, plan.type, plan.random, plan.mood);
        juce::MessageManager::callAsync ([this, plan, profile = profile, source = source, done, alive]() mutable
        {
            if (! alive->load())
                return;
            applyGenerate (plan, profile, source);
            generating = false;
            if (done)
                done();
        });
    });
}

void ZeroEngineProcessor::loadTasteAndHearts()
{
    if (heartsLoaded)
        return;
    heartsLoaded = true;
    taste.load (tasteFile());

    // Hearts are stored as whole states (the sound exactly as hearted, arp and
    // all); the profile Generate breeds from is read back through a private
    // engine's parameters.
    if (const auto xml = juce::XmlDocument::parse (heartsFile()))
    {
        ZeroEngineProcessor reader;
        reader.stopProfileSync();
        for (const auto* h : xml->getChildIterator())
            if (const auto* params = h->getFirstChildElement())
            {
                Heart heart;
                heart.genre = juce::jlimit (0, anyGenre - 1, h->getIntAttribute ("genre"));
                heart.type = juce::jlimit (0, 5, h->getIntAttribute ("type"));
                heart.state = juce::ValueTree::fromXml (*params);
                reader.state.replaceState (heart.state.createCopy());
                heart.profile = readProfile (reader.state, heart.genre, heart.type);
                const auto values = juce::StringArray::fromTokens (h->getStringAttribute ("features"), ",", "");
                for (int k = 0; k < juce::jmin (values.size(), TasteModel::numFeatures); ++k)
                    heart.features[(size_t) k] = values[k].getFloatValue();
                heart.name = h->getStringAttribute ("name");
                hearts.push_back (heart);
            }
    }
    for (auto& h : hearts) // hearts from before they had names
        if (h.name.isEmpty())
            h.name = randomName();
}

void ZeroEngineProcessor::judgeLastShown()
{
    // The last result, now that something else is wanted: played for 6 s or
    // more, it was kept; replaced within 4 s without a note played, skipped.
    // A kept one beats every skipped one among the recent results, and the
    // other way round (hearts have already judged their own).
    if (shown.empty() || shown.back().label != 0)
        return;
    auto& last = shown.back();
    const auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    if (last.playedSeconds >= 6.0f)
        last.label = 1;
    else if (last.playedSeconds < 0.3f && now - last.shownAt < 4.0)
        last.label = -1;
    if (last.label == 0)
        return;
    auto added = false;
    for (size_t i = 0; i + 1 < shown.size(); ++i)
    {
        const auto& other = shown[i];
        if (last.label == 1 && other.label == -1)
            taste.addWin (last.features, other.features);
        else if (last.label == -1 && other.label >= 1)
            taste.addWin (other.features, last.features);
        else
            continue;
        added = true;
    }
    if (added)
        taste.save (tasteFile());
}


void ZeroEngineProcessor::heartCurrent()
{
    if (generating) // a pick reads the hearts on its worker; it lands in a moment
        return;
    loadTasteAndHearts();
    if (currentHeart >= 0) // un-heart this sound
    {
        if (currentHeart < (int) hearts.size())
            hearts.erase (hearts.begin() + currentHeart);
        currentHeart = -1;
    }
    else
    {
        Heart heart;
        heart.genre = appliedGenre;
        heart.type = appliedType;
        heart.state = state.copyState();
        heart.name = randomName();
        heart.profile = readProfile (state, heart.genre, heart.type); // with any tweaks
        if (showingLatest && ! shown.empty())
        {
            // A Generate result: it beats the results shown before it that
            // weren't themselves liked.
            auto& latest = shown.back();
            heart.features = latest.features;
            latest.label = 2;
            for (size_t i = shown.size() - 1, beaten = 0; i-- > 0 && beaten < 6;)
                if (shown[i].label < 1)
                {
                    taste.addWin (latest.features, shown[i].features);
                    ++beaten;
                }
            taste.save (tasteFile());
        }
        else
        {
            if (checkers.empty())
            {
                checkers.push_back (std::make_unique<ZeroEngineProcessor>());
                checkers.back()->stopProfileSync();
            }
            heart.features = TasteModel::featuresOf (measureProfile (*checkers[0], heart.genre, heart.type, heart.profile), heart.type);
        }
        hearts.push_back (heart);
        constexpr size_t kept = 60;
        if (hearts.size() > kept)
            hearts.erase (hearts.begin());
        currentHeart = (int) hearts.size() - 1;
    }
    saveHearts();
}

void ZeroEngineProcessor::saveHearts() const
{
    juce::XmlElement xml ("Hearts");
    for (const auto& h : hearts)
    {
        auto* e = xml.createNewChildElement ("Heart");
        e->setAttribute ("name", h.name);
        e->setAttribute ("genre", h.genre);
        e->setAttribute ("type", h.type);
        juce::StringArray values;
        for (const auto v : h.features)
            values.add (juce::String (v, 3));
        e->setAttribute ("features", values.joinIntoString (","));
        if (auto params = h.state.createXml())
            e->addChildElement (params.release());
    }
    heartsFile().getParentDirectory().createDirectory();
    xml.writeTo (heartsFile());
}

juce::StringArray ZeroEngineProcessor::getRecentLikes()
{
    loadTasteAndHearts();
    juce::StringArray names;
    for (auto h = hearts.rbegin(); h != hearts.rend() && names.size() < 10; ++h)
        names.add (h->name);
    return names;
}

bool ZeroEngineProcessor::recallLike (const juce::String& name)
{
    loadTasteAndHearts();
    for (size_t i = 0; i < hearts.size(); ++i)
        if (hearts[i].name == name && hearts[i].state.isValid())
        {
            restoreTree (hearts[i].state);
            currentHeart = (int) i; // after restoreTree, which clears it
            return true;
        }
    return false;
}

juce::String ZeroEngineProcessor::randomName() const
{
    // Two words, one of each list, at random; a number after if every try is taken.
    static const juce::StringArray first { "Neon", "Chrome", "Velvet", "Static", "Hollow", "Molten", "Glass", "Solar", "Feral", "Ghost",
                                           "Iron", "Liquid", "Midnight", "Electric", "Frozen", "Golden", "Acid", "Silent", "Burning", "Cosmic",
                                           "Rogue", "Crystal", "Shadow", "Toxic", "Hyper", "Lunar", "Savage", "Digital", "Wild", "Phantom" };
    static const juce::StringArray second { "Viper", "Tide", "Engine", "Bloom", "Pulse", "Signal", "Comet", "Ember", "Circuit", "Storm",
                                            "Prism", "Echo", "Drift", "Monolith", "Halo", "Serpent", "Vortex", "Orbit", "Lantern", "Rift",
                                            "Wolf", "Mirage", "Spire", "Nova", "Reactor", "Canyon", "Siren", "Glacier", "Furnace", "Horizon" };
    auto taken = [this] (const juce::String& name)
    {
        if (favouritesFolder.getChildFile (name + ".xml").exists())
            return true;
        for (const auto& h : hearts)
            if (h.name == name)
                return true;
        return false;
    };
    juce::Random random;
    juce::String name;
    for (int attempt = 0; attempt < 50; ++attempt)
    {
        name = first[random.nextInt (first.size())] + " " + second[random.nextInt (second.size())];
        if (attempt == 49)
            name << " " << juce::String (random.nextInt ({ 2, 1000 }));
        if (! taken (name))
            break;
    }
    return name;
}


bool ZeroEngineProcessor::stepHistory (int direction)
{
    const auto target = historyPos + direction;
    if (target < 0 || target >= (int) history.size())
        return false;

    historyPos = target;
    restoreTree (history[(size_t) historyPos]);
    return true;
}

ZeroEngineProcessor::SoundCheck ZeroEngineProcessor::measureProfile (ZeroEngineProcessor& engine, int genre, int type, const Profile& profile, int transpose)
{
    // Three quarters of a second of a note (or a triad for chords and pads)
    // through a private engine, light (no spectral stage or oversampling: the
    // check needs the shape, not the last detail), then its SoundTraits.
    constexpr double rate = 48000.0;
    constexpr int block = 512, length = 72 * block; // 0.77 s

    engine.setPlayConfigDetails (0, 2, rate, block);
    engine.prepareToPlay (rate, block); // fresh voices and tails for every candidate
    engine.setLightRender (measureLight);
    // Measure the tone, not the fade-in: slow attacks (pads, FX) would otherwise
    // be compared part-way up their swell.
    auto heard = profile;
    heard.ampAttack = juce::jmin (heard.ampAttack, 0.01f);
    heard.modAttack = juce::jmin (heard.modAttack, 0.01f);
    engine.loadProfile (genre, type, heard);

    juce::AudioBuffer<float> out (2, length), chunk (2, block);
    juce::MidiBuffer midi;
    const auto root = (type == 0 ? 36 : type == 4 || type == 1 ? 72 : 60) + transpose; // bass C2, lead/pluck C5, else C4
    const auto chord = type == 2 || type == 3;
    constexpr int intervals[] { 0, 4, 7 };

    for (int pos = 0; pos < length; pos += block)
    {
        midi.clear();
        if (pos == 0)
            for (int i = 0; i < (chord ? 3 : 1); ++i)
                midi.addEvent (juce::MidiMessage::noteOn (1, root + intervals[i], (juce::uint8) 110), 0);
        chunk.clear();
        engine.processBlock (chunk, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, chunk, ch, 0, block);
    }
    // Clarity on the notes played (and the sub's, an octave or two down, below the
    // 80 Hz the measure starts at: no need to list it).
    std::vector<float> fundamentals;
    for (int i = 0; i < (chord ? 3 : 1); ++i)
        fundamentals.push_back ((float) juce::MidiMessage::getMidiNoteInHertz (root + intervals[i]));
    auto traits = SoundTraits::of (out, rate);
    traits.clarity = SoundTraits::clarityOf (out, rate, fundamentals);
    SoundTraits::measureTuning (out, rate, fundamentals.front(), traits);
    return traits;
}

float ZeroEngineProcessor::strayFrom (const SoundTraits& target, const SoundTraits& m, int type, bool free)
{
    // How far a sound strays from the genre/type's character (1 at the edge of
    // "still the same kind of sound"), or a huge value when it is near silent or
    // fails a tuning gate. In any genre (free) the bounds are looser (twice the
    // owner's spread, a wider brightness range) and loudness is left to the
    // profile's own trim.
    const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];
    if (free ? m.levelDb < -45.0f : m.levelDb < target.levelDb - 20.0f)
        return std::numeric_limits<float>::max();
    const auto loudness = free ? 0.0f : std::abs (m.levelDb - target.levelDb) / 6.0f;
    const auto brightness = std::abs (std::log2 (juce::jmax (20.0f, m.centroidHz) / juce::jmax (20.0f, target.centroidHz))) / (free ? 1.3f : 0.8f);
    const auto bassShare = std::abs (m.bassPct - target.bassPct) / (free ? juce::jmax (20.0f, 2.0f * guide.spread[3]) : 20.0f);
    const auto widthOff = std::abs (m.sidePct - target.sidePct) / (free ? juce::jmax (20.0f, 2.0f * guide.spread[1]) : 20.0f);
    // Tuning gates (not FX, which may be anything): the pitch more than 15
    // cents from where the designed sound sits, more wobble than it has (by
    // 12 cents; 25 in any genre, where a supersaw may meet a plain lead), or
    // a noisier spectrum: out of tune, smeared or hiss, whatever it scores.
    if (type != 5 && (std::abs (m.pitchOffsetCents - target.pitchOffsetCents) > 15.0f
                      || m.pitchSpreadCents > target.pitchSpreadCents + (free ? 25.0f : 12.0f)
                      || m.flatness > target.flatness + 0.05f))
        return std::numeric_limits<float>::max();
    return juce::jmax (juce::jmax (loudness, brightness), juce::jmax (bassShare, widthOff));
}

float ZeroEngineProcessor::sizeAndInterest (const SoundTraits& target, const SoundTraits& m, int type)
{
    // Each trait's gain over the target (the designed sound, or the owner's
    // sounds in any genre), in spreads of the owner's sounds of this type,
    // capped at 1.5 (no reward for extremes) and -2. Size: wider, more body,
    // denser; interest: more movement (half weight: wobble and beating earn it
    // too easily) and more partials.
    const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];
    const auto a = m.asArray(), d = target.asArray();
    auto gain = [&] (int k, bool lowerIsMore)
    {
        const auto z = (a[(size_t) k] - d[(size_t) k]) / juce::jmax (1.0e-3f, guide.spread[(size_t) k]);
        return juce::jlimit (-2.0f, 1.5f, lowerIsMore ? -z : z);
    };
    enum { level, side, centroid, bass, body, spread, crest, movement, richness };
    const auto big = gain (side, false) + 0.5f * gain (body, false) + 0.5f * gain (crest, true);
    const auto interest = 0.5f * gain (movement, false) + gain (richness, false);
    return big + interest;
}

float ZeroEngineProcessor::moodScore (const SoundTraits& target, const SoundTraits& m, int type)
{
    // Warm: darker than the target (octaves), more body, partials; clean: the
    // energy on the notes' harmonics and no noise; width near the target's
    // (neither narrow nor smeared wide); calm: movement past half a spread over
    // the target costs. Each part capped as in sizeAndInterest.
    const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];
    const auto a = m.asArray(), d = target.asArray();
    auto z = [&] (int k) { return (a[(size_t) k] - d[(size_t) k]) / juce::jmax (1.0e-3f, guide.spread[(size_t) k]); };
    auto capped = [] (float v) { return juce::jlimit (-2.0f, 1.5f, v); };
    enum { level, side, centroid, bass, body, spread, crest, movement, richness };
    const auto warmth = capped (-std::log2 (juce::jmax (20.0f, m.centroidHz) / juce::jmax (20.0f, target.centroidHz)) / 0.8f);
    // Harsh: more than half an octave brighter than the target costs on top.
    const auto harsh = juce::jmin (3.0f, 1.5f * juce::jmax (0.0f, std::log2 (juce::jmax (20.0f, m.centroidHz) / juce::jmax (20.0f, target.centroidHz)) - 0.5f));
    const auto clean = capped (4.0f * (m.clarity - 0.85f)) - capped (20.0f * m.flatness);
    const auto widthNear = -0.5f * std::abs (capped (z (side)));
    const auto calm = -capped (juce::jmax (0.0f, z (movement) - 0.5f));
    return warmth - harsh + 0.5f * capped (z (body)) + 0.5f * capped (z (richness)) + clean + widthNear + calm;
}

std::pair<Profile, int> ZeroEngineProcessor::pickGeneratedProfile (int genre, int type, juce::Random& random, int mood)
{
    // Render-and-check, then pick for size and interest:
    //  1. Many variations, each measured (SoundTraits) on private engines in parallel.
    //  2. Keep those still in the genre/type's character: loudness, brightness,
    //     bass and width near the designed profile's (each term 1 at the edge).
    //  3. Score the keepers, each trait on its own as a soft nudge (the owner's
    //     sounds are a guide for parts, never a whole to copy): wider, more body,
    //     denser, more movement and more partials than the designed sound score
    //     higher, measured in spreads of the owner's sounds of this type and
    //     capped at 1.5 (no reward for going to extremes); plus novelty, the
    //     distance from the last few results, so clicks keep surprising.
    //  4. Choose at random, weighted towards the best scores (softmax), so
    //     results stay varied. If none fit, the closest one wins.
    constexpr int candidates = 24;
    constexpr int engines = 4;
    while ((int) checkers.size() < engines)
    {
        checkers.push_back (std::make_unique<ZeroEngineProcessor>());
        checkers.back()->stopProfileSync();
    }

    // Free of a genre's character: any genre (blends led by one genre) or a mood.
    const auto free = genre == anyGenre || mood >= 0;
    const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];

    // What a keeper must stay near and what it is scored against: the designed
    // profile (never changes while running, so measured once), or in Free, where
    // no genre's character applies, the owner's sounds of this type.
    SoundTraits target;
    if (free)
    {
        const auto& m = guide.median;
        target.sidePct = m[1]; target.centroidHz = m[2]; target.bassPct = m[3]; target.bodyPct = m[4];
        target.spreadOct = m[5]; target.crestDb = m[6]; target.movementDb = m[7]; target.richness = m[8];
    }
    else
    {
        auto& cached = designedChecks[(size_t) (genre * 6 + type)];
        if (! cached.has_value())
            cached = measureProfile (*checkers[0], genre, type, makeProfile (genre, type));
        target = *cached;
    }

    // The variations. In Free each comes from any genre, and more than half are
    // blends of two genres' variations (their searched settings mixed 25-75 %,
    // the leading one's other settings and, half the time, the other's
    // wavetable): sounds no single genre makes. sources: the leading genre.
    std::vector<Profile> profiles;
    std::vector<int> sources;
    // Hearted sounds of this type breed some of the candidates (two per heart,
    // at most 8): a heart's searched settings nudged, a third of the time
    // crossed with another heart's. In Generate they play as the current genre;
    // in any genre, as the heart's own.
    // Detune no further than the designed sound's own (or 0.4) plus 0.1: past that
    // the unison beats against itself and reads as out of tune, not as width.
    auto capDetune = [&] (Profile p, int source)
    {
        p.detune = juce::jmin (p.detune, juce::jmax (makeProfile (source, type).detune, 0.4f) + 0.1f);
        return p;
    };
    // Any genre: one lead genre per click, at random but not one of the last
    // two mini clicks' (left to itself the judge favours the genres whose
    // sounds score biggest, trance leads and dubstep basses, click after click).
    auto leadGenre = genre;
    if (free)
    {
        std::vector<int> allowed;
        for (int g = 0; g < anyGenre; ++g)
            if (std::find (recentAnyGenres.begin(), recentAnyGenres.end(), g) == recentAnyGenres.end())
                allowed.push_back (g);
        leadGenre = allowed[(size_t) random.nextInt ((int) allowed.size())];
    }

    std::vector<const Heart*> parents;
    for (const auto& h : hearts)
        if (h.type == type)
            parents.push_back (&h);
    const auto offspring = juce::jmin (8, 2 * (int) parents.size());
    for (int i = 0; i < offspring; ++i)
    {
        const auto& parent = *parents[(size_t) random.nextInt ((int) parents.size())];
        auto units = ReferenceMatch::unitsOf (parent.profile);
        if (parents.size() > 1 && random.nextFloat() < 0.33f)
        {
            const auto& other = *parents[(size_t) random.nextInt ((int) parents.size())];
            const auto otherUnits = ReferenceMatch::unitsOf (other.profile);
            const auto mix = 0.3f + 0.4f * random.nextFloat();
            for (size_t k = 0; k < units.size(); ++k)
                units[k] += mix * (otherUnits[k] - units[k]);
        }
        for (int n = 0; n < 4; ++n)
        {
            auto& u = units[(size_t) random.nextInt (ReferenceMatch::numSearched)];
            u = juce::jlimit (0.0f, 1.0f, u + 0.16f * (random.nextFloat() * 2.0f - 1.0f));
        }
        profiles.push_back (capDetune (ReferenceMatch::withUnitsOf (parent.profile, units), free ? parent.genre : genre));
        sources.push_back (free ? parent.genre : genre);
    }

    const auto drawn = useJudge && GenerateNet::builtIn().ready() ? drawWithJudge : candidates; // more when the judge can screen them
    for (int i = offspring; i < drawn; ++i)
    {
        if (mood >= 0) // a mood's recipe
        {
            int base = 0;
            profiles.push_back (capDetune (Moods::make (mood, random, base), base));
            sources.push_back (base);
            continue;
        }
        if (! free)
        {
            profiles.push_back (capDetune (generateProfile (genre, type, random), genre));
            sources.push_back (genre);
            continue;
        }
        // The click's lead genre (chosen once, below) leads every candidate;
        // more than half blend in another genre as the smaller part (25-45 %),
        // with its wavetable a third of the time.
        auto p = generateProfile (leadGenre, type, random);
        const auto source = leadGenre;
        if (random.nextFloat() < 0.6f)
        {
            const auto b = (leadGenre + 1 + random.nextInt (anyGenre - 1)) % anyGenre;
            const auto other = generateProfile (b, type, random);
            const auto mix = 0.25f + 0.2f * random.nextFloat();
            auto units = ReferenceMatch::unitsOf (p);
            const auto otherUnits = ReferenceMatch::unitsOf (other);
            for (size_t k = 0; k < units.size(); ++k)
                units[k] += mix * (otherUnits[k] - units[k]);
            const auto table = random.nextFloat() < 0.33f ? other.waveTable : p.waveTable;
            p = ReferenceMatch::withUnitsOf (p, units);
            p.waveTable = table;
        }
        profiles.push_back (capDetune (p, source));
        sources.push_back (source);
    }

    auto spreadOf = [&] (int k) { return juce::jmax (1.0e-3f, guide.spread[(size_t) k]); };
    // A trait's gain over the designed sound, in the owner's spreads, capped.
    auto gain = [&] (int k, float value, float designed, bool lowerIsMore)
    {
        const auto z = (value - designed) / spreadOf (k);
        return juce::jlimit (-2.0f, 1.5f, lowerIsMore ? -z : z);
    };
    enum { level, side, centroid, bass, body, spread, crest, movement, richness };

    // How far a sound strays from the genre/type's character (1 at the edge of
    // "still the same kind of sound"), or a large value when it is near silent.
    // In Free the bounds are looser (twice the owner's spread, a wider brightness
    // range) and loudness is left to the profile's own trim.
    auto strayOf = [&] (const SoundTraits& m) { return strayFrom (target, m, type, free); };
    // Size + interest + novelty (see above).
    // The owner's taste (TasteModel), once it has a few judgments: one draw of
    // it per click (Thompson sampling: unsure early, so clicks explore), capped,
    // at full strength from 40 judgments; one click in five ignores it, so
    // results never settle into one style.
    std::optional<TasteModel::Features> tasteWeights;
    auto tasteStrength = 0.0f;
    if (taste.judgments() >= 4 && random.nextFloat() >= 0.2f)
    {
        tasteWeights = taste.sampleWeights (random);
        tasteStrength = juce::jmin (1.0f, (float) taste.judgments() / 40.0f);
    }
    auto tasteOf = [&] (const SoundTraits& m)
    {
        if (! tasteWeights.has_value())
            return 0.0f;
        const auto f = TasteModel::featuresOf (m, type);
        float dot = 0.0f;
        for (int k = 0; k < TasteModel::numFeatures; ++k)
            dot += (*tasteWeights)[(size_t) k] * f[(size_t) k];
        return tasteStrength * juce::jlimit (-1.5f, 1.5f, dot);
    };

    auto scoreOf = [&] (const SoundTraits& m, bool print = false)
    {
        const auto a = m.asArray(), d = target.asArray();
        const auto bigAndInteresting = mood >= 0 ? moodScore (target, m, type) : sizeAndInterest (target, m, type);
        auto novelty = 3.0f;
        for (const auto& r : recentGenerated)
        {
            const auto b = r.asArray();
            float sum = 0.0f;
            for (int k = 0; k < SoundTraits::count; ++k)
                if (k != centroid) // brightness in octaves, the rest in spreads
                    sum += juce::square ((a[(size_t) k] - b[(size_t) k]) / spreadOf (k));
            sum += juce::square (std::log2 (juce::jmax (20.0f, a[centroid]) / juce::jmax (20.0f, b[centroid])) / 0.8f);
            novelty = juce::jmin (novelty, std::sqrt (sum));
        }
        if (print && debugGenerate)
            std::printf ("    size+interest %+5.2f  novelty %4.2f  side %5.1f (designed %5.1f)  move %4.1f (%4.1f)  rich %.2f (%.2f)  crest %4.1f (%4.1f)  body %4.1f (%4.1f)\n",
                         bigAndInteresting, novelty, a[side], d[side], a[movement], d[movement], a[richness], d[richness], a[crest], d[crest], a[body], d[body]);
        return bigAndInteresting + 0.7f * novelty + 1.5f * tasteOf (m);
    };

    // Screen (the neural judge, GenerateNet, when trained): it predicts what each
    // of the drawn (drawWithJudge, 400) would measure, from its settings alone; the 20 it rates
    // best by the same fit and score (and 4 more at random, so a judge's blind
    // spots still get heard) go on to be rendered, with any bred from hearts.
    // About 2 ms; on the judge's tests its picks scored far above random ones.
    if (const auto& judge = GenerateNet::builtIn(); useJudge && judge.ready() && (int) profiles.size() > candidates)
    {
        std::vector<std::pair<float, int>> ranked;
        for (int i = offspring; i < (int) profiles.size(); ++i)
        {
            const auto predicted = GenerateNet::traitsOf (judge.run (GenerateNet::featuresOf (profiles[(size_t) i], sources[(size_t) i], type)), type);
            ranked.push_back ({ (strayOf (predicted) <= 1.0f ? 0.0f : -100.0f) + scoreOf (predicted), i });
        }
        std::sort (ranked.begin(), ranked.end(), [] (const auto& a, const auto& b) { return a.first > b.first; });
        const auto picked = candidates - offspring, explore = juce::jmin (4, picked);
        std::vector<int> keep;
        for (int i = 0; i < offspring; ++i)
            keep.push_back (i);
        for (int i = 0; i < picked - explore; ++i)
            keep.push_back (ranked[(size_t) i].second);
        for (int i = 0; i < explore; ++i)
            keep.push_back (ranked[(size_t) (picked - explore + random.nextInt ((int) ranked.size() - (picked - explore)))].second);
        std::vector<Profile> screened;
        std::vector<int> screenedSources;
        for (const auto i : keep)
        {
            screened.push_back (profiles[(size_t) i]);
            screenedSources.push_back (sources[(size_t) i]);
        }
        profiles = std::move (screened);
        sources = std::move (screenedSources);
    }

    std::vector<SoundTraits> traits ((size_t) candidates);
    {
        std::vector<std::thread> pool;
        for (int e = 0; e < engines; ++e)
            pool.emplace_back ([&, e]
            {
                for (int i = e; i < candidates; i += engines)
                    traits[(size_t) i] = measureProfile (*checkers[(size_t) e], sources[(size_t) i], type, profiles[(size_t) i]);
            });
        for (auto& t : pool)
            t.join();
    }

    struct Keeper { int index; float score; };
    std::vector<Keeper> keepers;
    int closest = -1;
    auto closestDistance = std::numeric_limits<float>::max();
    for (int i = 0; i < candidates; ++i)
    {
        const auto stray = strayOf (traits[(size_t) i]);
        if (stray < closestDistance)
        {
            closest = i;
            closestDistance = stray;
        }
        if (stray <= 1.0f)
            keepers.push_back ({ i, scoreOf (traits[(size_t) i]) });
    }

    lastGenerateFits = (int) keepers.size();
    if (keepers.empty())
        return closest >= 0 ? std::pair { profiles[(size_t) closest], sources[(size_t) closest] } : std::pair { makeProfile (sources[0], type), sources[0] };

    // Softmax at temperature 0.35: the best are likely, not certain.
    auto chosen = keepers.back().index;
    {
        const auto top = std::max_element (keepers.begin(), keepers.end(), [] (const Keeper& x, const Keeper& y) { return x.score < y.score; })->score;
        std::vector<double> weights;
        double sum = 0.0;
        for (const auto& k : keepers)
            sum += weights.emplace_back (std::exp ((k.score - top) / 0.35));
        auto pick = random.nextDouble() * sum;
        for (size_t k = 0; k < keepers.size(); ++k)
            if ((pick -= weights[k]) <= 0.0)
            {
                chosen = keepers[k].index;
                break;
            }
    }
    auto best = profiles[(size_t) chosen];
    auto source = sources[(size_t) chosen];
    auto bestTraits = traits[(size_t) chosen];
    auto bestScore = scoreOf (bestTraits, true);

    // Push: twice, one small nudge per engine (three of the settings Reference
    // Match searches, moved a little); the best nudge that scores higher and
    // still fits replaces it. Picking alone only chooses among what the
    // variations happen to be; this walks the chosen one towards bigger.
    for (int round = 0; round < 2; ++round)
    {
        std::vector<Profile> nudged ((size_t) engines);
        for (auto& p : nudged)
        {
            auto units = ReferenceMatch::unitsOf (best);
            for (int n = 0; n < 3; ++n)
            {
                auto& u = units[(size_t) random.nextInt (ReferenceMatch::numSearched)];
                u = juce::jlimit (0.0f, 1.0f, u + 0.15f * (random.nextFloat() * 2.0f - 1.0f));
            }
            p = capDetune (ReferenceMatch::withUnitsOf (best, units), source);
        }
        std::vector<SoundTraits> measured ((size_t) engines);
        {
            std::vector<std::thread> pool;
            for (int e = 0; e < engines; ++e)
                pool.emplace_back ([&, e] { measured[(size_t) e] = measureProfile (*checkers[(size_t) e], source, type, nudged[(size_t) e]); });
            for (auto& t : pool)
                t.join();
        }
        auto improved = -1;
        for (int e = 0; e < engines; ++e)
            if (strayOf (measured[(size_t) e]) <= 1.0f)
                if (const auto s = scoreOf (measured[(size_t) e]); s > bestScore + 0.05f)
                {
                    bestScore = s;
                    improved = e;
                }
        if (improved >= 0)
        {
            best = nudged[(size_t) improved];
            bestTraits = measured[(size_t) improved];
            if (debugGenerate)
                std::printf ("    pushed to %.2f\n", bestScore);
        }
    }

    // Octave check: a sound is played across the keyboard, not on one note. The
    // pick again an octave down and up (basses: a fifth and an octave up, as an
    // octave below C2 is mostly sub), each against itself at the test note:
    // still sounding, as much in tune, no more wobble or noise. If it falls
    // apart, the next best keepers (two at most) in its place.
    auto holdsUp = [&] (const Profile& p, int src, const SoundTraits& atRoot)
    {
        const int shifts[2] { type == 0 ? 7 : -12, 12 };
        SoundTraits heard[2];
        std::thread other ([&] { heard[1] = measureProfile (*checkers[1], src, type, p, shifts[1]); });
        heard[0] = measureProfile (*checkers[0], src, type, p, shifts[0]);
        other.join();
        for (const auto& m : heard)
            if (m.levelDb < atRoot.levelDb - 20.0f
                || (type != 5 && (std::abs (m.pitchOffsetCents - atRoot.pitchOffsetCents) > 15.0f
                                  || m.pitchSpreadCents > atRoot.pitchSpreadCents + 12.0f
                                  || m.flatness > atRoot.flatness + 0.05f)))
                return false;
        return true;
    };
    if (! holdsUp (best, source, bestTraits))
    {
        auto ranked = keepers;
        std::sort (ranked.begin(), ranked.end(), [] (const Keeper& x, const Keeper& y) { return x.score > y.score; });
        auto tries = 0;
        for (const auto& k : ranked)
        {
            if (k.index == chosen)
                continue;
            if (tries++ >= 2)
                break;
            if (holdsUp (profiles[(size_t) k.index], sources[(size_t) k.index], traits[(size_t) k.index]))
            {
                best = profiles[(size_t) k.index];
                source = sources[(size_t) k.index];
                bestTraits = traits[(size_t) k.index];
                if (debugGenerate)
                    std::printf ("    octave check: replaced by candidate %d\n", k.index);
                break;
            }
        }
        lastOctaveFailures++;
    }

    recentGenerated.push_back (bestTraits);
    if (recentGenerated.size() > 6)
        recentGenerated.erase (recentGenerated.begin());
    lastPickTraits = bestTraits;
    if (free)
    {
        recentAnyGenres.push_back (source);
        if (recentAnyGenres.size() > 2)
            recentAnyGenres.erase (recentAnyGenres.begin());
    }
    return { best, source };
}



void ZeroEngineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    synthBuffer.setSize (3, juce::jmax (samplesPerBlock, 4096)); // L, R, clean sub
    finishing.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    arp.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (finishing.getLatencySamples());
    keyboardState.reset();
    for (auto& word : soundingNotes)
        word.store (0, std::memory_order_relaxed);
}

bool ZeroEngineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ZeroEngineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    // Controllers: the mod wheel (CC 1) rides on top of Motion; MIDI learn takes
    // the next CC for the macro waiting to learn; learned CCs move their macros.
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (! message.isController())
            continue;

        const auto cc = message.getControllerNumber();
        const auto value = (float) message.getControllerValue() / 127.0f;
        if (cc == 1)
            macroMap.setModWheel (value);

        if (const auto learning = learningMacro.exchange (-1); learning >= 0)
            macroCC[(size_t) learning] = cc;

        for (size_t m = 0; m < macroCC.size(); ++m)
            if (macroCC[m].load() == cc)
                macroParams[m]->setValueNotifyingHost (value);
    }

    // Host tempo for the delay and synced LFOs; the fallback (120 BPM unless set) when
    // there is none (standalone, offline renders).
    auto bpm = (double) fallbackBpm.load();
    if (auto* host = getPlayHead())
        if (const auto position = host->getPosition())
            if (const auto hostTempo = position->getBpm())
                bpm = juce::jlimit (40.0, 300.0, *hostTempo);
    hostBpm.store ((float) bpm);
    livePosition.store (-1.0f, std::memory_order_relaxed); // voices that play this block overwrite it

    // Arpeggiator: the keys in, the pattern out (the input itself when it's off).
    Arpeggiator::Settings arpSettings;
    arpSettings.on = arpParams[0]->load() > 0.5f;
    arpSettings.latch = arpLatchParam->load() > 0.5f;
    arpSettings.mode = (int) arpParams[1]->load();
    arpSettings.beatsPerStep = arpRateBeats[juce::jlimit (0, (int) std::size (arpRateBeats) - 1, (int) arpParams[2]->load())];
    arpSettings.octaves = (int) arpParams[3]->load();
    arpSettings.gate = arpParams[4]->load();
    arpSettings.bpm = bpm;
    arpSettings.length = (int) seqLengthParam->load();
    for (int step = 0; step < Arpeggiator::maxSteps; ++step)
    {
        arpSettings.stepOn[(size_t) step] = seqParams[0][step]->load() > 0.5f;
        arpSettings.stepPitch[(size_t) step] = juce::roundToInt (seqParams[1][step]->load());
        arpSettings.stepAccent[(size_t) step] = seqParams[2][step]->load() > 0.5f;
    }
    const auto& notes = arp.process (midi, numSamples, arpSettings);
    for (const auto metadata : notes)
    {
        const auto message = metadata.getMessage();
        const auto bit = 1u << (message.getNoteNumber() & 31);
        auto& word = soundingNotes[(size_t) (message.getNoteNumber() >> 5) & 3];
        if (message.isNoteOn())
            word.fetch_or (bit, std::memory_order_relaxed);
        else if (message.isNoteOff())
            word.fetch_and (~bit, std::memory_order_relaxed);
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            for (auto& w : soundingNotes)
                w.store (0, std::memory_order_relaxed);
    }

    // Sub Split: voices write their sub to a third channel that skips the
    // chain. A block bigger than prepared falls back to the mixed path rather
    // than allocating here.
    const float* sub = nullptr;
    if (subSplit.load() > 0.5f && numSamples <= synthBuffer.getNumSamples())
    {
        juce::AudioBuffer<float> view (synthBuffer.getArrayOfWritePointers(), 3, numSamples);
        view.clear();
        synth.renderNextBlock (view, notes, 0, numSamples);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, view, juce::jmin (ch, 1), 0, numSamples);
        sub = view.getReadPointer (2);
    }
    else
    {
        synth.renderNextBlock (buffer, notes, 0, numSamples);
    }

    FinishingSettings settings;
    settings.grit     = grit.load();
    settings.ottDepth = juce::jlimit (0.0f, 1.0f, ottDepth.load() + macroMap.offset (MacroTarget::ottDepth));
    settings.spectral = spectral.load();
    settings.tiltDb   = juce::jlimit (-6.0f, 6.0f, tilt.load() + macroMap.offset (MacroTarget::tilt));
    settings.air      = air.load();
    settings.monoFreq = monoFreq.load();
    settings.midHighPassHz = juce::jlimit (20.0f, 1000.0f, midHighPass.load() * std::exp2 (macroMap.offset (MacroTarget::midHighPass)));

    // With no sub (leads, chords, pads, plucks) nothing replaces what the
    // high-pass removes, so keep it below the lowest note's fundamental: a low
    // hoover or brass chord keeps its root. Basses keep the fixed split, since
    // their clean sub carries the fundamental.
    if (subOn.load() < 0.5f)
    {
        int lowest = 128;
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (auto* voice = synth.getVoice (i); voice->isVoiceActive())
                lowest = juce::jmin (lowest, voice->getCurrentlyPlayingNote());

        // About 3/4 octave under the fundamental, where the 12 dB slope costs it
        // under 0.5 dB. Glide there over ~10 ms; hold the last value in silence.
        if (lowest < 128)
        {
            const auto target = (float) juce::MidiMessage::getMidiNoteInHertz (lowest) * 0.6f;
            const auto glide = 1.0f - std::exp (-(float) numSamples / (0.01f * (float) getSampleRate()));
            noteHighPassHz *= std::exp2 (glide * std::log2 (target / noteHighPassHz));
        }
        settings.midHighPassHz = juce::jlimit (20.0f, settings.midHighPassHz, noteHighPassHz);
    }
    settings.sideGain = stereoWidth.load() * (0.5f + width.load()); // Width also sets unison spread in the voice
    settings.outputDb = output.load() + levelTrim.load();

    const auto beats = delayTimeBeats[juce::jlimit (0, (int) std::size (delayTimeBeats) - 1, (int) delayTime.load())];
    settings.delaySeconds  = (float) (beats * 60.0 / bpm);
    settings.delayMix      = juce::jlimit (0.0f, 1.0f, delayMix.load() + macroMap.offset (MacroTarget::delayMix));
    settings.delayFeedback = delayFeedback.load();
    settings.reverbMix     = juce::jlimit (0.0f, 1.0f, reverbMix.load() + macroMap.offset (MacroTarget::reverbMix));
    settings.reverbSize    = reverbSize.load();

    finishing.process (buffer, settings, sub);

    // Display tap.
    auto write = scopeWrite.load (std::memory_order_relaxed);
    const auto* left = buffer.getReadPointer (0);
    const auto* right = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
    for (int i = 0; i < numSamples; ++i)
    {
        scope[(size_t) write] = 0.5f * (left[i] + right[i]);
        write = (write + 1) % scopeSize;
    }
    scopeWrite.store (write, std::memory_order_release);
}

void ZeroEngineProcessor::copyScope (float* dest, int numSamples) const noexcept
{
    numSamples = juce::jmin (numSamples, scopeSize);
    const auto end = scopeWrite.load (std::memory_order_acquire);
    for (int i = 0; i < numSamples; ++i)
        dest[i] = scope[(size_t) ((end - numSamples + i + scopeSize) % scopeSize)];
}

juce::AudioProcessorEditor* ZeroEngineProcessor::createEditor()
{
    return new ZeroEngineEditor (*this);
}

void ZeroEngineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
    {
        // MIDI learn belongs to the project (your controller), not to a sound, so it
        // is saved here and not in the state tree that favourites copy.
        juce::StringArray ccs;
        for (const auto& cc : macroCC)
            ccs.add (juce::String (cc.load()));
        xml->setAttribute ("macroCCs", ccs.joinIntoString (","));
        if (sampleHarmonics.has_value()) // the Sample wavetable: frame by frame, harmonic by harmonic
        {
            juce::StringArray values;
            for (const auto& frame : *sampleHarmonics)
                for (const auto a : frame)
                    values.add (juce::String (a, 5));
            xml->setAttribute ("sampleTable", values.joinIntoString (","));
        }
        copyXmlToBinary (*xml, destData);
    }
}

void ZeroEngineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        restoreState (*xml);
        const auto ccs = juce::StringArray::fromTokens (xml->getStringAttribute ("macroCCs"), ",", {});
        for (size_t m = 0; m < macroCC.size(); ++m)
            macroCC[m] = m < (size_t) ccs.size() ? ccs[(int) m].getIntValue() : -1;
        const auto values = juce::StringArray::fromTokens (xml->getStringAttribute ("sampleTable"), ",", {});
        if (values.size() == WavetableBank::numFrames * WavetableBank::numSampleHarmonics)
        {
            WavetableBank::Harmonics harmonics {};
            for (int i = 0; i < values.size(); ++i)
                harmonics[(size_t) (i / WavetableBank::numSampleHarmonics)][(size_t) (i % WavetableBank::numSampleHarmonics)] = values[i].getFloatValue();
            setSampleHarmonics (harmonics);
        }
    }
}

void ZeroEngineProcessor::generateArp (int type, juce::Random& random)
{
    // The arp is part of the roll. Switched on by hand, it stays on and gets new
    // settings; otherwise Generate decides, more often for types that suit an
    // arp (plucks about half the time, pads rarely), and an arp it switched on
    // may go off again on the next click. On: mode, rate, octaves, gate and a
    // new step pattern, each from choices that suit the type.
    auto set = [this] (const juce::String& id, float value)
    {
        auto* param = state.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    const auto byHand = arpParams[0]->load() > 0.5f && ! arpFromGenerate;
    // Only for types an arp suits on its own (never chords, pads or FX, where a
    // held chord under a moving pattern turns to mush).
    constexpr float chance[6] { 0.2f, 0.2f, 0.0f, 0.0f, 0.35f, 0.0f }; // Bass, Lead, Chord, Pad, Pluck, FX
    const auto on = byHand || random.nextFloat() < chance[juce::jlimit (0, 5, type)];
    arpFromGenerate = on && ! byHand;
    set (ParamIDs::arpOn, on ? 1.0f : 0.0f);
    if (! on)
        return;

    auto pick = [&random] (std::initializer_list<std::pair<int, float>> weighted)
    {
        float total = 0.0f;
        for (const auto& [value, weight] : weighted)
            total += weight;
        auto r = random.nextFloat() * total;
        for (const auto& [value, weight] : weighted)
            if ((r -= weight) <= 0.0f)
                return value;
        return weighted.begin()->first;
    };
    // Modes: Up, Down, Up / Down, Random, As Played, Sequence (uses the pattern).
    // No Random mode: it scrambles the notes.
    set (ParamIDs::arpMode, (float) pick ({ { 0, 2.0f }, { 1, 1.0f }, { 2, 2.0f }, { 4, 1.0f }, { 5, 3.0f } }));
    // Rates: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32.
    const auto rate = type == 0               ? pick ({ { 1, 3.0f }, { 3, 3.0f }, { 2, 1.0f } })
                    : type == 2 || type == 3  ? pick ({ { 0, 1.0f }, { 1, 3.0f }, { 3, 2.0f }, { 2, 1.0f } })
                    : type == 5               ? pick ({ { 3, 2.0f }, { 5, 1.0f }, { 4, 1.0f } })
                                              : pick ({ { 3, 4.0f }, { 1, 2.0f }, { 4, 1.0f }, { 5, 0.5f } });
    set (ParamIDs::arpRate, (float) rate);
    set (ParamIDs::arpOctaves, (float) (type == 0 ? 1 : pick ({ { 1, 3.0f }, { 2, 1.0f } })));
    const auto gate = type == 4 ? 0.3f + 0.3f * random.nextFloat()           // plucks: short
                    : type == 3 ? 0.7f + 0.3f * random.nextFloat()           // pads: long
                                : 0.4f + 0.45f * random.nextFloat();
    set (ParamIDs::arpGate, gate);
    // The groove's own rate most of the time; the type's pick otherwise.
    const auto grooveRate = generatePattern (random, appliedGenre);
    if (random.nextFloat() < 0.7f)
        set (ParamIDs::arpRate, (float) juce::jlimit (0, (int) arpRateNames.size() - 1, grooveRate));
}

namespace
{
    // Note steps per genre, for re-rolling a groove's pitch line now and then:
    // mostly the root, then the intervals the style leans on.
    const std::array<std::vector<int>, 12> genrePitches {{
        { 0, 0, 0, 12, 7, 12, 5 },        // EDM
        { 0, 0, 0, -12, 12, 7, 3 },       // Dubstep
        { 0, 0, 0, 0, 12, -12 },          // Techno
        { 0, 0, 0, 12, 7, 10 },           // Tech House
        { 0, 0, 12, 7, 3, -12 },          // Trap
        { 0, 0, 12, 7, 12, 5, 3 },        // Synthwave
        { 0, 0, 0, 12, -12, 7 },          // Drum & Bass
        { 0, 0, 7, 5, 12, 4 },            // Pop
        { 0, 0, 0, 12, 7, 10 },           // House
        { 0, 0, 12, 7, 12, 5, 3 },        // Trance
        { 0, 0, 7, 12, 5, 10 },           // Future Bass
        { 0, 0, 0, 12, -12, 7 },          // Hardstyle
    }};
}

int ZeroEngineProcessor::generatePattern (juce::Random& random, int genre)
{
    // A groove that suits the genre (ArpGrooves.h, about a hundred), with a step
    // or two flipped so no two are alike (the downbeat always plays); its own
    // pitch line, or one time in four the genre's intervals at random. Returns
    // the groove's suggested rate.
    genre = juce::jlimit (0, 11, genre);
    std::vector<int> suits;
    for (int g = 0; g < (int) std::size (ArpGrooves::all); ++g)
        if (ArpGrooves::all[g].genres[(size_t) genre])
            suits.push_back (g);
    const auto& groove = ArpGrooves::all[suits.empty() ? random.nextInt ((int) std::size (ArpGrooves::all))
                                                       : suits[(size_t) random.nextInt ((int) suits.size())]];
    juce::String steps (groove.steps);
    for (int flips = random.nextInt (3); --flips >= 0;)
    {
        const auto at = 1 + random.nextInt (Arpeggiator::maxSteps - 1);
        steps = steps.replaceSection (at, 1, steps[at] == '.' ? "x" : ".");
    }

    auto set = [this] (const juce::String& id, float value)
    {
        auto* param = state.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    const auto reroll = random.nextFloat() < 0.25f;
    const auto& pitches = genrePitches[(size_t) genre];
    for (int step = 0; step < Arpeggiator::maxSteps; ++step)
    {
        const auto c = steps[step];
        set (SeqIDs::on (step), step == 0 || c != '.' ? 1.0f : 0.0f);
        set (SeqIDs::accent (step), c == 'A' ? 1.0f : 0.0f);
        // Only octaves and fifths: they agree with whatever is held; thirds,
        // fourths and sevenths over a held chord or the wrong key clash.
        auto pitch = reroll ? pitches[(size_t) random.nextInt ((int) pitches.size())] : (int) groove.pitches[(size_t) step];
        const auto inOctave = ((pitch % 12) + 12) % 12;
        if (inOctave != 0 && inOctave != 7)
            pitch -= inOctave; // down to the octave below it
        set (SeqIDs::pitch (step), (float) pitch);
    }
    return groove.rate;
}

void ZeroEngineProcessor::setSampleHarmonics (const WavetableBank::Harmonics& harmonics)
{
    sampleTables.push_back (std::make_unique<WavetableBank> (harmonics)); // built here, off the audio thread
    sampleTable.store (sampleTables.back().get(), std::memory_order_release);
    sampleHarmonics = harmonics;
    // A voice reads the bank for one block at most; one replaced three
    // matches ago (each takes seconds) is long free.
    if (sampleTables.size() > 4)
        sampleTables.erase (sampleTables.begin());
}

void ZeroEngineProcessor::startMatch (const juce::File& reference)
{
    if (matcher == nullptr)
    {
        matchEngines = std::make_unique<MatchEngines>();
        matcher = std::make_unique<ReferenceMatch> (matchEngines->get());
    }

    // Searches every genre/type; the current one starts from its designed sound.
    syncProfile();
    const int genre = appliedGenre, type = appliedType;
    matchSummary.clear();
    matcher->start (reference, genre, type, makeProfile (genre, type), hostBpm.load()); // the project's tempo unless the file names one
}

juce::String ZeroEngineProcessor::getMatchStatus() const
{
    if (isMatching())
        return "MATCHING  " + juce::String (juce::roundToInt (100.0f * matcher->getProgress())) + "%";
    return juce::Time::getMillisecondCounterHiRes() < matchSummaryUntil ? matchSummary : juce::String();
}

void ZeroEngineProcessor::checkpoint()
{
    auto now = state.copyState();
    if (! lastKnown.isValid())
    {
        lastKnown = now; // the starting sound; nothing to undo yet
        return;
    }
    if (now.isEquivalentTo (lastKnown))
        return;

    constexpr size_t maxSteps = 100;
    undoStack.push_back (lastKnown);
    if (undoStack.size() > maxSteps)
        undoStack.erase (undoStack.begin());
    redoStack.clear();
    lastKnown = now;
}

bool ZeroEngineProcessor::undo()
{
    checkpoint(); // a change not yet recorded is the step to undo
    if (undoStack.empty())
        return false;

    redoStack.push_back (state.copyState());
    restoreTree (undoStack.back());
    undoStack.pop_back();
    lastKnown = state.copyState();
    return true;
}

bool ZeroEngineProcessor::redo()
{
    checkpoint();
    if (redoStack.empty())
        return false;

    undoStack.push_back (state.copyState());
    restoreTree (redoStack.back());
    redoStack.pop_back();
    lastKnown = state.copyState();
    return true;
}

void ZeroEngineProcessor::adoptCurrentGenreAndType()
{
    // An undone Genre/Type change brings back that sound's values with it; take
    // them as they are rather than letting syncProfile reload the designed profile.
    appliedGenre = (int) state.getRawParameterValue (ParamIDs::genre)->load();
    appliedType = (int) state.getRawParameterValue (ParamIDs::type)->load();
    loadMacroMap (macroMap, makeProfile (appliedGenre, appliedType));
}

bool ZeroEngineProcessor::restoreState (const juce::XmlElement& xml)
{
    if (! xml.hasTagName (state.state.getType()))
        return false;

    restoreTree (juce::ValueTree::fromXml (xml));
    return true;
}

void ZeroEngineProcessor::restoreTree (const juce::ValueTree& tree)
{
    state.replaceState (tree.createCopy());

    adoptCurrentGenreAndType(); // the saved values already include any tweaks; keep them
    showingLatest = false;
    currentHeart = -1; // history, a favourite, a session: not the last Generate result
}

juce::String ZeroEngineProcessor::saveFavourite()
{
    favouritesFolder.createDirectory();
    const auto base = genreNames[appliedGenre] + " " + typeNames[appliedType];

    int number = 1;
    while (favouritesFolder.getChildFile (base + " " + juce::String (number) + ".xml").exists())
        ++number;

    const auto name = base + " " + juce::String (number);
    if (auto xml = state.copyState().createXml())
        if (xml->writeTo (favouritesFolder.getChildFile (name + ".xml")))
            return name;
    return {};
}

juce::String ZeroEngineProcessor::quickSave()
{
    loadTasteAndHearts(); // so a name never repeats a heart's
    favouritesFolder.createDirectory();
    const auto name = randomName();
    if (auto xml = state.copyState().createXml())
        if (xml->writeTo (favouritesFolder.getChildFile (name + ".xml")))
        {
            matchSummary = "SAVED  " + name.toUpperCase();
            matchSummaryUntil = juce::Time::getMillisecondCounterHiRes() + 4000.0;
            return name;
        }
    return {};
}

juce::StringArray ZeroEngineProcessor::getFavourites() const
{
    auto files = favouritesFolder.findChildFiles (juce::File::findFiles, false, "*.xml");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getLastModificationTime() > b.getLastModificationTime(); });

    juce::StringArray names;
    for (const auto& file : files)
        names.add (file.getFileNameWithoutExtension());
    return names;
}

bool ZeroEngineProcessor::deleteFavourite (const juce::String& name)
{
    return favouritesFolder.getChildFile (name + ".xml").deleteFile();
}

juce::String ZeroEngineProcessor::renameFavourite (const juce::String& name, const juce::String& newName)
{
    const auto clean = juce::File::createLegalFileName (newName.trim());
    const auto target = favouritesFolder.getChildFile (clean + ".xml");
    if (clean.isEmpty() || target.exists())
        return {};
    return favouritesFolder.getChildFile (name + ".xml").moveFileTo (target) ? clean : juce::String();
}

bool ZeroEngineProcessor::loadFavourite (const juce::String& name)
{
    if (auto xml = juce::XmlDocument::parse (favouritesFolder.getChildFile (name + ".xml")))
        return restoreState (*xml);
    return false;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZeroEngineProcessor();
}

void ZeroEngineProcessor::resetForRender()
{
    synth.allNotesOff (0, false);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<ZeroVoice*> (synth.getVoice (i)))
            voice->reseed(); // the same start phases every time, as a fresh engine
    finishing.reset();
}

ZeroEngineProcessor::~ZeroEngineProcessor()
{
    alive->store (false);                   // a result still on its way is dropped
    generateWorker.removeAllJobs (true, 10000); // and a running pick finishes first
}
