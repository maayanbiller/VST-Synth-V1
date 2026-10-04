#pragma once
#include <juce_core/juce_core.h>

enum class ModSource
{
    None = 0,
    LFO1,
    LFO2,
    EnvFilter,
    EnvMod,
    Velocity,
    KeyTrack,
    PitchBend,
    ModWheel
};

enum class ModDestination
{
    None = 0,
    Osc1Pitch,
    Osc2Pitch,
    Osc1PWM,
    Osc2PWM,
    Osc1Level,
    Osc2Level,
    FilterCutoff,
    FilterRes,
    FilterDrive
};

struct ModRoute
{
    ModSource source = ModSource::None;
    ModDestination destination = ModDestination::None;
    float amount = 0.0f; // -1 to 1
};
