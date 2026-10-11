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
    
    
    
    
    
    
    
    
    
    

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Arrays for fast APVTS lookup
    

// Cached Parameters
    std::atomic<float>* noiseLevel = nullptr; std::atomic<float>* noiseColor = nullptr;
    std::atomic<float>* playMode = nullptr; std::atomic<float>* glideTime = nullptr;
    std::atomic<float>* modAmt1 = nullptr; std::atomic<float>* modAmt2 = nullptr; std::atomic<float>* modAmt3 = nullptr; std::atomic<float>* modAmt4 = nullptr;

    std::atomic<float>* modSrc1 = nullptr; std::atomic<float>* modDst1 = nullptr;
    std::atomic<float>* modSrc2 = nullptr; std::atomic<float>* modDst2 = nullptr;
    std::atomic<float>* modSrc3 = nullptr; std::atomic<float>* modDst3 = nullptr;
    std::atomic<float>* modSrc4 = nullptr; std::atomic<float>* modDst4 = nullptr;

    std::atomic<float>* masterVol = nullptr;
     
      

    std::atomic<float>* osc1Wave = nullptr; std::atomic<float>* osc1Oct = nullptr; std::atomic<float>* osc1Detune = nullptr; std::atomic<float>* osc1Level = nullptr; std::atomic<float>* osc1Pwm = nullptr;
    std::atomic<float>* osc2Wave = nullptr; std::atomic<float>* osc2Oct = nullptr; std::atomic<float>* osc2Detune = nullptr; std::atomic<float>* osc2Level = nullptr; std::atomic<float>* osc2Pwm = nullptr;
    std::atomic<float>* osc3Wave = nullptr; std::atomic<float>* osc3Oct = nullptr; std::atomic<float>* osc3Detune = nullptr; std::atomic<float>* osc3Level = nullptr; std::atomic<float>* osc3Pwm = nullptr;
    std::atomic<float>* osc4Wave = nullptr; std::atomic<float>* osc4Oct = nullptr; std::atomic<float>* osc4Detune = nullptr; std::atomic<float>* osc4Level = nullptr; std::atomic<float>* osc4Pwm = nullptr;

    std::atomic<float>* filterType = nullptr; std::atomic<float>* filterCutoff = nullptr; std::atomic<float>* filterRes = nullptr; std::atomic<float>* filterDrive = nullptr; std::atomic<float>* filterEnvAmt = nullptr; std::atomic<float>* filterKeyTrack = nullptr;
    
    std::atomic<float>* ampAttack = nullptr; std::atomic<float>* ampDecay = nullptr; std::atomic<float>* ampSustain = nullptr; std::atomic<float>* ampRelease = nullptr;
    std::atomic<float>* filtAttack = nullptr; std::atomic<float>* filtDecay = nullptr; std::atomic<float>* filtSustain = nullptr; std::atomic<float>* filtRelease = nullptr;
    std::atomic<float>* modAttack = nullptr; std::atomic<float>* modDecay = nullptr; std::atomic<float>* modSustain = nullptr; std::atomic<float>* modRelease = nullptr;
    
    std::atomic<float>* lfo1Wave = nullptr; std::atomic<float>* lfo1Rate = nullptr; std::atomic<float>* lfo1Depth = nullptr;
    std::atomic<float>* lfo2Wave = nullptr; std::atomic<float>* lfo2Rate = nullptr; std::atomic<float>* lfo2Depth = nullptr;

       
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSynthV1AudioProcessor)
};

