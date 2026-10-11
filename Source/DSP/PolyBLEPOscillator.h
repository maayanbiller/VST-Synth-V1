#pragma once
#include <juce_core/juce_core.h>

class PolyBLEPOscillator
{
public:
    enum class Waveform { Sine, Triangle, Saw, Square };

    PolyBLEPOscillator() = default;
    ~PolyBLEPOscillator() = default;

    void randomizePhase() { phase = juce::Random::getSystemRandom().nextFloat(); }

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

class UnisonOscillator
{
public:
    static constexpr int maxVoices = 8;

    UnisonOscillator() = default;
    ~UnisonOscillator() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        for (int i = 0; i < maxVoices; ++i)
        {
            voices[i].prepare(sampleRate);
        }
    }

    void randomizePhase()
    {
        for (int i = 0; i < maxVoices; ++i)
        {
            voices[i].randomizePhase();
        }
    }

    void setWaveform(PolyBLEPOscillator::Waveform wave)
    {
        for (int i = 0; i < maxVoices; ++i)
        {
            voices[i].setWaveform(wave);
        }
    }

    void setPWM(float pulseWidth)
    {
        for (int i = 0; i < maxVoices; ++i)
        {
            voices[i].setPWM(pulseWidth);
        }
    }

    void setVoices(int count)
    {
        numVoices = std::clamp(count, 1, maxVoices);
    }

    void setWidth(float width)
    {
        stereoWidth = std::clamp(width, 0.0f, 1.0f);
    }

    void setFrequencyAndDetune(float baseFreq, float detuneCents)
    {
        if (numVoices <= 1)
        {
            float cents = detuneCents;
            float freq = baseFreq * std::pow(2.0f, cents / 1200.0f);
            voices[0].setFrequency(freq);
            return;
        }

        // Symmetric detune and stereo pan spread across unison voices
        for (int i = 0; i < numVoices; ++i)
        {
            float t = -1.0f + 2.0f * (static_cast<float>(i) / static_cast<float>(numVoices - 1));
            // Musical unison detune spread: 15-25 cents base spread scaled with width
            float spreadCents = t * (12.0f + 16.0f * stereoWidth);
            float totalCents = detuneCents + spreadCents;
            float freq = baseFreq * std::pow(2.0f, totalCents / 1200.0f);
            voices[i].setFrequency(freq);
        }
    }

    void getNextStereoSample(float& outL, float& outR)
    {
        if (numVoices <= 1)
        {
            float s = voices[0].getNextSample();
            outL = s;
            outR = s;
            return;
        }

        float norm = 1.0f / std::sqrt(static_cast<float>(numVoices));

        // When width is 0, sum all voices into true mono with bit-for-bit identical L and R
        if (stereoWidth <= 0.001f)
        {
            float sum = 0.0f;
            for (int i = 0; i < numVoices; ++i)
                sum += voices[i].getNextSample();

            float s = sum * norm * 0.7071f;
            outL = s;
            outR = s;
            return;
        }

        float sumL = 0.0f;
        float sumR = 0.0f;

        for (int i = 0; i < numVoices; ++i)
        {
            float s = voices[i].getNextSample();
            float t = -1.0f + 2.0f * (static_cast<float>(i) / static_cast<float>(numVoices - 1));
            // Pan: 0.0 = full left, 0.5 = center, 1.0 = full right
            float pan = 0.5f + 0.5f * (t * stereoWidth);
            pan = std::clamp(pan, 0.0f, 1.0f);

            // Constant power stereo panning
            float gainL = std::cos(pan * juce::MathConstants<float>::halfPi);
            float gainR = std::sin(pan * juce::MathConstants<float>::halfPi);

            sumL += s * gainL;
            sumR += s * gainR;
        }

        outL = sumL * norm;
        outR = sumR * norm;
    }

private:
    std::array<PolyBLEPOscillator, maxVoices> voices;
    double currentSampleRate = 44100.0;
    int numVoices = 1;
    float stereoWidth = 0.5f;
};
