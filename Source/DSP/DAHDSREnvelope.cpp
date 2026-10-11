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

    // If attack and decay are effectively 0, start immediately on sustain level
    if (parameters.attack <= 0.0005f && parameters.decay <= 0.0005f && parameters.delay <= 0.0001f)
    {
        currentValue = parameters.sustain;
        currentState = State::Sustain;
        stateLengthSamples = 0;
        return;
    }
    else if (parameters.attack <= 0.0005f && parameters.delay <= 0.0001f)
    {
        currentValue = 1.0f;
        enterState(State::Hold, true);
        return;
    }

    currentValue = 0.0f;
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
            if (resetSamples) currentValue = 0.0f;
            stateLengthSamples = static_cast<int>(parameters.attack * sampleRate);
            remainingSamples = stateLengthSamples - samplesInCurrentState;
            if (remainingSamples > 0)
            {
                targetValue = 1.02f;
                coef = 1.0f - std::exp(-4.5f / (float)remainingSamples); 
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
                targetValue = parameters.sustain;
                coef = 1.0f - std::exp(-4.5f / (float)remainingSamples);
            }
            break;

        case State::Sustain:
            currentValue = parameters.sustain;
            stateLengthSamples = 0;
            return;

        case State::Release:
            stateLengthSamples = static_cast<int>(parameters.release * sampleRate);
            remainingSamples = std::max(1, stateLengthSamples - samplesInCurrentState);
            // Pure analog RC discharge targeting 0.0 with 60 dB decay over release time
            releaseMultiplier = std::exp(-6.90775f / (float)remainingSamples);
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
        if ((currentState == State::Delay || currentState == State::Hold) && samplesInCurrentState >= stateLengthSamples)
            advanceToNextState();
        return currentValue;
    }

    if (currentState == State::Release)
    {
        currentValue *= releaseMultiplier;
        samplesInCurrentState++;

        // Seamless asymptotic termination at inaudible noise floor (-94 dB)
        if (currentValue < 0.00002f)
        {
            currentValue = 0.0f;
            enterState(State::Idle, true);
            return 0.0f;
        }
        return currentValue;
    }

    // Exponential RC curve processing for Attack & Decay
    float output = currentValue;
    currentValue += (targetValue - currentValue) * coef;
    samplesInCurrentState++;

    if (samplesInCurrentState >= stateLengthSamples)
    {
        advanceToNextState();
    }

    return std::clamp(output, 0.0f, 1.0f);
}
