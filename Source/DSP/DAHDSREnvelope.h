#pragma once

#include <JuceHeader.h>

class DAHDSREnvelope
{
public:
    DAHDSREnvelope();

    struct Parameters
    {
        float delay   = 0.0f;
        float attack  = 0.1f;
        float hold    = 0.0f;
        float decay   = 0.1f;
        float sustain = 1.0f;
        float release = 0.1f;
    };

    void setSampleRate (double newSampleRate);
    void setParameters (const Parameters& newParams);
    void noteOn();
    void noteOff();
    void reset();
    bool isActive() const;

    float getNextSample();

private:
    enum class State {
        Idle,
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release
    };

    State currentState = State::Idle;
    Parameters parameters;
    double sampleRate = 44100.0;
    
    float currentValue = 0.0f;
    float startValue = 0.0f;
    float phase = 0.0f;
    float phaseInc = 0.0f;

    float decayExpVal = 1.0f;
    float decayExpMult = 1.0f;
    float releaseExpVal = 1.0f;
    float releaseExpMult = 1.0f;

    int delayCount = 0;
    int holdCount = 0;

    void enterAttack();
    void enterDecay();
    void enterRelease();
};
