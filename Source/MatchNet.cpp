#include "MatchNet.h"
#include "MatchNetWeights.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace MatchNet
{
    Features featuresOf (const SoundPrint& print)
    {
        Features f {};
        // Tone frames are 512 samples at 48 kHz (10.7 ms): stretch edges in frames.
        constexpr int edges[] { 0, 5, 14, 37, 94 };
        for (int s = 0; s < 4; ++s)
        {
            int n = 0;
            for (int frame = edges[s]; frame < edges[s + 1] && frame < (int) print.melTone.size(); ++frame, ++n)
                for (int b = 0; b < numBands; ++b)
                    f[(size_t) (s * numBands + b)] += print.melTone[(size_t) frame][(size_t) b];
            for (int b = 0; b < numBands; ++b)
            {
                const auto db = n > 0 ? f[(size_t) (s * numBands + b)] / (float) n : -80.0f;
                f[(size_t) (s * numBands + b)] = juce::jlimit (0.0f, 1.0f, 1.0f + db / 80.0f);
            }
        }
        f[4 * numBands] = juce::jlimit (0.0f, 1.0f, print.sidePct / 100.0f);
        f[4 * numBands + 1] = print.pitchHz > 0.0f ? juce::jlimit (-1.0f, 1.0f, std::log2 (print.pitchHz / 440.0f) / 4.0f) : 0.0f;
        return f;
    }

    std::array<float, numOutputs> Net::run (const Features& x) const
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
        layer (x.data(), numInputs, h1.data(), numHidden, true);
        layer (h1.data(), numHidden, h2.data(), numHidden, true);
        layer (h2.data(), numHidden, out.data(), numOutputs, false);
        return out;
    }

    const Net& builtIn()
    {
        static const Net net = []
        {
            Net n;
            if (MatchNetWeights::count == numWeights)
                n.weights.assign (MatchNetWeights::data, MatchNetWeights::data + MatchNetWeights::count);
            return n;
        }();
        return net;
    }

    std::vector<Guess> guess (const Net& net, const SoundPrint& print, int count)
    {
        std::vector<Guess> guesses;
        if (! net.ready() || print.melTone.empty())
            return guesses;
        const auto out = net.run (featuresOf (print));

        auto softmax = [] (const float* logits, int n)
        {
            std::vector<float> p (logits, logits + n);
            const auto top = *std::max_element (p.begin(), p.end());
            float sum = 0.0f;
            for (auto& v : p)
                sum += (v = std::exp (v - top));
            for (auto& v : p)
                v /= sum;
            return p;
        };
        const auto presets = softmax (out.data(), numPresets);
        const auto tables = softmax (out.data() + numPresets, numTables);
        const auto table = (int) (std::max_element (tables.begin(), tables.end()) - tables.begin());

        std::vector<int> order ((size_t) numPresets);
        std::iota (order.begin(), order.end(), 0);
        std::sort (order.begin(), order.end(), [&] (int a, int b) { return presets[(size_t) a] > presets[(size_t) b]; });
        for (int i = 0; i < juce::jmin (count, numPresets); ++i)
        {
            Guess g;
            g.genre = order[(size_t) i] / 6;
            g.type = order[(size_t) i] % 6;
            g.table = table;
            g.probability = presets[(size_t) order[(size_t) i]];
            for (int u = 0; u < numUnits; ++u)
                g.units[(size_t) u] = 1.0f / (1.0f + std::exp (-out[(size_t) (numPresets + numTables + u)]));
            guesses.push_back (g);
        }
        return guesses;
    }
}
