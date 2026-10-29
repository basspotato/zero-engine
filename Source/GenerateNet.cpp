#include "GenerateNet.h"
#include "GenerateNetWeights.h"
#include "GenerateTargets.h"
#include "ReferenceMatch.h"
#include "Wavetable.h"

namespace GenerateNet
{
    std::vector<float> featuresOf (const Profile& p, int genre, int type)
    {
        std::vector<float> f;
        f.reserve (128);
        for (const auto u : ReferenceMatch::unitsOf (p)) // the 22 searched settings, 0..1
            f.push_back (u);
        auto oneHot = [&f] (int index, int count, float value = 1.0f)
        {
            for (int i = 0; i < count; ++i)
                f.push_back (i == index ? value : 0.0f);
        };
        oneHot ((int) p.waveTable, WavetableBank::numBanks);
        oneHot ((int) p.waveTableB, WavetableBank::numBanks, p.tableMix);
        oneHot ((int) p.filterMode, 4);
        oneHot ((int) p.lfoShape, 5);
        oneHot (type, 6);
        oneHot (genre, 12);
        for (const auto v : { p.weight, p.grit, p.punch, p.width, p.motion, p.air, p.blend, p.tableMix, p.filterKeytrack, p.subOn,
                              p.reverbSize, p.spectral })
            f.push_back (juce::jlimit (0.0f, 1.0f, v));
        f.push_back (p.unison / 7.0f);
        f.push_back (0.5f + 0.5f * juce::jlimit (-1.0f, 1.0f, p.modToPos));
        f.push_back (juce::jlimit (-1.0f, 1.0f, p.modToPitch / 12.0f));
        f.push_back (juce::jlimit (0.0f, 1.0f, p.lfoToPitch));
        f.push_back (p.lfoSync / 9.0f);
        f.push_back (p.delayTime / 10.0f);
        f.push_back (juce::jlimit (-1.0f, 1.0f, p.levelTrim / 12.0f));
        f.push_back (juce::jlimit (0.0f, 1.0f, p.monoFreq / 500.0f));
        f.push_back (juce::jlimit (0.0f, 1.0f, std::log2 (juce::jmax (1.0f, p.modAttack * 1000.0f)) / 11.0f));
        f.push_back (juce::jlimit (0.0f, 1.0f, p.modSustain));
        return f;
    }

    int numInputs()
    {
        static const auto n = (int) featuresOf (makeProfile (0, 0), 0, 0).size();
        return n;
    }

    std::array<float, numOutputs> outputsOf (const SoundTraits& t, int type)
    {
        const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];
        std::array<float, numOutputs> o {};
        const auto a = t.asArray();
        for (int k = 0; k < SoundTraits::count; ++k)
            o[(size_t) k] = k == 2 ? std::log2 (juce::jmax (20.0f, a[2]) / juce::jmax (20.0f, guide.median[2])) / 0.8f
                                   : (a[(size_t) k] - guide.median[(size_t) k]) / juce::jmax (1.0e-3f, guide.spread[(size_t) k]);
        o[SoundTraits::count] = t.clarity;
        o[SoundTraits::count + 1] = t.pitchSpreadCents / 20.0f;
        o[SoundTraits::count + 2] = t.flatness * 20.0f;
        for (auto& v : o)
            v = juce::jlimit (-6.0f, 6.0f, v);
        return o;
    }

    SoundTraits traitsOf (const std::array<float, numOutputs>& o, int type)
    {
        const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];
        std::array<float, SoundTraits::count> a {};
        for (int k = 0; k < SoundTraits::count; ++k)
            a[(size_t) k] = k == 2 ? juce::jmax (20.0f, guide.median[2]) * std::exp2 (0.8f * o[2])
                                   : guide.median[(size_t) k] + o[(size_t) k] * juce::jmax (1.0e-3f, guide.spread[(size_t) k]);
        SoundTraits t;
        t.levelDb = a[0]; t.sidePct = a[1]; t.centroidHz = a[2]; t.bassPct = a[3]; t.bodyPct = a[4];
        t.spreadOct = a[5]; t.crestDb = a[6]; t.movementDb = a[7]; t.richness = a[8];
        t.clarity = o[SoundTraits::count];
        t.pitchSpreadCents = o[SoundTraits::count + 1] * 20.0f;
        t.flatness = o[SoundTraits::count + 2] / 20.0f;
        return t;
    }

    size_t numWeights()
    {
        const auto in = (size_t) numInputs();
        return in * numHidden + numHidden + (size_t) numHidden * numHidden + numHidden + (size_t) numOutputs * numHidden + numOutputs;
    }

    bool Net::ready() const noexcept { return ! weights.empty() && weights.size() == numWeights(); }

    std::array<float, numOutputs> Net::run (const std::vector<float>& x) const
    {
        std::array<float, numOutputs> out {};
        if (! ready())
            return out;
        const auto* w = weights.data();
        std::array<float, numHidden> h1 {}, h2 {};
        auto layer = [&w] (const float* in, int inputs, float* result, int outputs, bool relu)
        {
            const auto* bias = w + (size_t) outputs * inputs;
            for (int o = 0; o < outputs; ++o)
            {
                const auto* row = w + (size_t) o * inputs;
                auto sum = bias[o];
                for (int i = 0; i < inputs; ++i)
                    sum += row[i] * in[i];
                result[o] = relu ? juce::jmax (0.0f, sum) : sum;
            }
            w = bias + outputs;
        };
        layer (x.data(), (int) x.size(), h1.data(), numHidden, true);
        layer (h1.data(), numHidden, h2.data(), numHidden, true);
        layer (h2.data(), numHidden, out.data(), numOutputs, false);
        return out;
    }

    const Net& builtIn()
    {
        static const Net net = []
        {
            Net n;
            if (GenerateNetWeights::count == numWeights())
                n.weights.assign (GenerateNetWeights::data, GenerateNetWeights::data + GenerateNetWeights::count);
            return n;
        }();
        return net;
    }
}
