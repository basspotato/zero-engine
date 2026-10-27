#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Measurements on the four dimensions that explain most mix-to-mix variation
// (Wilson & Fazenda): loudness/dynamics, brightness, bass and width.
struct Descriptors
{
    double lufs = -100;        // integrated loudness, ITU-R BS.1770 (K-weighted, gated)
    double peakDb = -100;      // sample peak, dBFS
    double plr = 0;            // peak-to-loudness ratio, dB: lower = more squashed
    double centroidHz = 0;     // spectral centroid of the mid signal
    double highPct = 0;        // % of energy above 4 kHz
    double bassPct = 0;        // % of energy below 150 Hz
    double subPct = 0;         // % of energy below 60 Hz
    double sidePct = 0;        // side share of energy: 0 = mono, 50 = uncorrelated
    double correlation = 1;    // L/R correlation, -1..1
    double seconds = 0;        // length of the analysed audio
    double pitchHz = 0;        // median fundamental (YIN), 0 when unpitched or unclear
};

Descriptors analyse (const juce::AudioBuffer<float>& audio, double sampleRate);

// Display order and labels for tables and CSVs.
struct DescriptorField
{
    const char* name;
    const char* unit;
    double Descriptors::* member;
};

inline constexpr DescriptorField descriptorFields[] {
    { "lufs",        "LUFS", &Descriptors::lufs },
    { "peak_db",     "dB",   &Descriptors::peakDb },
    { "plr_db",      "dB",   &Descriptors::plr },
    { "centroid_hz", "Hz",   &Descriptors::centroidHz },
    { "high_pct",    "%",    &Descriptors::highPct },
    { "bass_pct",    "%",    &Descriptors::bassPct },
    { "sub_pct",     "%",    &Descriptors::subPct },
    { "side_pct",    "%",    &Descriptors::sidePct },
    { "correlation", "",     &Descriptors::correlation },
    { "pitch_hz",    "Hz",   &Descriptors::pitchHz },
};
