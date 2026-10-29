#pragma once

#include <juce_core/juce_core.h>
#include "SoundTraits.h"
#include <array>
#include <vector>

// The owner's taste in Generate's results, learned from what they do (no
// ratings): a favourite saved beats the results shown before it; a result
// played a while beats one clicked past without a note. A Bradley-Terry model:
// P(a beats b) = sigmoid(w . (f(a) - f(b))) on each sound's traits, fitted
// with a Gaussian prior (MAP, Newton) whose Laplace approximation gives the
// uncertainty Thompson sampling explores with. Message thread only; saved in
// %APPDATA%/Zero Engine/Taste.xml so it keeps learning across sessions.
class TasteModel
{
public:
    static constexpr int numFeatures = SoundTraits::count;
    using Features = std::array<float, numFeatures>;

    // A sound's traits in the spreads of the owner's sounds of its type
    // (brightness in octaves), clipped to +-3: comparable across types.
    static Features featuresOf (const SoundTraits&, int type);

    void addWin (const Features& winner, const Features& loser);
    int judgments() const noexcept { return (int) pairs.size(); }

    // One plausible taste, drawn from what has been learned so far: unsure
    // early (draws vary, so Generate explores), surer with every judgment.
    Features sampleWeights (juce::Random&) const;
    Features meanWeights() const noexcept { return mean; }

    void load (const juce::File&);
    void save (const juce::File&) const;

private:
    void fit();
    std::vector<std::pair<Features, Features>> pairs; // (winner, loser)
    Features mean {};
    std::array<Features, numFeatures> choleskyOfCovariance {}; // lower triangle
};

// Taste in the settings themselves (which wavetable, filter and LFO shape,
// the searched settings, the genre), which TasteModel can't see: it knows only
// what a sound measures. The same Bradley-Terry fit, on the neural judge's
// inputs (GenerateNet::featuresOf), from the same judgments plus each heart
// beating plain draws of its own genre and type. MAP only (TasteModel's draws
// already explore); fitted lazily, on Generate's worker, so a click stays
// instant. Saved in %APPDATA%/Zero Engine/TasteSettings.xml.
class SettingsTaste
{
public:
    using Features = std::vector<float>;

    void addWin (const Features& winner, const Features& loser);
    void setHeartWins (std::vector<std::pair<Features, Features>>);
    int judgments() const noexcept { return (int) (pairs.size() + heartWins.size()); }

    // w . f, by the fit as of the last refit() (0 before any).
    float scoreOf (const Features&) const noexcept;
    void refit(); // if anything changed since the last fit
    const std::vector<float>& weights() const noexcept { return w; }

    void load (const juce::File&);
    void save (const juce::File&) const;

private:
    std::vector<std::pair<Features, Features>> pairs, heartWins;
    std::vector<float> w;
    bool stale = false;
};
