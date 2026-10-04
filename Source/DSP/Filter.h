#pragma once
#include <juce_dsp/juce_dsp.h>

class SynthFilter
{
public:
    enum class Type { LP12, LP24, HP12, HP24, BP12, Notch };

    SynthFilter()
    {
        filter1.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        currentSampleRate = spec.sampleRate;
        filter1.prepare(spec);
        filter2.prepare(spec);
        drive.prepare(spec);
        drive.functionToUse = [](float x) { return std::tanh(x); }; // Soft clipper
    }

    void setType(Type type)
    {
        currentType = type;
        switch (type)
        {
            case Type::LP12:
            case Type::LP24:
                filter1.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                filter2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                break;
            case Type::HP12:
            case Type::HP24:
                filter1.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                filter2.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                break;
            case Type::BP12:
            case Type::Notch:
                filter1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                break;
        }
    }

    void setCutoff(float freq)
    {
        // Clamp cutoff to prevent instability
        float clamped = juce::jlimit(20.0f, static_cast<float>(currentSampleRate) * 0.49f, freq);
        filter1.setCutoffFrequency(clamped);
        filter2.setCutoffFrequency(clamped);
    }

    void setResonance(float res)
    {
        float mapped = juce::jmap(res, 0.0f, 10.0f, 0.1f, 10.0f); // Map 0-10 to 0.1-10
        filter1.setResonance(mapped);
        filter2.setResonance(mapped);
    }

    void setDrive(float d) // 1.0 to 10.0
    {
        driveMultiplier = d;
        makeupGain = 1.0f / std::sqrt(d);
    }

    template <typename ProcessContext>
    void process(const ProcessContext& context)
    {
        // Apply drive
        auto inputBlock = context.getInputBlock();
        auto outputBlock = context.getOutputBlock();
        
        for (size_t ch = 0; ch < outputBlock.getNumChannels(); ++ch)
        {
            auto* in = inputBlock.getChannelPointer(ch);
            auto* out = outputBlock.getChannelPointer(ch);
            for (size_t i = 0; i < outputBlock.getNumSamples(); ++i)
            {
                out[i] = std::tanh(in[i] * driveMultiplier);
            }
        }

        // Process filter
        filter1.process(context);
        
        // 24dB modes run through second filter
        if (currentType == Type::LP24 || currentType == Type::HP24)
        {
            filter2.process(context);
        }

        // Apply makeup
        for (size_t ch = 0; ch < outputBlock.getNumChannels(); ++ch)
        {
            auto* out = outputBlock.getChannelPointer(ch);
            for (size_t i = 0; i < outputBlock.getNumSamples(); ++i)
            {
                out[i] *= makeupGain;
            }
        }
    }

private:
    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter2; // Used for 24dB slopes
    juce::dsp::WaveShaper<float> drive;
    
    Type currentType = Type::LP12;
    double currentSampleRate = 44100.0;
    float driveMultiplier = 1.0f;
    float makeupGain = 1.0f;
};
