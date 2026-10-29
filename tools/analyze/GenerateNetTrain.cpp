#include "GenerateNetTrain.h"
#include "../../Source/GenerateNet.h"
#include "../../Source/Moods.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"
#include "../../Source/ReferenceMatch.h"
#include "../../Source/Wavetable.h"
#include <atomic>
#include <numeric>
#include <random>
#include <thread>

namespace
{
    struct Record { int genre = 0, type = 0; std::vector<float> features; std::array<float, GenerateNet::numOutputs> outputs {}; };

    // A candidate as Generate makes them: a plain variation, a two-genre blend
    // (any genre), or a variation pushed a little further (the push step).
    Profile candidate (int genre, int type, juce::Random& random, int& source)
    {
        source = genre;
        auto p = generateProfile (genre, type, random);
        const auto kind = random.nextFloat();
        if (kind < 0.25f)
        {
            const auto other = (genre + 1 + random.nextInt (11)) % 12;
            const auto q = generateProfile (other, type, random);
            const auto mix = 0.25f + 0.5f * random.nextFloat();
            auto u = ReferenceMatch::unitsOf (p);
            const auto v = ReferenceMatch::unitsOf (q);
            for (size_t k = 0; k < u.size(); ++k)
                u[k] += mix * (v[k] - u[k]);
            auto lead = mix > 0.5f ? q : p;
            const auto table = random.nextBool() ? q.waveTable : p.waveTable;
            p = ReferenceMatch::withUnitsOf (lead, u);
            p.waveTable = table;
            source = mix > 0.5f ? other : genre;
        }
        else if (kind < 0.5f)
        {
            auto u = ReferenceMatch::unitsOf (p);
            for (int n = 0; n < 5; ++n)
            {
                auto& x = u[(size_t) random.nextInt (ReferenceMatch::numSearched)];
                x = juce::jlimit (0.0f, 1.0f, x + 0.25f * (random.nextFloat() * 2.0f - 1.0f));
            }
            p = ReferenceMatch::withUnitsOf (p, u);
        }
        // Detune capped as Generate caps it.
        p.detune = juce::jmin (p.detune, juce::jmax (makeProfile (source, type).detune, 0.4f) + 0.1f);
        return p;
    }

    juce::File fileArg (const juce::StringArray& args, int index)
    {
        return args.size() > index && ! args[index].startsWith ("--")
                   ? juce::File::getCurrentWorkingDirectory().getChildFile (args[index])
                   : juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/net/generate.bin");
    }

    std::vector<Record> readRecords (const juce::File& file)
    {
        std::vector<Record> records;
        juce::FileInputStream in (file);
        if (! in.openedOk())
            return records;
        const auto count = in.readInt(), inputs = in.readInt();
        records.resize ((size_t) juce::jmax (0, count));
        for (auto& r : records)
        {
            r.genre = in.readInt();
            r.type = in.readInt();
            r.features.resize ((size_t) inputs);
            in.read (r.features.data(), (int) (sizeof (float) * (size_t) inputs));
            in.read (r.outputs.data(), (int) sizeof (r.outputs));
        }
        return records;
    }

    // Inputs -> 128 -> 128 -> outputs, ReLU, as GenerateNet::Net::run.
    struct Trainer
    {
        int in;
        std::vector<float> w;
        size_t w1 = 0, b1, w2, b2, w3, b3;
        static constexpr int h = GenerateNet::numHidden, out = GenerateNet::numOutputs;

        explicit Trainer (int inputs, std::mt19937& rng) : in (inputs)
        {
            b1 = w1 + (size_t) h * in; w2 = b1 + h; b2 = w2 + (size_t) h * h; w3 = b2 + h; b3 = w3 + (size_t) out * h;
            w.assign (b3 + out, 0.0f);
            std::normal_distribution<float> gauss (0.0f, 1.0f);
            auto fill = [&] (size_t at, int outputs, int inputs2)
            {
                const auto scale = std::sqrt (2.0f / (float) inputs2);
                for (size_t i = 0; i < (size_t) outputs * inputs2; ++i)
                    w[at + i] = gauss (rng) * scale;
            };
            fill (w1, h, in);
            fill (w2, h, h);
            fill (w3, out, h);
        }

