#pragma once

#include <JuceHeader.h>
#include "DSP/SynthEngine.h"
#include "DSP/SynthParameters.h"

class VSTSynthV1AudioProcessor : public juce::AudioProcessor
{
public:
    VSTSynthV1AudioProcessor();
    ~VSTSynthV1AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    SynthEngine synth;

private:
    std::vector<SynthVoice*> cachedVoicePointers;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Arrays for fast APVTS lookup
    

    // Cached Parameter Pointers (Initialized once in constructor, zero audio-thread allocation/locking)
    struct CachedParameters
    {
        std::atomic<float>* masterVol = nullptr;
        std::atomic<float>* noiseLevel = nullptr;
        std::atomic<float>* noiseColor = nullptr;
        std::atomic<float>* playMode = nullptr;
        std::atomic<float>* glideTime = nullptr;

        std::atomic<float>* oscWave[4] = { nullptr };
        std::atomic<float>* oscOct[4] = { nullptr };
        std::atomic<float>* oscDetune[4] = { nullptr };
        std::atomic<float>* oscLevel[4] = { nullptr };
        std::atomic<float>* oscPwm[4] = { nullptr };
        std::atomic<float>* oscVoices[4] = { nullptr };
        std::atomic<float>* oscWidth[4] = { nullptr };

        std::atomic<float>* filterType = nullptr;
        std::atomic<float>* filterCutoff = nullptr;
        std::atomic<float>* filterRes = nullptr;
        std::atomic<float>* filterDrive = nullptr;
        std::atomic<float>* filterEnvAmt = nullptr;
        std::atomic<float>* filterKeyTrack = nullptr;

        std::atomic<float>* ampAttack = nullptr;
        std::atomic<float>* ampDecay = nullptr;
        std::atomic<float>* ampSustain = nullptr;
        std::atomic<float>* ampRelease = nullptr;

        std::atomic<float>* filtAttack = nullptr;
        std::atomic<float>* filtDecay = nullptr;
        std::atomic<float>* filtSustain = nullptr;
        std::atomic<float>* filtRelease = nullptr;

        std::atomic<float>* modAttack = nullptr;
        std::atomic<float>* modDecay = nullptr;
        std::atomic<float>* modSustain = nullptr;
        std::atomic<float>* modRelease = nullptr;

        std::atomic<float>* lfoWave[2] = { nullptr };
        std::atomic<float>* lfoRate[2] = { nullptr };
        std::atomic<float>* lfoDepth[2] = { nullptr };

        std::atomic<float>* modBusAmts[4] = { nullptr };
        std::atomic<float>* modMatrix[5][8] = { { nullptr } };

        std::atomic<float>* modSrc[4] = { nullptr };
        std::atomic<float>* modDst[4] = { nullptr };
        std::atomic<float>* modAmts[4] = { nullptr };
    } cachedParams;

    struct DCBlocker
    {
        float x1 = 0.0f;
        float y1 = 0.0f;
        float R = 0.9993f;

        void prepare(double sampleRate)
        {
            R = 1.0f - (2.0f * juce::MathConstants<float>::pi * 5.0f / static_cast<float>(sampleRate));
            reset();
        }

        void reset()
        {
            x1 = 0.0f;
            y1 = 0.0f;
        }

        float processSample(float x)
        {
            float y = x - x1 + R * y1;
            x1 = x;
            if (std::abs(y) < 1e-8f || !std::isfinite(y))
            {
                y = 0.0f;
                y1 = 0.0f;
                x1 = 0.0f;
            }
            else
            {
                y1 = y;
            }
            return y;
        }
    };

    DCBlocker dcBlockerL, dcBlockerR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSynthV1AudioProcessor)
};

