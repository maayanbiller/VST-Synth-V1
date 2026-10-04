#include "DAHDSREnvelope.h"
#include <algorithm>
#include <cmath>

DAHDSREnvelope::DAHDSREnvelope()
{
}

void DAHDSREnvelope::setSampleRate (double newSampleRate)
{
    jassert (newSampleRate > 0.0);
    sampleRate = newSampleRate;
}

void DAHDSREnvelope::setParameters (const Parameters& newParams)
{
    // Check if parameters actually changed
    bool changed = (parameters.delay != newParams.delay ||
                    parameters.attack != newParams.attack ||
                    parameters.hold != newParams.hold ||
                    parameters.decay != newParams.decay ||
                    parameters.sustain != newParams.sustain ||
                    parameters.release != newParams.release);
    
    parameters = newParams;
    
    // Dynamically recalculate if we are actively moving
    if (changed && currentState != State::Idle && currentState != State::Sustain)
    {
        enterState(currentState, false); // false = don't reset sample counter
    }
}

void DAHDSREnvelope::noteOn()
{
    samplesInCurrentState = 0;
    enterState(State::Delay, true);
}

void DAHDSREnvelope::noteOff()
{
    if (currentState != State::Idle)
    {
        samplesInCurrentState = 0;
        enterState(State::Release, true);
    }
}

void DAHDSREnvelope::reset()
{
    currentState = State::Idle;
    currentValue = 0.0f;
}

bool DAHDSREnvelope::isActive() const
{
    return currentState != State::Idle;
}

void DAHDSREnvelope::enterState(State newState, bool resetSamples)
{
    currentState = newState;
    if (resetSamples) samplesInCurrentState = 0;

    int remainingSamples = 0;

    switch (currentState)
    {
        case State::Idle:
            currentValue = 0.0f;
            stateLengthSamples = 0;
            return;
            
        case State::Delay:
            if (resetSamples) currentValue = 0.0f;
            stateLengthSamples = static_cast<int>(parameters.delay * sampleRate);
            break;

        case State::Attack:
            stateLengthSamples = static_cast<int>(parameters.attack * sampleRate);
            remainingSamples = stateLengthSamples - samplesInCurrentState;
            if (remainingSamples > 0)
            {
                // Exponential curve overshooting to 1.01 for snappy attack
                targetValue = 1.01f;
                // Solve for coef: we want to reach 1.0 in remainingSamples.
                // Not perfectly rigorous here, but a fixed time constant works.
                // Let's use a standard RC approach:
                coef = 1.0f - std::exp(-5.0f / remainingSamples); 
            }
            break;

        case State::Hold:
            if (resetSamples) currentValue = 1.0f;
            stateLengthSamples = static_cast<int>(parameters.hold * sampleRate);
            break;

        case State::Decay:
            stateLengthSamples = static_cast<int>(parameters.decay * sampleRate);
            remainingSamples = stateLengthSamples - samplesInCurrentState;
            if (remainingSamples > 0)
            {
                // Exponential decay to sustain
                targetValue = parameters.sustain;
                coef = 1.0f - std::exp(-5.0f / remainingSamples);
            }
            break;

        case State::Sustain:
            currentValue = parameters.sustain;
            stateLengthSamples = 0;
            return;

        case State::Release:
            stateLengthSamples = static_cast<int>(parameters.release * sampleRate);
            remainingSamples = stateLengthSamples - samplesInCurrentState;
            if (remainingSamples > 0)
            {
                targetValue = 0.0f;
                coef = 1.0f - std::exp(-5.0f / remainingSamples);
            }
            break;
    }

    // Skip zero-length states safely
    while (currentState != State::Idle && currentState != State::Sustain && stateLengthSamples <= samplesInCurrentState)
    {
        advanceToNextState();
        if (currentState == State::Idle || currentState == State::Sustain) break;
    }
}

void DAHDSREnvelope::advanceToNextState()
{
    samplesInCurrentState = 0; // reset for next state
    switch (currentState)
    {
        case State::Delay:   enterState(State::Attack, true);  break;
        case State::Attack:  
            currentValue = 1.0f; 
            enterState(State::Hold, true);    
            break;
        case State::Hold:    enterState(State::Decay, true);   break;
        case State::Decay:   
            currentValue = parameters.sustain;
            enterState(State::Sustain, true); 
            break;
        case State::Sustain: break;
        case State::Release: 
            currentValue = 0.0f;
            enterState(State::Idle, true);    
            break;
        case State::Idle:    break;
    }
}

float DAHDSREnvelope::getNextSample()
{
    if (currentState == State::Idle || currentState == State::Sustain || currentState == State::Delay || currentState == State::Hold)
    {
        samplesInCurrentState++;
        if (stateLengthSamples > 0 && samplesInCurrentState >= stateLengthSamples)
            advanceToNextState();
        return currentValue;
    }

    // Exponential RC curve processing
    currentValue += (targetValue - currentValue) * coef;
    samplesInCurrentState++;

    if (samplesInCurrentState >= stateLengthSamples)
    {
        advanceToNextState();
    }

    return std::clamp(currentValue, 0.0f, 1.0f);
}