        float backprop (const Record& r, std::vector<float>& g) const
        {
            std::vector<float> z1 ((size_t) h), a1 ((size_t) h), z2 ((size_t) h), a2 ((size_t) h);
            std::array<float, out> o {};
            for (int j = 0; j < h; ++j)
            {
                auto s = w[b1 + (size_t) j];
                for (int i = 0; i < in; ++i)
                    s += w[w1 + (size_t) j * in + (size_t) i] * r.features[(size_t) i];
                z1[(size_t) j] = s;
                a1[(size_t) j] = juce::jmax (0.0f, s);
            }
            for (int j = 0; j < h; ++j)
            {
                auto s = w[b2 + (size_t) j];
                for (int i = 0; i < h; ++i)
                    s += w[w2 + (size_t) j * h + (size_t) i] * a1[(size_t) i];
                z2[(size_t) j] = s;
                a2[(size_t) j] = juce::jmax (0.0f, s);
            }
            float loss = 0.0f;
            std::array<float, out> d {};
            for (int k = 0; k < out; ++k)
            {
                auto s = w[b3 + (size_t) k];
                for (int i = 0; i < h; ++i)
                    s += w[w3 + (size_t) k * h + (size_t) i] * a2[(size_t) i];
                o[(size_t) k] = s;
                const auto e = s - r.outputs[(size_t) k];
                loss += e * e / out;
                d[(size_t) k] = 2.0f * e / out;
            }
            std::vector<float> d2 ((size_t) h, 0.0f), d1 ((size_t) h, 0.0f);
            for (int k = 0; k < out; ++k)
            {
                g[b3 + (size_t) k] += d[(size_t) k];
                for (int i = 0; i < h; ++i)
                {
                    g[w3 + (size_t) k * h + (size_t) i] += d[(size_t) k] * a2[(size_t) i];
                    d2[(size_t) i] += d[(size_t) k] * w[w3 + (size_t) k * h + (size_t) i];
                }
            }
            for (int j = 0; j < h; ++j)
            {
                const auto gj = z2[(size_t) j] > 0.0f ? d2[(size_t) j] : 0.0f;
                g[b2 + (size_t) j] += gj;
                for (int i = 0; i < h; ++i)
                {
                    g[w2 + (size_t) j * h + (size_t) i] += gj * a1[(size_t) i];
                    d1[(size_t) i] += gj * w[w2 + (size_t) j * h + (size_t) i];
                }
            }
            for (int j = 0; j < h; ++j)
            {
                const auto gj = z1[(size_t) j] > 0.0f ? d1[(size_t) j] : 0.0f;
                g[b1 + (size_t) j] += gj;
                for (int i = 0; i < in; ++i)
                    g[w1 + (size_t) j * in + (size_t) i] += gj * r.features[(size_t) i];
            }
            return loss;
        }
    };
}

