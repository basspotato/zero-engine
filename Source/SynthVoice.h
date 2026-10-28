#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MacroMap.h"

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
               std::atomic<float>& livePosition, int index);
    void reseed() { random.setSeed (seed); } // back to a fresh engine's phases (offline renders)

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
    } p;

    void setVelocity (float velocity);
    float noteVelocity = 1.0f;

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

    const MacroMap& macroMap;
    const std::atomic<float>& hostBpm; // set by the processor each block
    std::atomic<float>& livePosition;  // written for the display

    void updateEnvelopes();
    float nextLfo (float increment);

    juce::ADSR ampEnv, modEnv;
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
    int lastMode = poly;

    static constexpr int maxHeld = 16;
    int heldNotes[maxHeld] {};
    float heldVelocities[maxHeld] {};
    int numHeld = 0;
    bool sustainDown = false; // mono/legato: the last note keeps sounding until the pedal lifts
};
