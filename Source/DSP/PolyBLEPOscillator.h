#pragma once
#include <juce_core/juce_core.h>

class PolyBLEPOscillator
{
public:
    enum class Waveform { Sine, Triangle, Saw, Square };

    PolyBLEPOscillator() = default;
    ~PolyBLEPOscillator() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        inc = 0.0;
        phase = 0.0;
    }

    void setFrequency(float freq)
    {
        inc = freq / currentSampleRate;
    }

    void setWaveform(Waveform wave)
    {
        waveform = wave;
    }

    void setPWM(float pulseWidth)
    {
        // Limit PWM between 5% and 95%
        pwm = juce::jlimit(0.05f, 0.95f, pulseWidth);
    }

    float getNextSample()
    {
        float value = 0.0f;
        float t = phase;

        switch (waveform)
        {
            case Waveform::Sine:
                value = std::sin(juce::MathConstants<float>::twoPi * t);
                break;
            case Waveform::Triangle:
                value = -1.0f + (2.0f * t);
                value = 2.0f * (std::fabs(value) - 0.5f);
                break; // Needs PolyBLEP integration for pure perfection, but good enough for now
            case Waveform::Saw:
                value = (2.0f * t) - 1.0f;
                value -= polyBlep(t);
                break;
            case Waveform::Square:
            {
                value = t < pwm ? 1.0f : -1.0f;
                value += polyBlep(t);
                value -= polyBlep(std::fmod(t + 1.0f - pwm, 1.0f));
                break;
            }
        }

        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;

        return value;
    }

private:
    float polyBlep(float t) const
    {
        float dt = inc;
        if (t < dt) 
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt) 
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        else 
        {
            return 0.0f;
        }
    }

    double currentSampleRate = 44100.0;
    float phase = 0.0f;
    float inc = 0.0f;
    float pwm = 0.5f;
    Waveform waveform = Waveform::Saw;
};