int runGenerateNetData (const juce::StringArray& args)
{
    const auto count = args.size() > 1 ? args[1].getIntValue() : 30000;
    const auto out = fileArg (args, 2);
    const auto seed = args.size() > 3 ? args[3].getIntValue() : 1;
    out.getParentDirectory().createDirectory();
    std::vector<Record> records ((size_t) count);
    std::atomic<int> next { 0 }, done { 0 };
    const auto threads = (int) juce::jlimit (1u, 8u, std::thread::hardware_concurrency() - 1);
    const auto began = juce::Time::getMillisecondCounter();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&, t]
        {
            ZeroEngineProcessor engine;
            engine.stopProfileSync();
            juce::Random random ((juce::int64) seed * 7919 + t);
            for (int i = next++; i < count; i = next++)
            {
                auto& r = records[(size_t) i];
                r.genre = random.nextInt (12);
                r.type = random.nextInt (6);
                int source = r.genre;
                // A third from the mood recipes (Mood mode), the rest as Genre mode draws.
                Profile p;
                if (random.nextFloat() < 0.33f)
                {
                    const auto mood = random.nextInt (Moods::count);
                    r.type = Moods::typeOf (mood);
                    p = Moods::make (mood, random, source);
                }
                else
                    p = candidate (r.genre, r.type, random, source);
                if (random.nextFloat() < 0.15f) // some as Mutate makes them: five settings +-25 %, a third with another second wavetable
                {
                    auto units = ReferenceMatch::unitsOf (p);
                    for (int n = 0; n < 5; ++n)
                    {
                        auto& u = units[(size_t) random.nextInt (ReferenceMatch::numSearched)];
                        u = juce::jlimit (0.0f, 1.0f, u + 0.25f * (random.nextFloat() * 2.0f - 1.0f));
                    }
                    p = ReferenceMatch::withUnitsOf (p, units);
                    if (random.nextFloat() < 0.33f)
                    {
                        p.waveTableB = (float) random.nextInt (WavetableBank::numBanks - 1);
                        p.tableMix = juce::jlimit (0.0f, 1.0f, p.tableMix + 0.15f + 0.2f * random.nextFloat());
                    }
                }
                r.genre = source;
                r.features = GenerateNet::featuresOf (p, source, r.type);
                r.outputs = GenerateNet::outputsOf (ZeroEngineProcessor::measureForTools (engine, source, r.type, p), r.type);
                if (const auto d = ++done; d % 2000 == 0)
                    std::printf ("  %d of %d, %.0f s\n", d, count, (juce::Time::getMillisecondCounter() - began) / 1000.0);
            }
        });
    for (auto& t : pool)
        t.join();

    juce::FileOutputStream stream (out);
    if (! stream.openedOk())
        return 1;
    stream.setPosition (0);
    stream.truncate();
    stream.writeInt (count);
    stream.writeInt (GenerateNet::numInputs());
    for (const auto& r : records)
    {
        stream.writeInt (r.genre);
        stream.writeInt (r.type);
        stream.write (r.features.data(), sizeof (float) * r.features.size());
        stream.write (r.outputs.data(), sizeof (r.outputs));
    }
    std::printf ("wrote %d records (%d inputs) to %s\n", count, GenerateNet::numInputs(), out.getFullPathName().toRawUTF8());
    return 0;
}

