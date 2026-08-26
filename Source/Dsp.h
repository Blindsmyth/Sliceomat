#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <juce_dsp/juce_dsp.h>

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
        noise.reset();

        juce::dsp::ProcessSpec spec{sampleRate, (juce::uint32)samplesPerBlock, 1};
        lpfL.prepare(spec);
        lpfR.prepare(spec);
        lpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

        const float smoothSec = 0.02f;
        gain1.reset(sampleRate, smoothSec);
        gain2.reset(sampleRate, smoothSec);
        volMod.reset(sampleRate, smoothSec);
        pitch.reset(sampleRate, smoothSec);
        reso.reset(sampleRate, smoothSec);
        filterEnvMod.reset(sampleRate, smoothSec);

        lpfL.reset();
        lpfR.reset();
        envelope.reset();
        lastCutoff = -1.0f;
        lastReso = -1.0f;
    }

    void reset()
    {
        envelope.reset();
        lpfL.reset();
        lpfR.reset();
        lastCutoff = -1.0f;
        lastReso = -1.0f;
    }

    void setTargets(float g1, float g2, float attack, float decay,
                    float pitchMidi, float resonance, float vol, float fenv)
    {
        gain1.setTargetValue(g1);
        gain2.setTargetValue(g2);
        volMod.setTargetValue(vol);
        pitch.setTargetValue(pitchMidi);
        reso.setTargetValue(resonance);
        filterEnvMod.setTargetValue(fenv);
        envelope.setTimes(attack, decay);
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
            const float n = noise.next();

            const float cutoffMidi = p + env * fm * filterEnvSpanSemitones;
            const float cutoffHz = juce::jlimit(20.0f, nyquistLimit, midiToHz(cutoffMidi));
            const float q = juce::jlimit(0.05f, 0.98f, r);

            if (std::abs(cutoffHz - lastCutoff) > 0.1f || std::abs(q - lastReso) > 0.001f)
            {
                lpfL.setCutoffFrequency(cutoffHz);
                lpfR.setCutoffFrequency(cutoffHz);
                lpfL.setResonance(q);
                lpfR.setResonance(q);
                lastCutoff = cutoffHz;
                lastReso = q;
            }

            const float inL = left[i];
            left[i] = lpfL.processSample(0, (n * g1 + inL * g2) * vcaGain);

            if (right != nullptr)
            {
                const float inR = right[i];
                right[i] = lpfR.processSample(0, (n * g1 + inR * g2) * vcaGain);
            }
        }
    }

private:
    double sampleRate = 44100.0;
    AttackDecayEnvelope envelope;
    WhiteNoise noise;
    juce::dsp::StateVariableTPTFilter<float> lpfL, lpfR;
    juce::SmoothedValue<float> gain1, gain2, volMod, pitch, reso, filterEnvMod;
    float lastCutoff = -1.0f;
    float lastReso = -1.0f;
};

} // namespace sliceomat
