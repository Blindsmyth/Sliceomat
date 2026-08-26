#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <juce_dsp/juce_dsp.h>
#include "Filters.h"

namespace sliceomat
{

inline float midiToHz(float midiNote)
{
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

// Linear attack, exponential decay. Retriggers from the current level.
struct AttackDecayEnvelope
{
    void prepare(double sampleRateToUse)
    {
        sampleRate = sampleRateToUse;
        updateRates();
    }

    void setTimes(float attackSeconds, float decaySeconds)
    {
        attackSec = std::max(0.0001f, attackSeconds);
        decaySec = std::max(0.001f, decaySeconds);
        updateRates();
    }

    void trigger()
    {
        attacking = true;
    }

    float process()
    {
        if (attacking)
        {
            value += attackInc;
            if (value >= 1.0f)
            {
                value = 1.0f;
                attacking = false;
            }
        }
        else
        {
            value *= decayCoeff;
            if (value < 1.0e-6f)
                value = 0.0f;
        }

        return value;
    }

    void reset()
    {
        value = 0.0f;
        attacking = false;
    }

private:
    void updateRates()
    {
        if (sampleRate <= 0.0)
            return;

        attackInc = 1.0f / (float)(attackSec * sampleRate);
        // Decay to -60 dB over decaySec
        decayCoeff = std::exp(-std::log(1000.0f) / (float)(decaySec * sampleRate));
    }

    double sampleRate = 44100.0;
    float attackSec = 0.001f;
    float decaySec = 0.5f;
    float attackInc = 0.0f;
    float decayCoeff = 0.999f;
    float value = 0.0f;
    bool attacking = false;
};

struct WhiteNoise
{
    float next()
    {
        state = state * 1664525u + 1013904223u;
        return (float)(int32_t)state * (1.0f / 2147483648.0f);
    }

    void reset(uint32_t seed = 0xA341316Cu) { state = seed; }

private:
    uint32_t state = 0xA341316Cu;
};

struct Engine
{
    static constexpr float filterEnvSpanSemitones = 64.0f;

    void prepare(double sampleRateToUse, int samplesPerBlock)
    {
        sampleRate = sampleRateToUse;
        envelope.prepare(sampleRate);
        noiseL.reset(0xA341316Cu);
        noiseR.reset(0xC2B2AE35u);
        filters.prepare(sampleRate, samplesPerBlock);

        const float smoothSec = 0.02f;
        gain1.reset(sampleRate, smoothSec);
        gain2.reset(sampleRate, smoothSec);
        volMod.reset(sampleRate, smoothSec);
        pitch.reset(sampleRate, smoothSec);
        reso.reset(sampleRate, smoothSec);
        filterEnvMod.reset(sampleRate, smoothSec);

        filters.reset();
        envelope.reset();
    }

    void reset()
    {
        envelope.reset();
        filters.reset();
    }

    void setTargets(float g1, float g2, float attack, float decay,
                    float pitchMidi, float resonance, float vol, float fenv,
                    int filterType)
    {
        gain1.setTargetValue(g1);
        gain2.setTargetValue(g2);
        volMod.setTargetValue(vol);
        pitch.setTargetValue(pitchMidi);
        reso.setTargetValue(resonance);
        filterEnvMod.setTargetValue(fenv);
        envelope.setTimes(attack, decay);
        filters.setType((FilterType) juce::jlimit(
            0, (int) FilterType::NumTypes - 1, filterType));
    }

    void trigger() { envelope.trigger(); }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(2, buffer.getNumChannels());
        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        const float nyquistLimit = (float)sampleRate * 0.45f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = envelope.process();
            const float g1 = gain1.getNextValue();
            const float g2 = gain2.getNextValue();
            const float vm = volMod.getNextValue();
            const float p = pitch.getNextValue();
            const float r = reso.getNextValue();
            const float fm = filterEnvMod.getNextValue();

            const float vcaGain = 1.0f + (env - 1.0f) * vm; // lerp(1, env, volMod)
            const float nL = noiseL.next();
            const float nR = noiseR.next();

            const float cutoffMidi = p + env * fm * filterEnvSpanSemitones;
            const float cutoffHz = juce::jlimit(20.0f, nyquistLimit, midiToHz(cutoffMidi));
            const float q = mapResoToQ(filters.getType(), r);

            const float inL = left[i];
            left[i] = filters.process(0, (nL * g1 + inL * g2) * vcaGain, cutoffHz, q);

            if (right != nullptr)
            {
                const float inR = right[i];
                right[i] = filters.process(1, (nR * g1 + inR * g2) * vcaGain, cutoffHz, q);
            }
        }
    }

private:
    double sampleRate = 44100.0;
    AttackDecayEnvelope envelope;
    WhiteNoise noiseL, noiseR;
    FilterBank filters;
    juce::SmoothedValue<float> gain1, gain2, volMod, pitch, reso, filterEnvMod;
};

} // namespace sliceomat
