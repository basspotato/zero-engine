#include "TasteModel.h"
#include "GenerateTargets.h"

namespace
{
    constexpr int dims = TasteModel::numFeatures;
    using Matrix = std::array<std::array<double, dims>, dims>;

    // Lower-triangular L with L Lt = a (a symmetric positive definite).
    Matrix cholesky (const Matrix& a)
    {
        Matrix l {};
        for (int i = 0; i < dims; ++i)
            for (int j = 0; j <= i; ++j)
            {
                auto sum = a[(size_t) i][(size_t) j];
                for (int k = 0; k < j; ++k)
                    sum -= l[(size_t) i][(size_t) k] * l[(size_t) j][(size_t) k];
                l[(size_t) i][(size_t) j] = i == j ? std::sqrt (juce::jmax (1.0e-12, sum)) : sum / l[(size_t) j][(size_t) j];
            }
        return l;
    }

    // Solves a x = b through a's Cholesky factor.
    std::array<double, dims> solve (const Matrix& l, std::array<double, dims> b)
    {
        for (int i = 0; i < dims; ++i)
        {
            for (int k = 0; k < i; ++k)
                b[(size_t) i] -= l[(size_t) i][(size_t) k] * b[(size_t) k];
            b[(size_t) i] /= l[(size_t) i][(size_t) i];
        }
        for (int i = dims - 1; i >= 0; --i)
        {
            for (int k = i + 1; k < dims; ++k)
                b[(size_t) i] -= l[(size_t) k][(size_t) i] * b[(size_t) k];
            b[(size_t) i] /= l[(size_t) i][(size_t) i];
        }
        return b;
    }
}

TasteModel::Features TasteModel::featuresOf (const SoundTraits& t, int type)
{
    const auto& guide = GenerateTargets::byType[(size_t) juce::jlimit (0, 5, type)];
    const auto a = t.asArray();
    Features f {};
    for (int k = 0; k < dims; ++k)
    {
        const auto z = k == 2 // brightness: octaves from the owner's median, a 0.8-octave step
                           ? std::log2 (juce::jmax (20.0f, a[2]) / juce::jmax (20.0f, guide.median[2])) / 0.8f
                           : (a[(size_t) k] - guide.median[(size_t) k]) / juce::jmax (1.0e-3f, guide.spread[(size_t) k]);
        f[(size_t) k] = juce::jlimit (-3.0f, 3.0f, z);
    }
    f[0] = 0.0f; // level: the profile's trim and the meter decide it, not taste
    return f;
}

void TasteModel::addWin (const Features& winner, const Features& loser)
{
    pairs.push_back ({ winner, loser });
    constexpr size_t kept = 400; // the most recent: taste moves
    if (pairs.size() > kept)
        pairs.erase (pairs.begin(), pairs.begin() + (std::ptrdiff_t) (pairs.size() - kept));
    fit();
}

void TasteModel::fit()
{
    // MAP under a unit Gaussian prior on each weight: Newton steps on the
    // log-posterior; the Hessian there is the Laplace approximation's precision.
    std::array<double, dims> w {};
    Matrix precision {};
    for (int iteration = 0; iteration < 12; ++iteration)
    {
        std::array<double, dims> gradient {};
        precision = {};
        for (int i = 0; i < dims; ++i)
        {
            gradient[(size_t) i] = -w[(size_t) i];
            precision[(size_t) i][(size_t) i] = 1.0;
        }
        for (const auto& [a, b] : pairs)
        {
            std::array<double, dims> d {};
            double margin = 0.0;
            for (int i = 0; i < dims; ++i)
            {
                d[(size_t) i] = (double) a[(size_t) i] - b[(size_t) i];
                margin += w[(size_t) i] * d[(size_t) i];
            }
            const auto p = 1.0 / (1.0 + std::exp (-margin));
            for (int i = 0; i < dims; ++i)
            {
                gradient[(size_t) i] += (1.0 - p) * d[(size_t) i];
                for (int j = 0; j < dims; ++j)
                    precision[(size_t) i][(size_t) j] += p * (1.0 - p) * d[(size_t) i] * d[(size_t) j];
            }
        }
        const auto step = solve (cholesky (precision), gradient);
        for (int i = 0; i < dims; ++i)
            w[(size_t) i] += step[(size_t) i];
    }
    for (int i = 0; i < dims; ++i)
        mean[(size_t) i] = (float) w[(size_t) i];

    // Covariance = precision^-1; its Cholesky factor draws samples. Column by
    // column: covariance e_j = solve (precision, e_j).
    Matrix covariance {};
    const auto l = cholesky (precision);
    for (int j = 0; j < dims; ++j)
    {
        std::array<double, dims> e {};
        e[(size_t) j] = 1.0;
        const auto column = solve (l, e);
        for (int i = 0; i < dims; ++i)
            covariance[(size_t) i][(size_t) j] = column[(size_t) i];
    }
    const auto factor = cholesky (covariance);
    for (int i = 0; i < dims; ++i)
        for (int j = 0; j < dims; ++j)
            choleskyOfCovariance[(size_t) i][(size_t) j] = (float) factor[(size_t) i][(size_t) j];
}

TasteModel::Features TasteModel::sampleWeights (juce::Random& random) const
{
    if (pairs.empty())
        return {};
    Features z {}, w = mean;
    for (auto& v : z)
    {
        // A standard normal from two uniforms (Box-Muller).
        const auto u1 = juce::jmax (1.0e-9, random.nextDouble()), u2 = random.nextDouble();
        v = (float) (std::sqrt (-2.0 * std::log (u1)) * std::cos (juce::MathConstants<double>::twoPi * u2));
    }
    for (int i = 0; i < dims; ++i)
        for (int j = 0; j <= i; ++j)
            w[(size_t) i] += choleskyOfCovariance[(size_t) i][(size_t) j] * z[(size_t) j];
    return w;
}

void TasteModel::load (const juce::File& file)
{
    pairs.clear();
    if (const auto xml = juce::XmlDocument::parse (file))
        for (const auto* pair : xml->getChildIterator())
        {
            const auto a = juce::StringArray::fromTokens (pair->getStringAttribute ("winner"), ",", "");
            const auto b = juce::StringArray::fromTokens (pair->getStringAttribute ("loser"), ",", "");
            if (a.size() != dims || b.size() != dims)
                continue;
            Features fa {}, fb {};
            for (int i = 0; i < dims; ++i)
            {
                fa[(size_t) i] = a[i].getFloatValue();
                fb[(size_t) i] = b[i].getFloatValue();
            }
            pairs.push_back ({ fa, fb });
        }
    if (! pairs.empty())
        fit();
}

void TasteModel::save (const juce::File& file) const
{
    juce::XmlElement xml ("Taste");
    auto join = [] (const Features& f)
    {
        juce::StringArray parts;
        for (const auto v : f)
            parts.add (juce::String (v, 3));
        return parts.joinIntoString (",");
    };
    for (const auto& [a, b] : pairs)
    {
        auto* pair = xml.createNewChildElement ("Pick");
        pair->setAttribute ("winner", join (a));
        pair->setAttribute ("loser", join (b));
    }
    file.getParentDirectory().createDirectory();
    xml.writeTo (file);
}
