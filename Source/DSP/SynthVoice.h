#pragma once

#include <JuceHeader.h>
#include "DAHDSREnvelope.h"
#include "PolyBLEPOscillator.h"
#include "LFO.h"
#include "Filter.h"
#include "ModMatrix.h"

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};


struct GlobalSynthParams {
    float oscOct[4];
    float oscDetune[4];
    float oscLevel[4];
    float oscPwm[4];
    int oscWave[4];
    int oscVoices[4];
    float oscWidth[4];

    float noiseLevel, noiseColor;
    int playMode; // 0=Poly, 1=Mono, 2=Legato
    float glideTime;

    int filterType;
    float filterCutoff, filterRes, filterDrive, filterEnvAmt, filterKeyTrack;

    DAHDSREnvelope::Parameters ampParams, filtParams, modParams;

    float lfoRate[2];
    float lfoDepth[2];
    int lfoWave[2];

    int modSrc[4];
    int modDst[4];
    float modAmts[4];

    int modMatrix[5][8]; // 5 rows (sources) x 8 cols (destinations), cell values 0..5
    float modBusAmts[4]; // 4 bus amount knobs (-1.0 to 1.0)
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice();
    
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void retargetNote (int newMidiNote, float newVelocity, bool retriggerEnvelopes);
    bool isVoiceActive() const override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void updateBaseParameters(const GlobalSynthParams& params);

private:
    GlobalSynthParams currentParams;
    void calculateModulation(float& o1PitchMod, float& o2PitchMod, float& o3PitchMod, float& o4PitchMod, float& o1PwmMod, float& o2PwmMod, float& o3PwmMod, float& o4PwmMod, float& fCutoffMod, float& fResMod);

    UnisonOscillator osc1, osc2, osc3, osc4;
    SynthFilter filter;
    
    DAHDSREnvelope ampEnv, filterEnv, modEnv;
    LFO lfo1, lfo2;

    int baseLfo1Target = 0; int baseLfo2Target = 0; int baseModEnvTarget = 0;

    double currentSampleRate = 44100.0;
    bool isPrepared = false;
    juce::SmoothedValue<float> currentVelocity {1.0f};
    juce::SmoothedValue<float> currentMidiPitch {60.0f};
    float lastMidiNote = -1.0f;
    int currentMidiNote = 60;
    juce::SmoothedValue<float> currentPitchBend {0.0f};
    float modWheelValue = 0.0f;

    // Smoothed Base parameters
    juce::SmoothedValue<float> baseO1Level {1.0f}, baseO2Level {0.0f}, baseO3Level {0.0f}, baseO4Level {0.0f};
    juce::SmoothedValue<float> baseO1Pwm {0.5f}, baseO2Pwm {0.5f}, baseO3Pwm {0.5f}, baseO4Pwm {0.5f};
    float baseO1Octave = 0, baseO1Detune = 0;
    float baseO2Octave = 0, baseO2Detune = 0;
    float baseO3Octave = 0, baseO3Detune = 0;
    float baseO4Octave = 0, baseO4Detune = 0;
    
    juce::SmoothedValue<float> baseFCutoff {1000.0f};
    juce::SmoothedValue<float> baseFRes {1.0f};
    juce::SmoothedValue<float> baseFDrive {1.0f};
    
    float baseFEnvAmt = 0.0f, baseFKeyTrack = 0.0f;
    float baseLfo1Depth = 1.0f, baseLfo2Depth = 1.0f;
    float noiseZ1 = 0.0f, noiseZ2 = 0.0f;

    juce::AudioBuffer<float> voiceBuffer;
};
