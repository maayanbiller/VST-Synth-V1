#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

class LFO
{
public:
    enum class Waveform { Sine, Triangle, Saw, Square, Random };

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        phase = 0.0f;
        randomValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    void setFrequency(float freq) noexcept
    {
        inc = static_cast<float>(freq / currentSampleRate);
    }

    void setWaveform(Waveform wave) noexcept
    {
        waveform = wave;
    }

    static inline float fastSin2Pi(float phase01) noexcept
    {
        float pShift = phase01 + 0.25f;
        if (pShift >= 1.0f) pShift -= 1.0f;
        float x = pShift - 0.5f;
        float u = 0.25f - std::fabs(x);
        float u2 = u * u;
        return u * (6.283185307f + u2 * (-41.34170224f + u2 * (81.60524928f + u2 * (-76.70585975f + u2 * 42.0586939f))));
    }

    inline float getNextSample() noexcept
    {
        float value = 0.0f;
        
        switch (waveform)
        {
            case Waveform::Sine:
                value = fastSin2Pi(phase);
                break;
            case Waveform::Triangle:
                value = -1.0f + (2.0f * phase);
                value = 2.0f * (std::fabs(value) - 0.5f);
                break;
            case Waveform::Saw:
                value = (2.0f * phase) - 1.0f;
                break;
            case Waveform::Square:
                value = phase < 0.5f ? 1.0f : -1.0f;
                break;
            case Waveform::Random:
                value = randomValue;
                break;
        }

        phase += inc;
        if (phase >= 1.0f) 
        {
            phase -= 1.0f;
            if (waveform == Waveform::Random)
                randomValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        }

        return value;
    }

    inline void advance(int numSamples) noexcept
    {
        if (numSamples <= 0) return;
        phase += inc * static_cast<float>(numSamples);
        while (phase >= 1.0f)
        {
            phase -= 1.0f;
            if (waveform == Waveform::Random)
                randomValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        }
        while (phase < 0.0f)
            phase += 1.0f;
    }

private:
    double currentSampleRate = 44100.0;
    float phase = 0.0f;
    float inc = 0.0f;
    float randomValue = 0.0f;
    Waveform waveform = Waveform::Sine;
};

