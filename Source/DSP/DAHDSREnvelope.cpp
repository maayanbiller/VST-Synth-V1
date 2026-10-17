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
    float clampedSustain = std::clamp(newParams.sustain, 0.0f, 1.0f);
    if (parameters.delay == newParams.delay &&
        parameters.attack == newParams.attack &&
        parameters.hold == newParams.hold &&
        parameters.decay == newParams.decay &&
        parameters.sustain == clampedSustain &&
        parameters.release == newParams.release)
    {
        return;
    }

    parameters = newParams;
    parameters.sustain = clampedSustain;

    if (currentState == State::Sustain)
    {
        currentValue = parameters.sustain;
    }
    else if (currentState == State::Attack)
    {
        int nA = std::max(1, static_cast<int>(parameters.attack * sampleRate));
        phaseInc = 1.0f / static_cast<float>(nA);
    }
    else if (currentState == State::Decay)
    {
        int nD = std::max(1, static_cast<int>(parameters.decay * sampleRate));
        phaseInc = 1.0f / static_cast<float>(nD);
        decayExpMult = std::exp(-4.0f * phaseInc);
        decayExpVal = std::exp(-4.0f * std::min(1.0f, phase));
    }
    else if (currentState == State::Release)
    {
        int nR = std::max(1, static_cast<int>(parameters.release * sampleRate));
        phaseInc = 1.0f / static_cast<float>(nR);
        releaseExpMult = std::exp(-4.5f * phaseInc);
        releaseExpVal = std::exp(-4.5f * std::min(1.0f, phase));
    }
}

void DAHDSREnvelope::noteOn()
{
    // Requirement: If attack and decay are effectively 0, start immediately on sustain level
    if (parameters.attack <= 0.0001f && parameters.decay <= 0.0001f && parameters.delay <= 0.0001f)
    {
        currentValue = parameters.sustain;
        startValue = parameters.sustain;
        currentState = State::Sustain;
        return;
    }

    if (currentState == State::Idle)
    {
        currentValue = 0.0f;
        startValue = 0.0f;
    }
    else
    {
        // Clickless retriggering from existing level
        startValue = currentValue;
    }

    if (parameters.delay > 0.0001f)
    {
        delayCount = static_cast<int>(parameters.delay * sampleRate);
        currentState = State::Delay;
    }
    else
    {
        enterAttack();
    }
}

void DAHDSREnvelope::enterAttack()
{
    int nA = std::max(1, static_cast<int>(parameters.attack * sampleRate));
    phase = 0.0f;
    phaseInc = 1.0f / static_cast<float>(nA);
    currentState = State::Attack;
}

void DAHDSREnvelope::enterDecay()
{
    int nD = std::max(1, static_cast<int>(parameters.decay * sampleRate));
    phase = 0.0f;
    phaseInc = 1.0f / static_cast<float>(nD);
    decayExpVal = 1.0f;
    decayExpMult = std::exp(-4.0f * phaseInc);
    startValue = currentValue;
    currentState = State::Decay;
}

void DAHDSREnvelope::enterRelease()
{
    int nR = std::max(1, static_cast<int>(parameters.release * sampleRate));
    phase = 0.0f;
    phaseInc = 1.0f / static_cast<float>(nR);
    releaseExpVal = 1.0f;
    releaseExpMult = std::exp(-4.5f * phaseInc);
    startValue = currentValue;
    currentState = State::Release;
}

void DAHDSREnvelope::noteOff()
{
    if (currentState != State::Idle)
    {
        if (currentValue <= 0.00001f || parameters.release <= 0.0001f)
        {
            currentValue = 0.0f;
            currentState = State::Idle;
        }
        else
        {
            enterRelease();
        }
    }
}

void DAHDSREnvelope::reset()
{
    currentState = State::Idle;
    currentValue = 0.0f;
    startValue = 0.0f;
    phase = 0.0f;
    phaseInc = 0.0f;
    decayExpVal = 1.0f;
    releaseExpVal = 1.0f;
    delayCount = 0;
    holdCount = 0;
}

bool DAHDSREnvelope::isActive() const
{
    return currentState != State::Idle;
}

float DAHDSREnvelope::getNextSample()
{
    switch (currentState)
    {
        case State::Idle:
            return 0.0f;

        case State::Delay:
            delayCount--;
            if (delayCount <= 0)
                enterAttack();
            return 0.0f;

        case State::Attack:
        {
            float x = std::min(1.0f, phase);
            // Musical smoothstep S-curve: zero initial derivative (no pop/jump), zero landing derivative
            float sCurve = x * x * (3.0f - 2.0f * x);
            float val = startValue + (1.0f - startValue) * sCurve;
            currentValue = val;

            phase += phaseInc;
            if (phase >= 1.0f)
            {
                currentValue = 1.0f;
                if (parameters.hold > 0.0001f)
                {
                    holdCount = static_cast<int>(parameters.hold * sampleRate);
                    currentState = State::Hold;
                }
                else if (parameters.decay > 0.0001f)
                {
                    enterDecay();
                }
                else
                {
                    currentState = State::Sustain;
                }
            }
            return val;
        }

        case State::Hold:
            holdCount--;
            if (holdCount <= 0)
            {
                if (parameters.decay > 0.0001f)
                    enterDecay();
                else
                    currentState = State::Sustain;
            }
            return 1.0f;

        case State::Decay:
        {
            // Normalized analog RC decay landing seamlessly on sustain level using recursive exp multiplier
            constexpr float expMinD = 0.01831563888f; // exp(-4.0f)
            constexpr float normD = 1.0f / (1.0f - expMinD);
            float expTerm = std::max(0.0f, (decayExpVal - expMinD) * normD);
            decayExpVal *= decayExpMult;

            float val = parameters.sustain + (startValue - parameters.sustain) * expTerm;
            currentValue = val;

            phase += phaseInc;
            if (phase >= 1.0f)
            {
                currentValue = parameters.sustain;
                currentState = State::Sustain;
            }
            return val;
        }

        case State::Sustain:
            currentValue = parameters.sustain;
            return currentValue;

        case State::Release:
        {
            // Normalized analog RC release landing on exact 0.000000f without cutoff cliff
            constexpr float expMinR = 0.01110899653f; // exp(-4.5f)
            constexpr float normR = 1.0f / (1.0f - expMinR);
            float expTerm = std::max(0.0f, (releaseExpVal - expMinR) * normR);
            releaseExpVal *= releaseExpMult;

            float val = startValue * expTerm;
            currentValue = val;

            phase += phaseInc;
            if (phase >= 1.0f || val <= 0.0f)
            {
                currentValue = 0.0f;
                currentState = State::Idle;
                return 0.0f;
            }
            return val;
        }
    }

    return 0.0f;
}
