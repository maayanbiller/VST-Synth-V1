#pragma once
#include <juce_dsp/juce_dsp.h>

class SynthFilter
{
public:
    enum class Type { LP12, LP24, HP12, HP24, BP12, Notch };

    SynthFilter()
    {
        filter1L.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter1R.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter2L.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter2R.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        currentSampleRate = spec.sampleRate;
        juce::dsp::ProcessSpec monoSpec { spec.sampleRate, spec.maximumBlockSize, 1 };
        filter1L.prepare(monoSpec);
        filter1R.prepare(monoSpec);
        filter2L.prepare(monoSpec);
        filter2R.prepare(monoSpec);
    }

    void reset()
    {
        filter1L.reset();
        filter1R.reset();
        filter2L.reset();
        filter2R.reset();
    }

    void setType(Type type)
    {
        currentType = type;
        switch (type)
        {
            case Type::LP12:
            case Type::LP24:
                filter1L.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                filter1R.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                filter2L.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                filter2R.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                break;
            case Type::HP12:
            case Type::HP24:
                filter1L.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                filter1R.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                filter2L.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                filter2R.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                break;
            case Type::BP12:
            case Type::Notch:
                filter1L.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                filter1R.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                break;
        }
    }

    void setCutoff(float freq)
    {
        // Clamp cutoff to prevent instability
        float clamped = juce::jlimit(20.0f, static_cast<float>(currentSampleRate) * 0.49f, freq);
        filter1L.setCutoffFrequency(clamped);
        filter1R.setCutoffFrequency(clamped);
        filter2L.setCutoffFrequency(clamped);
        filter2R.setCutoffFrequency(clamped);
    }

    void setResonance(float res)
    {
        // Smooth musical resonance mapping: Butterworth flat Q=0.707 at minimum up to Q=16.0 at max
        float clampedRes = std::clamp(res, 0.1f, 10.0f);
        float norm = (clampedRes - 0.1f) / 9.9f;
        float q = 0.7071f + (norm * norm * 15.3f);

        if (currentType == Type::LP24 || currentType == Type::HP24)
        {
            // Distribute Q smoothly across cascaded stages so 24dB doesn't create excessive Q^2 squeal
            float stageQ = std::max(0.7071f, std::sqrt(q));
            filter1L.setResonance(stageQ);
            filter1R.setResonance(stageQ);
            filter2L.setResonance(stageQ);
            filter2R.setResonance(stageQ);
        }
        else
        {
            filter1L.setResonance(q);
            filter1R.setResonance(q);
            filter2L.setResonance(q);
            filter2R.setResonance(q);
        }
    }

    void setDrive(float d) // 1.0 to 10.0
    {
        driveAmount = std::clamp(d, 1.0f, 10.0f);
    }

    template <typename ProcessContext>
    void process(const ProcessContext& context)
    {
        auto inputBlock = context.getInputBlock();
        auto outputBlock = context.getOutputBlock();
        size_t numChannels = outputBlock.getNumChannels();
        
        // Warm saturation stage
        if (driveAmount > 1.05f)
        {
            float driveGain = 1.0f + 0.35f * (driveAmount - 1.0f);
            float normFactor = 1.0f / std::sqrt(1.0f + 0.2f * (driveAmount - 1.0f));

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto* in = inputBlock.getChannelPointer(ch);
                auto* out = outputBlock.getChannelPointer(ch);
                for (size_t i = 0; i < outputBlock.getNumSamples(); ++i)
                {
                    out[i] = std::tanh(in[i] * driveGain) * normFactor;
                }
            }
        }
        else if (inputBlock.getChannelPointer(0) != outputBlock.getChannelPointer(0))
        {
            outputBlock.copyFrom(inputBlock);
        }

        // Process filter channels
        auto blockL = outputBlock.getSingleChannelBlock(0);
        juce::dsp::ProcessContextReplacing<float> ctxL(blockL);
        filter1L.process(ctxL);
        if (currentType == Type::LP24 || currentType == Type::HP24)
            filter2L.process(ctxL);

        if (numChannels > 1)
        {
            auto blockR = outputBlock.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> ctxR(blockR);
            filter1R.process(ctxR);
            if (currentType == Type::LP24 || currentType == Type::HP24)
                filter2R.process(ctxR);
        }
    }

private:
    juce::dsp::StateVariableTPTFilter<float> filter1L, filter1R;
    juce::dsp::StateVariableTPTFilter<float> filter2L, filter2R; // Used for 24dB slopes
    
    Type currentType = Type::LP12;
    double currentSampleRate = 44100.0;
    float driveAmount = 1.0f;
};