int runGenerateNetTrain (const juce::StringArray& args)
{
    auto records = readRecords (fileArg (args, 1));
    if (records.empty())
    {
        std::printf ("no data: run gennetdata first\n");
        return 1;
    }
    const auto epochs = args.size() > 2 && ! args[2].startsWith ("--") ? args[2].getIntValue() : 40;
    std::mt19937 rng (3);
    std::shuffle (records.begin(), records.end(), rng);
    const auto held = records.size() / 10, trainCount = records.size() - held;
    const auto inputs = (int) records.front().features.size();
    Trainer net (inputs, rng);
    std::printf ("%d records, %d inputs: %d to train on, %d held out\n", (int) records.size(), inputs, (int) trainCount, (int) held);

    constexpr int batch = 64;
    const auto threads = (int) juce::jlimit (1u, 8u, std::thread::hardware_concurrency());
    std::vector<std::vector<float>> grads ((size_t) threads, std::vector<float> (net.w.size()));
    std::vector<float> m (net.w.size(), 0.0f), v (net.w.size(), 0.0f);
    int step = 0;
    const char* names[] { "level", "width", "bright", "bass", "body", "spread", "crest", "move", "rich", "clarity", "wobble", "noise" };

    auto evaluate = [&]
    {
        // Per output: R^2 on the held-out tenth (1 perfect, 0 no better than the mean).
        GenerateNet::Net probe;
        probe.weights = net.w;
        std::array<double, GenerateNet::numOutputs> se {}, mean {}, var {};
        for (size_t i = trainCount; i < records.size(); ++i)
            for (int k = 0; k < GenerateNet::numOutputs; ++k)
                mean[(size_t) k] += records[i].outputs[(size_t) k] / (double) held;
        for (size_t i = trainCount; i < records.size(); ++i)
        {
            const auto o = probe.run (records[i].features);
            for (int k = 0; k < GenerateNet::numOutputs; ++k)
            {
                se[(size_t) k] += juce::square ((double) o[(size_t) k] - records[i].outputs[(size_t) k]);
                var[(size_t) k] += juce::square ((double) records[i].outputs[(size_t) k] - mean[(size_t) k]);
            }
        }
        std::printf ("    held-out R2:");
        for (int k = 0; k < GenerateNet::numOutputs; ++k)
            std::printf (" %s %.2f", names[k], 1.0 - se[(size_t) k] / juce::jmax (1.0e-9, var[(size_t) k]));
        std::printf ("\n");
    };

    for (int epoch = 0; epoch < epochs; ++epoch)
    {
        std::shuffle (records.begin(), records.begin() + (long) trainCount, rng);
        const auto lr = 1.0e-3f * (epoch < epochs * 2 / 3 ? 1.0f : 0.3f);
        double total = 0.0;
        for (size_t start = 0; start + batch <= trainCount; start += batch)
        {
            std::vector<double> losses ((size_t) threads, 0.0);
            std::vector<std::thread> pool;
            for (int t = 0; t < threads; ++t)
                pool.emplace_back ([&, t]
                {
                    auto& g = grads[(size_t) t];
                    std::fill (g.begin(), g.end(), 0.0f);
                    for (size_t i = start + (size_t) t; i < start + batch; i += (size_t) threads)
                        losses[(size_t) t] += net.backprop (records[i], g);
                });
            for (auto& t : pool)
                t.join();
            ++step;
            const auto c1 = 1.0f - std::pow (0.9f, (float) step), c2 = 1.0f - std::pow (0.999f, (float) step);
            for (size_t i = 0; i < net.w.size(); ++i)
            {
                float g = 0.0f;
                for (const auto& gt : grads)
                    g += gt[i];
                g /= batch;
                m[i] = 0.9f * m[i] + 0.1f * g;
                v[i] = 0.999f * v[i] + 0.001f * g * g;
                net.w[i] -= lr * (m[i] / c1) / (std::sqrt (v[i] / c2) + 1.0e-8f);
            }
            for (const auto l : losses)
                total += l;
        }
        std::printf ("epoch %d: loss %.3f\n", epoch + 1, total / (double) trainCount);
        if ((epoch + 1) % 10 == 0 || epoch + 1 == epochs)
            evaluate();
    }

    const auto weightsFile = juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/net/generate_weights.bin");
    juce::FileOutputStream w (weightsFile);
    if (w.openedOk())
    {
        w.setPosition (0);
        w.truncate();
        w.write (net.w.data(), sizeof (float) * net.w.size());
        std::printf ("weights in %s\n", weightsFile.getFullPathName().toRawUTF8());
    }
    if (args.contains ("--write"))
    {
        juce::String text;
        text << "#pragma once\n\n#include <cstddef>\n\n"
             << "// Generate's trained neural judge (GenerateNet.h). Written by\n"
             << "// `ZeroAnalyze gennettrain --write` from " << (int) records.size() << " engine renders; do not edit.\n"
             << "namespace GenerateNetWeights\n{\n    inline constexpr float values[] {\n";
        for (size_t i = 0; i < net.w.size(); ++i)
            text << (i % 8 == 0 ? "        " : " ") << juce::String (net.w[i], 6) << "f," << (i % 8 == 7 ? "\n" : "");
        text << "\n    };\n    inline constexpr size_t count = std::size (values);\n    inline constexpr const float* data = values;\n}\n";
        juce::File::getCurrentWorkingDirectory().getChildFile ("Source/GenerateNetWeights.h").replaceWithText (text);
        std::printf ("wrote Source/GenerateNetWeights.h\n");
    }
    return 0;
}

