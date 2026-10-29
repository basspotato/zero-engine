#include "MatchNetTrain.h"
#include "../../Source/MatchNet.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"
#include <atomic>
#include <random>
#include <thread>

namespace
{
    constexpr double rate = 48000.0;
    constexpr int block = 512;

    struct Record
    {
        MatchNet::Features features {};
        int preset = 0, table = 0;
        std::array<float, MatchNet::numUnits> units {};
    };

    // One note through an engine reused between renders: settle, play, release.
    juce::AudioBuffer<float> renderNote (ZeroEngineProcessor& engine, int genre, int type, const Profile& profile, int note, double held)
    {
        engine.resetForRender();
        engine.loadProfile (genre, type, profile);
        juce::AudioBuffer<float> out (2, (int) (2.0 * rate)), chunk (2, block);
        juce::MidiBuffer midi;
        for (int i = 0; i < 10; ++i)
        {
            chunk.clear();
            engine.processBlock (chunk, midi);
        }
        const auto off = (int) (held * rate);
        for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
        {
            midi.clear();
            if (pos == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            if (off >= pos && off < pos + block)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), off - pos);
            chunk.clear();
            engine.processBlock (chunk, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, chunk, ch, 0, block);
        }
        return out;
    }

    Record makeRecord (ZeroEngineProcessor& engine, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> unit (0.0f, 1.0f);
        std::normal_distribution<float> nudge (0.0f, 0.25f);
        Record r;
        r.preset = (int) (unit (rng) * MatchNet::numPresets) % MatchNet::numPresets;
        const auto genre = r.preset / 6, type = r.preset % 6;
        juce::Random random ((juce::int64) rng());
        auto profile = unit (rng) < 0.5f ? makeProfile (genre, type) : generateProfile (genre, type, random);

        // Its settings moved now and then, so the net learns the space around
        // each preset, not just the presets.
        auto units = ReferenceMatch::unitsOf (profile);
        for (auto& u : units)
            if (unit (rng) < 0.6f)
                u = juce::jlimit (0.0f, 1.0f, u + nudge (rng));
        profile = ReferenceMatch::withUnitsOf (profile, units);
        if (unit (rng) < 0.4f || (int) profile.waveTable >= MatchNet::numTables)
            profile.waveTable = (float) ((int) (unit (rng) * MatchNet::numTables) % MatchNet::numTables);
        r.table = (int) profile.waveTable;
        r.units = ReferenceMatch::unitsOf (profile);

        const auto low = type == 0 ? 28 : 48, high = type == 0 ? 52 : 84;
        const auto note = low + (int) (unit (rng) * (float) (high - low + 1)) % (high - low + 1);
        const auto held = 0.2 + 1.4 * unit (rng);
        const auto print = SoundPrint::of (renderNote (engine, genre, type, profile, note, held), rate, true);
        r.features = MatchNet::featuresOf (print);
        return r;
    }

    juce::File dataFile (const juce::StringArray& args, int index)
    {
        return args.size() > index && ! args[index].startsWith ("--")
                   ? juce::File::getCurrentWorkingDirectory().getChildFile (args[index])
                   : juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/net/data.bin");
    }
}

