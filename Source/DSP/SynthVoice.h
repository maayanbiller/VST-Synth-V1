#pragma once

#include <JuceHeader.h>
#include "DAHDSREnvelope.h"
#include "PolyBLEPOscillator.h"
#include "LFO.h"
#include "Filter.h"
#include "ModMatrix.h"
#include <array>

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice();
    
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // Parameter updates from Processor
    void updateBaseParameters(
        float o1Semi, float o1Detune, float o1Level, float o1Pwm, PolyBLEPOscillator::Waveform o1Wave,
        float o2Semi, float o2Detune, float o2Level, float o2Pwm, PolyBLEPOscillator::Waveform o2Wave,
        float subLevel, float subOct, PolyBLEPOscillator::Waveform subWave,
        float noiseLvl,
        SynthFilter::Type fType, float fCutoff, float fRes, float fDrive, float fEnvAmt, float fKeyTrack,
        const DAHDSREnvelope::Parameters& ampParams,
        const DAHDSREnvelope::Parameters& filtParams,
        const DAHDSREnvelope::Parameters& modParams,
        float lfo1Rate, LFO::Waveform lfo1Wave, float lfo1Depth,
        float lfo2Rate, LFO::Waveform lfo2Wave, float lfo2Depth,
        const std::array<ModRoute, 8>& modRoutes
    );

private:
    void calculateModulation(float& o1PitchMod, float& o2PitchMod, float& o1PwmMod, float& o2PwmMod, float& fCutoffMod, float& fResMod);

    PolyBLEPOscillator osc1;
    PolyBLEPOscillator osc2;
    PolyBLEPOscillator subOsc;
    
    SynthFilter filter;
    
    DAHDSREnvelope ampEnv;
    DAHDSREnvelope filterEnv;
    DAHDSREnvelope modEnv;
    
    LFO lfo1;
    LFO lfo2;

    std::array<ModRoute, 8> routes;

    double currentSampleRate = 44100.0;
    bool isPrepared = false;
    float currentVelocity = 1.0f;
    int currentMidiNote = 60;
    float currentPitchBend = 0.0f;
    float modWheelValue = 0.0f;
    float randomNoise = 0.0f;

    // Base parameters
    float baseO1Semi = 0, baseO1Detune = 0, baseO1Level = 1, baseO1Pwm = 0.5f;
    float baseO2Semi = 0, baseO2Detune = 0, baseO2Level = 0, baseO2Pwm = 0.5f;
    float baseSubLevel = 0, baseSubOct = -1;
    float baseNoiseLvl = 0;
    float baseFCutoff = 1000.0f, baseFRes = 1.0f, baseFDrive = 1.0f, baseFEnvAmt = 0.0f, baseFKeyTrack = 0.0f;
    float baseLfo1Depth = 1.0f, baseLfo2Depth = 1.0f;
};
