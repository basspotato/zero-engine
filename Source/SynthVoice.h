#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MacroMap.h"
#include "Envelope.h"
#include "Wavetable.h"

struct ZeroSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

// One note: up to 7 unison wavetable oscillators spread in pitch and stereo,
// a sine sub, an amp envelope, a mod envelope and a per-voice LFO.
class ZeroVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int maxUnison = 7;

    // index seeds the voice's randomness (unison start phases, S&H), so a
    // fresh engine renders identically; phases still vary from note to note.
    ZeroVoice (juce::AudioProcessorValueTreeState& state, const MacroMap& macroMap, const std::atomic<float>& hostBpm,
               const std::atomic<const WavetableBank*>& sampleTable,
               std::atomic<float>& livePosition, int index);
    void reseed() { random.setSeed (seed); ampEnv.reset(); modEnv.reset(); stolen = false; } // back to a fresh engine's state (offline renders)

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newValue) override;
    void controllerMoved (int, int) override {}
    void setCurrentPlaybackSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

    // Mono/legato: move this sounding voice to a new note, sliding over the
    // Glide time. retrigger restarts the envelopes (Mono); Legato doesn't.
    void glideTo (int midiNote, float velocity, bool retrigger, bool jump = false); // jump: no slide, start on the note

private:
    struct Params
    {
        std::atomic<float>* weight;
        std::atomic<float>* punch;
        std::atomic<float>* width;
        std::atomic<float>* motion;
        std::atomic<float>* wavePos;
        std::atomic<float>* unison;
        std::atomic<float>* detune;
        std::atomic<float>* blend;
        std::atomic<float>* ampAttack;
        std::atomic<float>* ampDecay;
        std::atomic<float>* ampSustain;
        std::atomic<float>* ampRelease;
        std::atomic<float>* modAttack;
        std::atomic<float>* modDecay;
        std::atomic<float>* modSustain;
        std::atomic<float>* modRelease;
        std::atomic<float>* modToPos;
        std::atomic<float>* lfoRate;
        std::atomic<float>* lfoShape;
        std::atomic<float>* lfoToPos;
        std::atomic<float>* lfoToPitch;
        std::atomic<float>* modToPitch;
        std::atomic<float>* subOctave;
        std::atomic<float>* glide;
        std::atomic<float>* bendRange;
        std::atomic<float>* filterMode;
        std::atomic<float>* filterCutoff;
        std::atomic<float>* filterRes;
        std::atomic<float>* filterEnv;
        std::atomic<float>* filterKeytrack;
        std::atomic<float>* lfoToCutoff;
        std::atomic<float>* velToCutoff;
        std::atomic<float>* velSens;
        std::atomic<float>* waveTable;
        std::atomic<float>* waveTableB;
        std::atomic<float>* tableMix;
        std::atomic<float>* subOn;
        std::atomic<float>* lfoSync;
        std::atomic<float>* acidCutoff;
        std::atomic<float>* acidRes;
        std::atomic<float>* acidEnvMod;
        std::atomic<float>* acidDecay;
        std::atomic<float>* acidAccent;
        std::atomic<float>* acidFilter;
        std::atomic<float>* acidWave;
        std::atomic<float>* grit;  // 303 mode: drives its filter
        std::atomic<float>* air;   // 303 mode: brighter, more resonant
    } p;

    void setVelocity (float velocity);
    float noteVelocity = 1.0f;
    // On a voice still sounding, level and velocity's cutoff ease to a new
    // note's over ~3 ms, and its pitch moves in 2.5 ms (quickSlide) when it
    // isn't sliding: instant steps click.
    float levelNow = 0.0f, velocityNow = 1.0f;
    float ampRounded = 0.0f; // the amp envelope, corners rounded over ~0.5 ms
    bool quickSlide = false;
    bool retriggered = false; // this note restarted a voice still sounding (the mod envelope then rises over 4 ms at least)

    // Topology-preserving state-variable filter (Zavalishin), one channel, one stage.
    struct Svf
    {
        float ic1 = 0.0f, ic2 = 0.0f;

        struct Coeffs { float k = 2.0f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f; };

        static Coeffs make (float g, float k) noexcept
        {
            Coeffs c;
            c.k = k;
            c.a1 = 1.0f / (1.0f + g * (g + k));
            c.a2 = g * c.a1;
            c.a3 = g * c.a2;
            return c;
        }

        void process (float x, const Coeffs& c, float& low, float& band, float& high) noexcept
        {
            const auto v3 = x - ic2;
            const auto v1 = c.a1 * ic1 + c.a2 * v3;
            const auto v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            low = v2;
            band = v1;
            high = x - c.k * v1 - v2;
        }
    };

    float filterSample (int ch, float x, int mode, const Svf::Coeffs& resonant, const Svf::Coeffs& flat) noexcept;

    Svf filters[2][2]; // [stage][channel]

    // The 303 switch's filter: four one-pole stages in a ladder with resonance
    // feedback, the linear part solved in the same sample (TPT, Zavalishin) and
    // the input soft-clipped (the squelch). Only near-DC is kept out of the
    // feedback, so the passband drops as resonance rises (the bass thins, as on
    // a 303), partly made up. Stops just short of self-oscillation.
    struct Ladder303
    {
        float s[4] {};
        float feedbackLow = 0.0f; // the low part kept out of the feedback

        struct Coeffs { float G = 0.0f, G4 = 0.0f, k = 0.0f, makeUp = 1.0f, lowCoeff = 0.0f; };

        static Coeffs make (float g, float resonance, float lowCoeff) noexcept
        {
            Coeffs c;
            c.G = g / (1.0f + g);
            c.G4 = c.G * c.G * c.G * c.G;
            c.k = 3.95f * resonance;          // 4 would ring on its own
            c.makeUp = 1.0f + 0.45f * c.k;    // part of the passband back
            c.lowCoeff = lowCoeff;
            return c;
        }

        float process (float x, const Coeffs& c) noexcept
        {
            const auto rest = 1.0f - c.G;
            const auto memory = c.G * c.G * c.G * rest * s[0] + c.G * c.G * rest * s[1] + c.G * rest * s[2] + rest * s[3];
            const auto y4 = (c.G4 * x + memory + c.k * c.G4 * feedbackLow) / (1.0f + c.k * c.G4);
            auto u = std::tanh (x - c.k * (y4 - feedbackLow));
            for (auto& state : s)
            {
                const auto v = (u - state) * c.G;
                const auto y = v + state;
                state = y + v;
                u = y;
            }
            feedbackLow += c.lowCoeff * (u - feedbackLow);
            return u * c.makeUp;
        }

        // 303 mode: twice per sample (coefficients made for the doubled rate),
        // driven harder into the soft clip, the two outputs averaged down.
        // Cleaner screaming resonance at high cutoffs.
        float processTwice (float x, const Coeffs& c, float drive) noexcept
        {
            const auto halfway = 0.5f * (lastIn + x);
            lastIn = x;
            const auto a = process (halfway * drive, c);
            const auto b = process (x * drive, c);
            return 0.5f * (a + b) / std::sqrt (2.0f * drive); // harder drive comes out a little louder, not quieter
        }
        float lastIn = 0.0f;
    };
    Ladder303 ladders[2]; // [channel]

    const MacroMap& macroMap;
    const std::atomic<float>& hostBpm; // set by the processor each block
    std::atomic<float>& livePosition;  // written for the display
    const std::atomic<const WavetableBank*>& sampleTable; // this instance's Sample bank (null: the shared saw)

    void updateEnvelopes();
    float nextLfo (float increment); // smoothed
    float rawLfo (float increment);

    Envelope ampEnv, modEnv; // analog-style curves (Envelope.h)
    bool releasing = false; // after note-off: envelope settings must not be re-applied (see updateEnvelopes)
    const juce::int64 seed;
    juce::Random random;

    // Pitch in octaves (log2 Hz), so glides are even in pitch.
    double pitch = std::log2 (440.0), targetPitch = std::log2 (440.0);
    float bend = 0.0f; // pitch wheel, -1..1
    float level = 0.0f;
    double phases[maxUnison] {};
    double subPhase = 0.0;
    float lfoPhase = 0.0f;
    float lfoHeld = 0.0f;
    bool stolen = false; // stopped at once with its envelopes kept for the next note to rise from
    float lfoSmoothed = 0.0f, lfoSmoothing = 1.0f; // the 3 ms smoothing (set per note from the sample rate)

    // The 303 row (main page). Like the TB-303: its own filter sweep, restarted
    // by each note but not by a slide; accented notes (velocity 100+) get a
    // short fixed sweep, a louder note, and charge the accent "wow" that climbs
    // over back-to-back accents and relaxes after.
    void startAcid() noexcept;
    float acidSweep = 0.0f;   // 1 at a note, falling by the Decay knob (fixed and short on an accent)
    float acidPulse = 0.0f;   // an accented note's own sweep
    float sweepTarget = 0.0f, pulseTarget = 0.0f; // what they rise to in ~1.5 ms (an instant jump of the filter clicks)
    float acidCharge = 0.0f;  // the accent sweep capacitor: rises fast, lets go slowly
    bool acidAccented = false;
    float accentLevel = 1.0f; // the accent's level boost, eased (no click between notes)
};

// juce::Synthesiser plus Mono and Legato play modes: one voice and a stack of
// held notes, so releasing a note slides back to the one still held.
class ZeroSynth : public juce::Synthesiser
{
public:
    explicit ZeroSynth (juce::AudioProcessorValueTreeState& state);

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;
    void allNotesOff (int midiChannel, bool allowTailOff) override;
    void handleSustainPedal (int midiChannel, bool isDown) override;

private:
    enum Mode { poly, mono, legato };

    void removeHeld (int note);

    std::atomic<float>& playMode;
    std::atomic<float>& filter303; // the 303 switch: Legato whatever the sound's mode, as a 303 plays
    int currentMode() const noexcept { return filter303.load() > 0.5f ? (int) legato : (int) playMode.load(); }
    int lastMode = poly;

    static constexpr int maxHeld = 16;
    int heldNotes[maxHeld] {};
    float heldVelocities[maxHeld] {};
    int numHeld = 0;
    bool sustainDown = false; // mono/legato: the last note keeps sounding until the pedal lifts
};
