#pragma once
#include <juce_core/juce_core.h>

class LFO
{
public:
    enum class Waveform { Sine, Triangle, Saw, Square, Random };

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        phase = 0.0f;
        randomValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    void setFrequency(float freq)
    {
        inc = freq / currentSampleRate;
    }

    void setWaveform(Waveform wave)
    {
        waveform = wave;
    }

    float getNextSample()
    {
        float value = 0.0f;
        
        switch (waveform)
        {
            case Waveform::Sine:
                value = std::sin(juce::MathConstants<float>::twoPi * phase);
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

private:
    double currentSampleRate = 44100.0;
    float phase = 0.0f;
    float inc = 0.0f;
    float randomValue = 0.0f;
    Waveform waveform = Waveform::Sine;
};
