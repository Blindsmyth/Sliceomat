#pragma once

#include "faust/DiodeLadder.hpp"
#include "faust/Korg35HPF.hpp"
#include "faust/Korg35LPF.hpp"
#include "faust/MoogHalfLadder.hpp"
#include "faust/MoogLadder.hpp"
#include "faust/Oberheim.hpp"

#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace sliceomat
{

enum class FilterType
{
    SvfLpf = 0,
    DiodeLadder,
    Korg35Lpf,
    Korg35Hpf,
    MoogLadder,
    MoogHalfLadder,
    OberheimLp,
    OberheimHp,
    OberheimBp,
    OberheimNotch,
    NumTypes
};

inline juce::StringArray filterTypeNames()
{
    return {
        "SVF LPF",
        "Diode Ladder",
        "Korg 35 LPF",
        "Korg 35 HPF",
        "Moog Ladder",
        "Moog Half Ladder",
        "Oberheim LP",
        "Oberheim HP",
        "Oberheim BP",
        "Oberheim Notch"
    };
}

inline float mapResoToQ(FilterType type, float reso01)
{
    reso01 = juce::jlimit(0.0f, 1.0f, reso01);

    switch (type)
    {
        case FilterType::SvfLpf:
            return juce::jmap(reso01, 0.05f, 0.98f);
        case FilterType::DiodeLadder:
        case FilterType::MoogLadder:
        case FilterType::MoogHalfLadder:
            return juce::jmap(reso01, 0.7072f, 25.0f);
        case FilterType::Korg35Lpf:
        case FilterType::Korg35Hpf:
        case FilterType::OberheimLp:
        case FilterType::OberheimHp:
        case FilterType::OberheimBp:
        case FilterType::OberheimNotch:
            return juce::jmap(reso01, 0.5f, 10.0f);
        case FilterType::NumTypes:
            return juce::jmap(reso01, 0.05f, 0.98f);
    }
}

template <typename Filter>
inline float processFaustMono(Filter& filter, float input, float cutoffHz, float q)
{
    filter.set_cutoff(cutoffHz);
    filter.set_q(q);
    float output = 0.0f;
    filter.process(&input, &output, 1);
    return output;
}

inline float processOberheim(Oberheim& filter, float input, float cutoffHz, float q, int outIndex)
{
    filter.set_cutoff(cutoffHz);
    filter.set_q(q);
    float bsf = 0.0f, bpf = 0.0f, hpf = 0.0f, lpf = 0.0f;
    filter.process(&input, &bsf, &bpf, &hpf, &lpf, 1);
    switch (outIndex)
    {
        case 0: return bsf; // Notch
        case 1: return bpf;
        case 2: return hpf;
        default: return lpf;
    }
}

struct FilterBank
{
    void prepare(double sampleRateToUse, int samplesPerBlock)
    {
        juce::dsp::ProcessSpec spec{sampleRateToUse, (juce::uint32) samplesPerBlock, 1};
        svf[0].prepare(spec);
        svf[1].prepare(spec);
        svf[0].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        svf[1].setType(juce::dsp::StateVariableTPTFilterType::lowpass);

        const float sr = (float) sampleRateToUse;
        for (int ch = 0; ch < 2; ++ch)
        {
            diode[ch].init(sr);
            korgLp[ch].init(sr);
            korgHp[ch].init(sr);
            moog[ch].init(sr);
            moogHalf[ch].init(sr);
            oberheim[ch].init(sr);
        }

        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            svf[ch].reset();
            diode[ch].clear();
            korgLp[ch].clear();
            korgHp[ch].clear();
            moog[ch].clear();
            moogHalf[ch].clear();
            oberheim[ch].clear();
        }
        lastCutoff = -1.0f;
        lastQ = -1.0f;
    }

    void setType(FilterType type)
    {
        if (type == currentType)
            return;

        currentType = type;
        reset();
    }

    FilterType getType() const { return currentType; }

    float process(int channel, float input, float cutoffHz, float q)
    {
        const int ch = juce::jlimit(0, 1, channel);

        switch (currentType)
        {
            case FilterType::SvfLpf:
                if (std::abs(cutoffHz - lastCutoff) > 0.1f || std::abs(q - lastQ) > 0.001f)
                {
                    svf[0].setCutoffFrequency(cutoffHz);
                    svf[1].setCutoffFrequency(cutoffHz);
                    svf[0].setResonance(q);
                    svf[1].setResonance(q);
                    lastCutoff = cutoffHz;
                    lastQ = q;
                }
                return svf[ch].processSample(0, input);

            case FilterType::DiodeLadder:
                return processFaustMono(diode[ch], input, cutoffHz, q);
            case FilterType::Korg35Lpf:
                return processFaustMono(korgLp[ch], input, cutoffHz, q);
            case FilterType::Korg35Hpf:
                return processFaustMono(korgHp[ch], input, cutoffHz, q);
            case FilterType::MoogLadder:
                return processFaustMono(moog[ch], input, cutoffHz, q);
            case FilterType::MoogHalfLadder:
                return processFaustMono(moogHalf[ch], input, cutoffHz, q);
            case FilterType::OberheimNotch:
                return processOberheim(oberheim[ch], input, cutoffHz, q, 0);
            case FilterType::OberheimBp:
                return processOberheim(oberheim[ch], input, cutoffHz, q, 1);
            case FilterType::OberheimHp:
                return processOberheim(oberheim[ch], input, cutoffHz, q, 2);
            case FilterType::OberheimLp:
                return processOberheim(oberheim[ch], input, cutoffHz, q, 3);
            case FilterType::NumTypes:
                return input;
        }
    }

private:
    FilterType currentType = FilterType::SvfLpf;
    juce::dsp::StateVariableTPTFilter<float> svf[2];
    DiodeLadder diode[2];
    Korg35LPF korgLp[2];
    Korg35HPF korgHp[2];
    MoogLadder moog[2];
    MoogHalfLadder moogHalf[2];
    Oberheim oberheim[2];
    float lastCutoff = -1.0f;
    float lastQ = -1.0f;
};

} // namespace sliceomat