int runGenerateNetScreen (const juce::StringArray&)
{
    // Does screening help? For 24 profiles (two per genre), 200 candidates each,
    // all rendered and judged as Generate does (fit, then size + interest against
    // the designed sound); the net's 24 best-predicted against 24 at random and
    // the true best 24: their mean true score and how many fit.
    GenerateNet::Net net;
    {
        juce::FileInputStream in (juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/net/generate_weights.bin"));
        if (! in.openedOk())
        {
            std::printf ("no weights: run gennettrain first\n");
            return 1;
        }
        net.weights.resize (GenerateNet::numWeights());
        in.read (net.weights.data(), (int) (sizeof (float) * net.weights.size()));
    }
    ZeroEngineProcessor engine;
    engine.stopProfileSync();
    double sumNet = 0.0, sumRandom = 0.0, sumBest = 0.0;
    int fitNet = 0, fitRandom = 0, fitBest = 0;
    for (int g = 0; g < 12; ++g)
        for (const auto t : { g % 6, (g + 3) % 6 })
        {
            juce::Random random (1000 + g * 10 + t);
            const auto designed = ZeroEngineProcessor::measureForTools (engine, g, t, makeProfile (g, t));
            struct C { float predicted, truth; bool fits; };
            std::vector<C> cs;
            for (int i = 0; i < 200; ++i)
            {
                const auto p = generateProfile (g, t, random);
                const auto predictedTraits = GenerateNet::traitsOf (net.run (GenerateNet::featuresOf (p, g, t)), t);
                auto predictedFits = ZeroEngineProcessor::strayFrom (designed, predictedTraits, t, false) <= 1.0f;
                const auto m = ZeroEngineProcessor::measureForTools (engine, g, t, p);
                const auto fits = ZeroEngineProcessor::strayFrom (designed, m, t, false) <= 1.0f;
                cs.push_back ({ (predictedFits ? 0.0f : -10.0f) + ZeroEngineProcessor::sizeAndInterest (designed, predictedTraits, t),
                                fits ? ZeroEngineProcessor::sizeAndInterest (designed, m, t) : -4.0f, fits });
            }
            auto top = [&] (auto key)
            {
                auto sorted = cs;
                std::sort (sorted.begin(), sorted.end(), [&] (const C& a, const C& b) { return key (a) > key (b); });
                double s = 0.0;
                int f = 0;
                for (int i = 0; i < 24; ++i)
                {
                    s += sorted[(size_t) i].truth / 24.0;
                    f += sorted[(size_t) i].fits;
                }
                return std::pair { s, f };
            };
            const auto [nScore, nFit] = top ([] (const C& c) { return c.predicted; });
            const auto [bScore, bFit] = top ([] (const C& c) { return c.truth; });
            double rScore = 0.0;
            int rFit = 0;
            for (int i = 0; i < 24; ++i)
            {
                rScore += cs[(size_t) i].truth / 24.0;
                rFit += cs[(size_t) i].fits;
            }
            std::printf ("%-12s %-6s net %+5.2f (%2d fit)  random %+5.2f (%2d fit)  best %+5.2f (%2d fit)\n", genreNames[g].toRawUTF8(),
                         typeNames[t].toRawUTF8(), nScore, nFit, rScore, rFit, bScore, bFit);
            sumNet += nScore; sumRandom += rScore; sumBest += bScore;
            fitNet += nFit; fitRandom += rFit; fitBest += bFit;
        }
    std::printf ("\nmean true score of 24 picked: net %+.2f, random %+.2f, true best %+.2f; fitting: %d / %d / %d of 576\n",
                 sumNet / 24.0, sumRandom / 24.0, sumBest / 24.0, fitNet, fitRandom, fitBest);
    return 0;
}