int runNetData (const juce::StringArray& args)
{
    const auto count = args.size() > 1 ? args[1].getIntValue() : 20000;
    const auto out = dataFile (args, 2);
    const auto seed = args.size() > 3 ? (unsigned) args[3].getIntValue() : 1u;
    out.getParentDirectory().createDirectory();

    std::vector<Record> records ((size_t) count);
    std::atomic<int> next { 0 }, done { 0 };
    const auto threads = (int) juce::jlimit (1u, 8u, std::thread::hardware_concurrency() - 1);
    std::vector<std::thread> pool;
    const auto began = juce::Time::getMillisecondCounter();
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&, t]
        {
            ZeroEngineProcessor engine;
            engine.stopProfileSync();
            engine.setPlayConfigDetails (0, 2, rate, block);
            engine.prepareToPlay (rate, block);
            engine.setLightRender (true);
            std::mt19937 rng (seed * 7919u + (unsigned) t);
            for (int i = next++; i < count; i = next++)
            {
                records[(size_t) i] = makeRecord (engine, rng);
                if (const auto d = ++done; d % 1000 == 0)
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
    for (const auto& r : records)
    {
        stream.write (r.features.data(), sizeof (r.features));
        stream.writeInt (r.preset);
        stream.writeInt (r.table);
        stream.write (r.units.data(), sizeof (r.units));
    }
    std::printf ("wrote %d records to %s\n", count, out.getFullPathName().toRawUTF8());
    return 0;
}

int runNetTrain (const juce::StringArray& args)
{
    using namespace MatchNet;
    const auto in = dataFile (args, 1);
    const auto epochs = args.size() > 2 && ! args[2].startsWith ("--") ? args[2].getIntValue() : 30;
    const auto write = args.contains ("--write");

    juce::FileInputStream stream (in);
    if (! stream.openedOk())
    {
        std::printf ("no data at %s (run netdata first)\n", in.getFullPathName().toRawUTF8());
        return 1;
    }
    std::vector<Record> records ((size_t) stream.readInt());
    for (auto& r : records)
    {
        stream.read (r.features.data(), sizeof (r.features));
        r.preset = stream.readInt();
        r.table = stream.readInt();
        stream.read (r.units.data(), sizeof (r.units));
    }
    std::mt19937 rng (5);
    std::shuffle (records.begin(), records.end(), rng);
    const auto held = records.size() / 10; // the last tenth checks how well it does on sounds it never saw
    const auto trainCount = records.size() - held;
    std::printf ("%d records: %d to train on, %d held out\n", (int) records.size(), (int) trainCount, (int) held);

    // The layers (see MatchNet.h for the layout), He-initialised.
    Net net;
    net.weights.resize (numWeights);
    {
        std::normal_distribution<float> gauss (0.0f, 1.0f);
        size_t at = 0;
        auto fill = [&] (int outputs, int inputs)
        {
            const auto scale = std::sqrt (2.0f / (float) inputs);
            for (int i = 0; i < outputs * inputs; ++i)
                net.weights[at++] = gauss (rng) * scale;
            for (int i = 0; i < outputs; ++i)
                net.weights[at++] = 0.0f;
        };
        fill (numHidden, numInputs);
        fill (numHidden, numHidden);
        fill (numOutputs, numHidden);
    }
    const size_t w1 = 0, b1 = w1 + (size_t) numHidden * numInputs, w2 = b1 + numHidden, b2 = w2 + (size_t) numHidden * numHidden,
                 w3 = b2 + numHidden, b3 = w3 + (size_t) numOutputs * numHidden;

    // One sample forward and back: adds its gradient into grad, returns its loss.
    constexpr float unitWeight = 4.0f;
    auto backprop = [&] (const Record& r, std::vector<float>& grad)
    {
        const auto& W = net.weights;
        std::array<float, numHidden> z1 {}, h1 {}, z2 {}, h2 {};
        std::array<float, numOutputs> o {};
        for (int j = 0; j < numHidden; ++j)
        {
            auto s = W[b1 + (size_t) j];
            for (int i = 0; i < numInputs; ++i)
                s += W[w1 + (size_t) j * numInputs + (size_t) i] * r.features[(size_t) i];
            z1[(size_t) j] = s;
            h1[(size_t) j] = juce::jmax (0.0f, s);
        }
        for (int j = 0; j < numHidden; ++j)
        {
            auto s = W[b2 + (size_t) j];
            for (int i = 0; i < numHidden; ++i)
                s += W[w2 + (size_t) j * numHidden + (size_t) i] * h1[(size_t) i];
            z2[(size_t) j] = s;
            h2[(size_t) j] = juce::jmax (0.0f, s);
        }
        for (int k = 0; k < numOutputs; ++k)
        {
            auto s = W[b3 + (size_t) k];
            for (int i = 0; i < numHidden; ++i)
                s += W[w3 + (size_t) k * numHidden + (size_t) i] * h2[(size_t) i];
            o[(size_t) k] = s;
        }

        // Output gradients: softmax cross-entropy for preset and table, squared
        // error on the sigmoid of each unit.
        std::array<float, numOutputs> d {};
        float loss = 0.0f;
        auto softmaxPart = [&] (int from, int n, int truth)
        {
            float top = -1.0e30f, sum = 0.0f;
            for (int k = 0; k < n; ++k)
                top = juce::jmax (top, o[(size_t) (from + k)]);
            for (int k = 0; k < n; ++k)
                sum += std::exp (o[(size_t) (from + k)] - top);
            for (int k = 0; k < n; ++k)
            {
                const auto p = std::exp (o[(size_t) (from + k)] - top) / sum;
                d[(size_t) (from + k)] = p - (k == truth ? 1.0f : 0.0f);
                if (k == truth)
                    loss -= std::log (juce::jmax (1.0e-9f, p));
            }
        };
        softmaxPart (0, numPresets, r.preset);
        softmaxPart (numPresets, numTables, r.table);
        for (int u = 0; u < numUnits; ++u)
        {
            const auto k = (size_t) (numPresets + numTables + u);
            const auto s = 1.0f / (1.0f + std::exp (-o[k]));
            const auto e = s - r.units[(size_t) u];
            loss += unitWeight * e * e / numUnits;
            d[k] = unitWeight * 2.0f * e * s * (1.0f - s) / numUnits;
        }

        std::array<float, numHidden> d2 {}, d1 {};
        for (int k = 0; k < numOutputs; ++k)
        {
            const auto g = d[(size_t) k];
            grad[b3 + (size_t) k] += g;
            for (int i = 0; i < numHidden; ++i)
            {
                grad[w3 + (size_t) k * numHidden + (size_t) i] += g * h2[(size_t) i];
                d2[(size_t) i] += g * W[w3 + (size_t) k * numHidden + (size_t) i];
            }
        }
        for (int j = 0; j < numHidden; ++j)
        {
            const auto g = z2[(size_t) j] > 0.0f ? d2[(size_t) j] : 0.0f;
            grad[b2 + (size_t) j] += g;
            for (int i = 0; i < numHidden; ++i)
            {
                grad[w2 + (size_t) j * numHidden + (size_t) i] += g * h1[(size_t) i];
                d1[(size_t) i] += g * W[w2 + (size_t) j * numHidden + (size_t) i];
            }
        }
        for (int j = 0; j < numHidden; ++j)
        {
            const auto g = z1[(size_t) j] > 0.0f ? d1[(size_t) j] : 0.0f;
            grad[b1 + (size_t) j] += g;
            for (int i = 0; i < numInputs; ++i)
                grad[w1 + (size_t) j * numInputs + (size_t) i] += g * r.features[(size_t) i];
        }
        return loss;
    };

    // Adam over minibatches; the batch is split across threads, each adding into
    // its own gradient.
    constexpr int batch = 64;
    constexpr float beta1 = 0.9f, beta2 = 0.999f;
    const auto threads = (int) juce::jlimit (1u, 8u, std::thread::hardware_concurrency());
    std::vector<std::vector<float>> grads ((size_t) threads, std::vector<float> (numWeights));
    std::vector<float> m (numWeights, 0.0f), v (numWeights, 0.0f);
    int step = 0;

    auto evaluate = [&]
    {
        int presetTop1 = 0, presetTop6 = 0, tableTop1 = 0;
        double unitError = 0.0;
        for (size_t i = trainCount; i < records.size(); ++i)
        {
            const auto& r = records[i];
            const auto out = net.run (r.features);
            std::vector<int> order ((size_t) numPresets);
            std::iota (order.begin(), order.end(), 0);
            std::sort (order.begin(), order.end(), [&] (int a, int b) { return out[(size_t) a] > out[(size_t) b]; });
            presetTop1 += order[0] == r.preset;
            presetTop6 += std::find (order.begin(), order.begin() + 6, r.preset) != order.begin() + 6;
            tableTop1 += (int) (std::max_element (out.begin() + numPresets, out.begin() + numPresets + numTables) - (out.begin() + numPresets)) == r.table;
            for (int u = 0; u < numUnits; ++u)
                unitError += std::abs (1.0f / (1.0f + std::exp (-out[(size_t) (numPresets + numTables + u)])) - r.units[(size_t) u]);
        }
        const auto n = (double) juce::jmax ((size_t) 1, held);
        std::printf ("    held out: preset right %.0f %% (in its top 6 %.0f %%), wavetable right %.0f %%, settings off by %.3f on average\n",
                     100.0 * presetTop1 / n, 100.0 * presetTop6 / n, 100.0 * tableTop1 / n, unitError / (n * numUnits));
    };

    for (int epoch = 0; epoch < epochs; ++epoch)
    {
        std::shuffle (records.begin(), records.begin() + (long) trainCount, rng);
        const auto lr = 1.0e-3f * (epoch < epochs * 2 / 3 ? 1.0f : 0.3f);
        double epochLoss = 0.0;
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
                        losses[(size_t) t] += backprop (records[i], g);
                });
            for (auto& t : pool)
                t.join();
            ++step;
            const auto c1 = 1.0f - std::pow (beta1, (float) step), c2 = 1.0f - std::pow (beta2, (float) step);
            for (size_t w = 0; w < numWeights; ++w)
            {
                float g = 0.0f;
                for (const auto& gt : grads)
                    g += gt[w];
                g /= (float) batch;
                m[w] = beta1 * m[w] + (1.0f - beta1) * g;
                v[w] = beta2 * v[w] + (1.0f - beta2) * g * g;
                net.weights[w] -= lr * (m[w] / c1) / (std::sqrt (v[w] / c2) + 1.0e-8f);
            }
            for (auto l : losses)
                epochLoss += l;
        }
        std::printf ("epoch %d: loss %.3f\n", epoch + 1, epochLoss / (double) trainCount);
        if ((epoch + 1) % 5 == 0 || epoch + 1 == epochs)
            evaluate();
    }

    if (write)
    {
        const auto header = juce::File::getCurrentWorkingDirectory().getChildFile ("Source/MatchNetWeights.h");
        juce::String text;
        text << "#pragma once\n\n#include <cstddef>\n\n"
             << "// The trained Reference Match network (MatchNet.h). Written by\n"
             << "// `ZeroAnalyze nettrain --write` from " << (int) records.size() << " engine renders; do not edit.\n"
             << "namespace MatchNetWeights\n{\n    inline constexpr float values[] {\n";
        for (size_t w = 0; w < numWeights; ++w)
            text << (w % 8 == 0 ? "        " : " ") << juce::String (net.weights[w], 6) << "f," << (w % 8 == 7 ? "\n" : "");
        text << "\n    };\n    inline constexpr size_t count = std::size (values);\n    inline constexpr const float* data = values;\n}\n";
        header.replaceWithText (text);
        std::printf ("wrote %s\n", header.getFullPathName().toRawUTF8());
    }
    return 0;
}
