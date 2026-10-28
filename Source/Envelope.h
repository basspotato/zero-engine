#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// An analog-style ADSR: each stage moves exponentially toward a target set a
// little past where it should end (as an RC circuit charging toward a higher
// voltage), so it curves like a real envelope yet still arrives in finite time.
// Attack curves gently (charging toward 1.3); decay and release fall fast, then
// trail off, as plucks and the Jupiter-8's envelopes do.
//
// Decay and release are stretched 1.9x inside: a stage then falls 20 dB in about
// the time a straight-line one did, so presets designed with the old linear
// envelope keep their length, with a longer quiet tail. Same interface as
// juce::ADSR. Real-time safe.
class Envelope
{
public:
    using Parameters = juce::ADSR::Parameters;

    void setSampleRate (double newRate) noexcept { sampleRate = newRate; recalculate(); }
    void setParameters (const Parameters& p) noexcept { params = p; recalculate(); }

    void noteOn() noexcept { stage = params.attack > 0.0f ? Stage::attack : Stage::decay; if (stage == Stage::decay) level = 1.0f; }
    void noteOff() noexcept { if (stage != Stage::idle) stage = Stage::release; }
    void reset() noexcept { stage = Stage::idle; level = 0.0f; }
    bool isActive() const noexcept { return stage != Stage::idle; }

    float getNextSample() noexcept
    {
        switch (stage)
        {
            case Stage::attack:
                level = attackBase + level * attackCoeff;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::decay;
                }
                break;

            case Stage::decay:
                level = decayBase + level * decayCoeff;
                if (level <= params.sustain)
                {
                    level = params.sustain;
                    stage = Stage::sustain;
                }
                break;

            case Stage::sustain:
                level = params.sustain;
                break;

            case Stage::release:
                level = releaseBase + level * releaseCoeff;
                if (level <= 0.0f)
                    reset();
                break;

            case Stage::idle:
                break;
        }
        return level;
    }

private:
    enum class Stage { idle, attack, decay, sustain, release };

    // How far past its end each stage aims, as a share of the stage's span:
    // small means a strongly curved (exponential) stage.
    static constexpr float attackOvershoot = 0.3f, fallOvershoot = 0.01f, fallStretch = 1.9f;

    // Per-sample coefficient for a stage of `seconds` that aims `overshoot` past its end.
    float coefficient (float seconds, float overshoot) const noexcept
    {
        const auto samples = juce::jmax (1.0, (double) seconds * sampleRate);
        return (float) std::exp (-std::log ((1.0 + overshoot) / overshoot) / samples);
    }

    void recalculate() noexcept
    {
        attackCoeff = coefficient (params.attack, attackOvershoot);
        attackBase = (1.0f + attackOvershoot) * (1.0f - attackCoeff);

        decayCoeff = coefficient (params.decay * fallStretch, fallOvershoot);
        decayBase = (params.sustain - fallOvershoot * (1.0f - params.sustain)) * (1.0f - decayCoeff);

        releaseCoeff = coefficient (params.release * fallStretch, fallOvershoot);
        releaseBase = -fallOvershoot * (1.0f - releaseCoeff);
    }

    Parameters params;
    double sampleRate = 44100.0;
    Stage stage = Stage::idle;
    float level = 0.0f;
    float attackCoeff = 0.0f, attackBase = 0.0f, decayCoeff = 0.0f, decayBase = 0.0f, releaseCoeff = 0.0f, releaseBase = 0.0f;
};
