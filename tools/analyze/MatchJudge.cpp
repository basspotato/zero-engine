#include "MatchJudge.h"

// A verbatim copy of SoundPrint::distance and its helpers as of commit 038d590
// (weights all 1). See MatchJudge.h.
namespace
{
    using Frames = std::vector<std::array<float, SoundPrint::melBands>>;

    double melOf (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); }
    double hzOfMel (double m) { return 700.0 * (std::pow (10.0, m / 2595.0) - 1.0); }

    double bandCentreHz (int b)
    {
        const auto lo = melOf (40.0), hi = melOf (16000.0);
        return hzOfMel (lo + (hi - lo) * (b + 1) / (SoundPrint::melBands + 1));
    }

    int sharedBands (const SoundPrint& a, const SoundPrint& b)
    {
        const auto top = juce::jmin (a.maxHz, b.maxHz);
        int n = 0;
        while (n < SoundPrint::melBands && bandCentreHz (n) < top)
            ++n;
        return juce::jmax (1, n);
    }

    float melDistance (bool overLonger, int bands, const Frames& a, const Frames& b)
    {
        double sum = 0.0;
        int n = 0;
        const auto frames = overLonger ? juce::jmax (a.size(), b.size()) : juce::jmin (a.size(), b.size());
        for (size_t f = 0; f < frames; ++f)
            for (size_t band = 0; band < (size_t) bands; ++band)
            {
                const auto x = f < a.size() ? a[f][band] : -80.0f, y = f < b.size() ? b[f][band] : -80.0f;
                if (x > -70.0f || y > -70.0f)
                {
                    sum += std::abs (x - y);
                    ++n;
                }
            }
        return n > 0 ? (float) (sum / n) : 0.0f;
    }

    struct FrameFeatures { std::array<float, 13> mfcc {}; float centroid = 0.0f, loudnessDb = -80.0f; };

    FrameFeatures featuresOf (const std::array<float, SoundPrint::melBands>& frame, int bands)
    {
        FrameFeatures f;
        for (int k = 1; k <= 13; ++k)
        {
            double sum = 0.0;
            for (int b = 0; b < bands; ++b)
                sum += frame[(size_t) b] * std::cos (juce::MathConstants<double>::pi * k * (b + 0.5) / bands);
            f.mfcc[(size_t) k - 1] = (float) (sum * std::sqrt (2.0 / bands));
        }
        double power = 0.0, weighted = 0.0;
        for (int b = 0; b < bands; ++b)
        {
            const auto p = std::pow (10.0, frame[(size_t) b] / 10.0);
            power += p;
            weighted += b * p;
        }
        f.loudnessDb = (float) (10.0 * std::log10 (power + 1.0e-12));
        f.centroid = power > 0.0 ? (float) (weighted / power) : 0.0f;
        return f;
    }

    struct Composite { float mfcc = 0.0f, centroid = 0.0f, loudness = 0.0f; };

    Composite compositeDistance (bool overLonger, int bands, const Frames& a, const Frames& b)
    {
        std::array<float, SoundPrint::melBands> silence;
        silence.fill (-80.0f);
        const auto frames = overLonger ? juce::jmax (a.size(), b.size()) : juce::jmin (a.size(), b.size());
        double mfcc = 0.0, centroid = 0.0, loudness = 0.0;
        int n = 0, sounding = 0;
        for (size_t i = 0; i < frames; ++i)
        {
            const auto x = featuresOf (i < a.size() ? a[i] : silence, bands), y = featuresOf (i < b.size() ? b[i] : silence, bands);
            if (x.loudnessDb < -60.0f && y.loudnessDb < -60.0f)
                continue;
            double m = 0.0;
            for (size_t k = 0; k < x.mfcc.size(); ++k)
                m += std::abs (x.mfcc[k] - y.mfcc[k]);
            mfcc += m / (double) x.mfcc.size();
            loudness += std::abs (juce::jmax (-60.0f, x.loudnessDb) - juce::jmax (-60.0f, y.loudnessDb));
            ++n;
            if (x.loudnessDb > -60.0f && y.loudnessDb > -60.0f)
            {
                centroid += std::abs (x.centroid - y.centroid);
                ++sounding;
            }
        }
        Composite c;
        if (n > 0)
        {
            c.mfcc = (float) (mfcc / n);
            c.loudness = (float) (loudness / n);
        }
        if (sounding > 0)
            c.centroid = (float) (centroid / sounding);
        return c;
    }
}

float frozenJudge (const SoundPrint& a, const SoundPrint& b)
{
    if (std::abs (a.levelDb - b.levelDb) > 40.0f || a.melTone.empty() || b.melTone.empty())
        return 100.0f;
    const auto overLonger = ! a.cut && ! b.cut;
    const auto bands = sharedBands (a, b);
    const auto mel = 0.5f * (melDistance (overLonger, bands, a.melTone, b.melTone) + melDistance (overLonger, bands, a.melAttack, b.melAttack)) / 4.0f;
    const auto c = compositeDistance (overLonger, bands, a.melTone, b.melTone);
    const auto width = (a.sidePct - b.sidePct) / 15.0f;
    return mel + c.mfcc / 4.0f + c.centroid / 2.0f + c.loudness / 3.0f + 0.5f * width * width;
}

juce::String frozenJudgeWord (float d)
{
    return d < 2.0f ? "VERY CLOSE" : d < 3.5f ? "CLOSE" : d < 5.0f ? "ROUGH" : "LOOSE";
}
