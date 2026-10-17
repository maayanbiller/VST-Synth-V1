#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

class SynthFilter
{
public:
    enum class Type { LP12, LP24, HP12, HP24, BP12, Notch };

    SynthFilter() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        currentSampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
        lastCutoff = -1.0f;
        lastRes = -1.0f;
        reset();
        updateCoefficients(1000.0f, 1.0f);
    }

    void reset() noexcept
    {
        s1L_1 = 0.0f; s2L_1 = 0.0f;
        s1R_1 = 0.0f; s2R_1 = 0.0f;
        s1L_2 = 0.0f; s2L_2 = 0.0f;
        s1R_2 = 0.0f; s2R_2 = 0.0f;
    }

    void setType(Type type) noexcept
    {
        if (currentType != type)
        {
            currentType = type;
            // Recompute Q distribution if switching between 12dB and 24dB
            float r = lastRes > 0.0f ? lastRes : 1.0f;
            lastRes = -1.0f;
            setResonance(r);
        }
    }

    void setCutoff(float freq) noexcept
    {
        float clamped = juce::jlimit(20.0f, static_cast<float>(currentSampleRate) * 0.49f, freq);
        if (std::abs(clamped - lastCutoff) > 0.001f)
        {
            lastCutoff = clamped;
            g = static_cast<float>(std::tan(juce::MathConstants<double>::pi * static_cast<double>(clamped) / currentSampleRate));
            h = 1.0f / (1.0f + R2 * g + g * g);
        }
    }

    void setResonance(float res) noexcept
    {
        float clampedRes = std::clamp(res, 0.1f, 10.0f);
        if (std::abs(clampedRes - lastRes) > 0.0001f)
        {
            lastRes = clampedRes;
            float norm = (clampedRes - 0.1f) / 9.9f;
            float q = 0.7071f + (norm * norm * 15.3f);

            if (currentType == Type::LP24 || currentType == Type::HP24)
            {
                float stageQ = std::max(0.7071f, std::sqrt(q));
                R2 = 1.0f / stageQ;
            }
            else
            {
                R2 = 1.0f / q;
            }
            h = 1.0f / (1.0f + R2 * g + g * g);
        }
    }

    void setDrive(float d) noexcept // 1.0 to 10.0
    {
        driveAmount = std::clamp(d, 1.0f, 10.0f);
    }

    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        auto inputBlock = context.getInputBlock();
        auto outputBlock = context.getOutputBlock();
        size_t numChannels = outputBlock.getNumChannels();
        size_t numSamples = outputBlock.getNumSamples();

        // 1. Warm C2-continuous Pade [3/2] rational saturation stage
        if (driveAmount > 1.05f)
        {
            float driveGain = 1.0f + 0.35f * (driveAmount - 1.0f);
            float normFactor = 1.0f / std::sqrt(1.0f + 0.2f * (driveAmount - 1.0f));

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                const float* in = inputBlock.getChannelPointer(ch);
                float* out = outputBlock.getChannelPointer(ch);
                for (size_t i = 0; i < numSamples; ++i)
                {
                    float x = std::clamp(in[i] * driveGain, -3.0f, 3.0f);
                    float x2 = x * x;
                    out[i] = (x * (27.0f + x2) / (27.0f + 9.0f * x2)) * normFactor;
                }
            }
        }
        else if (inputBlock.getChannelPointer(0) != outputBlock.getChannelPointer(0))
        {
            outputBlock.copyFrom(inputBlock);
        }

        // 2. Stereo-shared Zavalishin TPT State Variable Filter processing
        const float gVal = g;
        const float r2PlusG = R2 + gVal;
        const float hVal = h;
        const Type type = currentType;

        float* chL = outputBlock.getChannelPointer(0);
        float* chR = (numChannels > 1) ? outputBlock.getChannelPointer(1) : nullptr;

        if (chR != nullptr)
        {
            for (size_t i = 0; i < numSamples; ++i)
            {
                // Stage 1 Left & Right
                float xL = chL[i];
                float yHL = (xL - r2PlusG * s1L_1 - s2L_1) * hVal;
                float yBL = gVal * yHL + s1L_1;
                s1L_1 = gVal * yHL + yBL;
                float yLL = gVal * yBL + s2L_1;
                s2L_1 = gVal * yBL + yLL;

                float xR = chR[i];
                float yHR = (xR - r2PlusG * s1R_1 - s2R_1) * hVal;
                float yBR = gVal * yHR + s1R_1;
                s1R_1 = gVal * yHR + yBR;
                float yLR = gVal * yBR + s2R_1;
                s2R_1 = gVal * yBR + yLR;

                switch (type)
                {
                    case Type::LP12:
                        chL[i] = yLL;
                        chR[i] = yLR;
                        break;
                    case Type::HP12:
                        chL[i] = yHL;
                        chR[i] = yHR;
                        break;
                    case Type::BP12:
                    case Type::Notch:
                        chL[i] = yBL;
                        chR[i] = yBR;
                        break;
                    case Type::LP24:
                    {
                        float yHL2 = (yLL - r2PlusG * s1L_2 - s2L_2) * hVal;
                        float yBL2 = gVal * yHL2 + s1L_2;
                        s1L_2 = gVal * yHL2 + yBL2;
                        float yLL2 = gVal * yBL2 + s2L_2;
                        s2L_2 = gVal * yBL2 + yLL2;

                        float yHR2 = (yLR - r2PlusG * s1R_2 - s2R_2) * hVal;
                        float yBR2 = gVal * yHR2 + s1R_2;
                        s1R_2 = gVal * yHR2 + yBR2;
                        float yLR2 = gVal * yBR2 + s2R_2;
                        s2R_2 = gVal * yBR2 + yLR2;

                        chL[i] = yLL2;
                        chR[i] = yLR2;
                        break;
                    }
                    case Type::HP24:
                    {
                        float yHL2 = (yHL - r2PlusG * s1L_2 - s2L_2) * hVal;
                        float yBL2 = gVal * yHL2 + s1L_2;
                        s1L_2 = gVal * yHL2 + yBL2;
                        float yLL2 = gVal * yBL2 + s2L_2;
                        s2L_2 = gVal * yBL2 + yLL2;

                        float yHR2 = (yHR - r2PlusG * s1R_2 - s2R_2) * hVal;
                        float yBR2 = gVal * yHR2 + s1R_2;
                        s1R_2 = gVal * yHR2 + yBR2;
                        float yLR2 = gVal * yBR2 + s2R_2;
                        s2R_2 = gVal * yBR2 + yLR2;

                        chL[i] = yHL2;
                        chR[i] = yHR2;
                        break;
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < numSamples; ++i)
            {
                float xL = chL[i];
                float yHL = (xL - r2PlusG * s1L_1 - s2L_1) * hVal;
                float yBL = gVal * yHL + s1L_1;
                s1L_1 = gVal * yHL + yBL;
                float yLL = gVal * yBL + s2L_1;
                s2L_1 = gVal * yBL + yLL;

                if (type == Type::LP12) chL[i] = yLL;
                else if (type == Type::HP12) chL[i] = yHL;
                else if (type == Type::BP12 || type == Type::Notch) chL[i] = yBL;
                else if (type == Type::LP24)
                {
                    float yHL2 = (yLL - r2PlusG * s1L_2 - s2L_2) * hVal;
                    float yBL2 = gVal * yHL2 + s1L_2;
                    s1L_2 = gVal * yHL2 + yBL2;
                    float yLL2 = gVal * yBL2 + s2L_2;
                    s2L_2 = gVal * yBL2 + yLL2;
                    chL[i] = yLL2;
                }
                else if (type == Type::HP24)
                {
                    float yHL2 = (yHL - r2PlusG * s1L_2 - s2L_2) * hVal;
                    float yBL2 = gVal * yHL2 + s1L_2;
                    s1L_2 = gVal * yHL2 + yBL2;
                    float yLL2 = gVal * yBL2 + s2L_2;
                    s2L_2 = gVal * yBL2 + yLL2;
                    chL[i] = yHL2;
                }
            }
        }
    }

private:
    void updateCoefficients(float cutoff, float res) noexcept
    {
        setCutoff(cutoff);
        setResonance(res);
    }

    float g = 0.07f;
    float R2 = 1.4142f;
    float h = 0.9f;
    float lastCutoff = -1.0f;
    float lastRes = -1.0f;

    float s1L_1 = 0.0f, s2L_1 = 0.0f;
    float s1R_1 = 0.0f, s2R_1 = 0.0f;
    float s1L_2 = 0.0f, s2L_2 = 0.0f;
    float s1R_2 = 0.0f, s2R_2 = 0.0f;

    Type currentType = Type::LP12;
    double currentSampleRate = 44100.0;
    float driveAmount = 1.0f;
};

