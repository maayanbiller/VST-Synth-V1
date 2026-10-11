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
    int samplesInCurrentState = 0;
    int stateLengthSamples = 0;
    float targetValue = 0.0f;
    float coef = 0.0f;
    float releaseMultiplier = 0.0f;

    void advanceToNextState();
    void enterState(State newState, bool resetSamples = true);
};
