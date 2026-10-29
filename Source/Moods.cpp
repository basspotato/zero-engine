#include "Moods.h"
#include "Parameters.h"

namespace
{
    // Wavetable banks (waveTableNames order).
    enum Table { basic, growl, digital, organ, fm, wavefold, pwm, neuro, bell, choir, harsh, analog, jupiter };
    enum Type { bass, lead, chord, pad, pluck, fx };
    enum Genre { edm, dubstep, techno, techHouse, trap, synthwave, dnb, pop, house, trance, futureBass, hardstyle };
}

namespace Moods
{
    const char* nameOf (int mood)
    {
        constexpr const char* names[count] { "LUSH PAD", "AIRY DRONE", "WARM KEYS", "GLASSY BELL", "DARK TEXTURE", "SOFT PLUCK" };
        return names[juce::jlimit (0, count - 1, mood)];
    }

    int typeOf (int mood)
    {
        constexpr int types[count] { pad, pad, chord, pluck, fx, pluck };
        return types[juce::jlimit (0, count - 1, mood)];
    }

    Profile make (int mood, juce::Random& random, int& baseGenre)
    {
        mood = juce::jlimit (0, count - 1, mood);
        auto between = [&] (float lo, float hi) { return lo + random.nextFloat() * (hi - lo); };
        auto logBetween = [&] (float lo, float hi) { return lo * std::pow (hi / lo, random.nextFloat()); };
        auto pick = [&] (std::initializer_list<int> options) { return *(options.begin() + random.nextInt ((int) options.size())); };

        // The genres whose profiles of this type sit nearest the mood fill what
        // the recipe leaves alone.
        const auto type = typeOf (mood);
        baseGenre = mood == darkTexture ? pick ({ techno, dubstep, dnb })
                  : mood == warmKeys    ? pick ({ house, pop, synthwave })
                                        : pick ({ synthwave, trance, futureBass, pop });
        auto p = makeProfile (baseGenre, type);

        // What every mood shares: no wobble on the pitch, free (unsynced) slow
        // movement, a gentle finish, no hard clipping drive.
        p.lfoToPitch = 0.0f;
        p.modToPitch = 0.0f;
        p.lfoSync = 0.0f;
        p.lfoShape = (float) pick ({ 0, 1 }); // sine or triangle: smooth
        p.spectral = between (0.0f, 0.2f);
        p.ottDepth = between (0.0f, 0.3f);
        p.playMode = 0.0f;
        p.glide = 0.0f;

        switch (mood)
        {
            case lushPad:
                p.waveTable = (float) pick ({ jupiter, analog, choir, basic });
                p.waveTableB = (float) pick ({ choir, jupiter, analog });
                p.tableMix = between (0.2f, 0.5f);
                p.wavePos = between (0.2f, 0.7f);
                p.unison = (float) (3 + random.nextInt (4));
                p.detune = between (0.15f, 0.35f);
                p.ampAttack = logBetween (0.3f, 1.5f);
                p.ampDecay = between (1.0f, 3.0f);
                p.ampSustain = between (0.7f, 1.0f);
                p.ampRelease = between (1.5f, 3.0f);
                p.filterMode = (float) pick ({ 0, 1 });
                p.filterCutoff = logBetween (1500.0f, 5000.0f);
                p.filterRes = between (0.0f, 0.2f);
                p.filterEnv = between (0.0f, 1.0f);
                p.lfoRate = logBetween (0.1f, 0.6f);
                p.lfoToPos = between (0.1f, 0.3f);
                p.lfoToCutoff = between (0.0f, 0.5f);
                p.reverbMix = between (0.35f, 0.6f);
                p.reverbSize = between (0.7f, 1.0f);
                p.delayMix = between (0.0f, 0.2f);
                p.width = between (0.5f, 0.8f);
                p.grit = between (0.0f, 0.15f);
                p.air = between (0.4f, 0.7f);
                p.motion = between (0.3f, 0.6f);
                p.tilt = between (-2.0f, 1.0f);
                break;

            case airyDrone:
                p.waveTable = (float) pick ({ choir, organ, basic, jupiter });
                p.waveTableB = (float) pick ({ choir, basic, organ });
                p.tableMix = between (0.2f, 0.6f);
                p.wavePos = between (0.0f, 0.5f);
                p.unison = (float) (2 + random.nextInt (4));
                p.detune = between (0.1f, 0.3f);
                p.ampAttack = logBetween (1.0f, 2.0f);
                p.ampDecay = 3.0f;
                p.ampSustain = 1.0f;
                p.ampRelease = between (2.0f, 3.0f);
                p.filterMode = 0.0f;
                p.filterCutoff = logBetween (1500.0f, 5000.0f); // darker: the owner heard harshness
                p.filterRes = between (0.0f, 0.15f);
                p.filterEnv = 0.0f;
                p.lfoRate = logBetween (0.05f, 0.3f);
                p.lfoToPos = between (0.2f, 0.5f);
                p.lfoToCutoff = between (0.3f, 1.0f);
                p.reverbMix = between (0.5f, 0.8f);
                p.reverbSize = between (0.8f, 1.0f);
                p.delayMix = between (0.1f, 0.3f);
                p.delayFeedback = between (0.3f, 0.5f);
                p.width = between (0.6f, 0.9f);
                p.grit = between (0.0f, 0.1f);
                p.air = between (0.45f, 0.7f);
                p.motion = between (0.4f, 0.7f);
                p.tilt = between (-1.5f, 1.0f);
                break;

            case warmKeys:
                p.waveTable = (float) pick ({ jupiter, organ, basic, fm });
                p.waveTableB = (float) pick ({ organ, jupiter, analog });
                p.tableMix = between (0.0f, 0.4f);
                p.wavePos = between (0.1f, 0.5f);
                p.unison = (float) (1 + random.nextInt (3));
                p.detune = between (0.05f, 0.2f);
                p.ampAttack = logBetween (0.002f, 0.02f);
                p.ampDecay = between (0.6f, 1.8f);
                p.ampSustain = between (0.3f, 0.6f);
                p.ampRelease = between (0.3f, 0.8f);
                p.filterMode = 0.0f;
                p.filterCutoff = logBetween (1000.0f, 4000.0f);
                p.filterRes = between (0.0f, 0.2f);
                p.filterEnv = between (0.5f, 2.0f);
                p.modDecay = between (0.3f, 1.0f);
                p.lfoRate = logBetween (0.2f, 1.0f);
                p.lfoToPos = between (0.0f, 0.15f);
                p.lfoToCutoff = between (0.0f, 0.2f);
                p.reverbMix = between (0.15f, 0.35f);
                p.reverbSize = between (0.4f, 0.8f);
                p.delayMix = between (0.0f, 0.15f);
                p.width = between (0.3f, 0.6f);
                p.grit = between (0.0f, 0.1f); // clean: more read as distorted mids
                p.ottDepth = between (0.0f, 0.15f);
                p.spectral = between (0.0f, 0.1f);
                p.air = between (0.2f, 0.5f);
                p.motion = between (0.1f, 0.3f);
                p.tilt = between (-3.0f, 0.0f);
                break;

            case glassyBell:
                p.waveTable = (float) pick ({ bell, fm, basic });
                p.waveTableB = (float) pick ({ basic, bell });
                p.tableMix = between (0.0f, 0.3f);
                p.wavePos = between (0.2f, 0.8f);
                p.unison = (float) (1 + random.nextInt (2));
                p.detune = between (0.0f, 0.1f);
                p.ampAttack = logBetween (0.001f, 0.01f);
                p.ampDecay = between (0.8f, 2.5f);
                p.ampSustain = between (0.0f, 0.2f);
                p.ampRelease = between (0.8f, 2.0f);
                p.filterMode = 0.0f;
                p.filterCutoff = logBetween (3500.0f, 9000.0f); // softer top: the owner found bells too much
                p.filterRes = between (0.0f, 0.15f);
                p.filterEnv = between (0.0f, 1.0f);
                p.lfoRate = logBetween (0.2f, 1.0f);
                p.lfoToPos = between (0.0f, 0.2f);
                p.lfoToCutoff = 0.0f;
                p.reverbMix = between (0.25f, 0.42f);
                p.reverbSize = between (0.6f, 0.9f);
                p.delayMix = between (0.15f, 0.35f);
                p.delayFeedback = between (0.2f, 0.35f);
                p.width = between (0.4f, 0.7f);
                p.grit = 0.0f;
                p.air = between (0.3f, 0.55f);
                p.motion = between (0.1f, 0.3f);
                p.tilt = between (-1.0f, 1.0f);
                break;

            case darkTexture:
                p.waveTable = (float) pick ({ growl, neuro, wavefold, digital, choir });
                p.waveTableB = (float) pick ({ choir, growl, analog });
                p.tableMix = between (0.2f, 0.6f);
                p.wavePos = between (0.2f, 0.8f);
                p.unison = (float) (2 + random.nextInt (4));
                p.detune = between (0.1f, 0.3f);
                p.ampAttack = logBetween (0.2f, 1.0f);
                p.ampDecay = 2.0f;
                p.ampSustain = 0.8f;
                p.ampRelease = between (1.0f, 2.5f);
                p.filterMode = 0.0f;
                p.filterCutoff = logBetween (300.0f, 1500.0f);
                p.filterRes = between (0.2f, 0.5f);
                p.filterEnv = between (0.0f, 1.0f);
                p.lfoRate = logBetween (0.1f, 1.0f);
                p.lfoToPos = between (0.2f, 0.5f);
                p.lfoToCutoff = between (0.5f, 1.5f);
                p.reverbMix = between (0.4f, 0.7f);
                p.reverbSize = between (0.7f, 1.0f);
                p.delayMix = between (0.0f, 0.25f);
                p.width = between (0.5f, 0.8f);
                p.grit = between (0.2f, 0.5f);
                p.air = between (0.1f, 0.4f);
                p.motion = between (0.4f, 0.8f);
                p.tilt = between (-4.0f, -1.0f);
                break;

            default: // softPluck
                p.waveTable = (float) pick ({ analog, jupiter, basic, pwm });
                p.waveTableB = (float) pick ({ jupiter, analog, choir });
                p.tableMix = between (0.0f, 0.4f);
                p.wavePos = between (0.2f, 0.7f);
                p.unison = (float) (1 + random.nextInt (3));
                p.detune = between (0.05f, 0.2f);
                p.ampAttack = logBetween (0.001f, 0.005f);
                p.ampDecay = between (0.2f, 0.6f);
                p.ampSustain = between (0.0f, 0.15f);
                p.ampRelease = between (0.2f, 0.6f);
                p.filterMode = 0.0f;
                p.filterCutoff = logBetween (800.0f, 3000.0f);
                p.filterRes = between (0.05f, 0.3f);
                p.filterEnv = between (1.5f, 3.5f);
                p.modDecay = between (0.1f, 0.4f);
                p.lfoRate = logBetween (0.2f, 1.0f);
                p.lfoToPos = between (0.0f, 0.15f);
                p.lfoToCutoff = 0.0f;
                p.reverbMix = between (0.2f, 0.4f);
                p.reverbSize = between (0.5f, 0.8f);
                p.delayMix = between (0.1f, 0.3f);
                p.delayFeedback = between (0.2f, 0.4f);
                p.width = between (0.4f, 0.7f);
                p.grit = between (0.0f, 0.15f);
                p.air = between (0.3f, 0.6f);
                p.motion = between (0.1f, 0.3f);
                p.tilt = between (-2.0f, 1.0f);
                break;
        }
        p.levelTrim = juce::jmin (p.levelTrim, 0.0f); // after the clipper: never above the profile's own
        return p;
    }
}
