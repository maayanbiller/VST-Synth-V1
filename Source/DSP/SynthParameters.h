#pragma once
#include <juce_core/juce_core.h>

namespace IDs
{
    // Master
    static constexpr const char* MASTER_VOL = "MASTER_VOL";

    // Osc 1
    static constexpr const char* OSC1_WAVE = "OSC1_WAVE"; // 0:Sine, 1:Tri, 2:Saw, 3:Square
    static constexpr const char* OSC1_SEMI = "OSC1_SEMI"; // -24 to 24
    static constexpr const char* OSC1_DETUNE = "OSC1_DETUNE"; // -100 to 100 cents
    static constexpr const char* OSC1_LEVEL = "OSC1_LEVEL"; // 0 to 1
    static constexpr const char* OSC1_PWM = "OSC1_PWM"; // 0 to 1

    // Osc 2
    static constexpr const char* OSC2_WAVE = "OSC2_WAVE";
    static constexpr const char* OSC2_SEMI = "OSC2_SEMI";
    static constexpr const char* OSC2_DETUNE = "OSC2_DETUNE";
    static constexpr const char* OSC2_LEVEL = "OSC2_LEVEL";
    static constexpr const char* OSC2_PWM = "OSC2_PWM";

    // Sub Osc
    static constexpr const char* SUB_WAVE = "SUB_WAVE"; // 0:Sine, 1:Square
    static constexpr const char* SUB_OCT = "SUB_OCT"; // -1 or -2
    static constexpr const char* SUB_LEVEL = "SUB_LEVEL";

    // Noise
    static constexpr const char* NOISE_LEVEL = "NOISE_LEVEL";

    // Filter
    static constexpr const char* FILTER_TYPE = "FILTER_TYPE"; // 0:LP12, 1:LP24, 2:HP12, 3:HP24, 4:BP12, 5:Notch
    static constexpr const char* FILTER_CUTOFF = "FILTER_CUTOFF"; // 20Hz to 20kHz
    static constexpr const char* FILTER_RES = "FILTER_RES"; // 0 to 10
    static constexpr const char* FILTER_DRIVE = "FILTER_DRIVE"; // 1 to 10
    static constexpr const char* FILTER_ENV_AMT = "FILTER_ENV_AMT"; // -1 to 1
    static constexpr const char* FILTER_KEYTRACK = "FILTER_KEYTRACK"; // 0 to 1

    // Amp Envelope
    static constexpr const char* AMP_ATTACK = "AMP_ATTACK"; // 0 to 5s
    static constexpr const char* AMP_DECAY = "AMP_DECAY"; // 0 to 5s
    static constexpr const char* AMP_SUSTAIN = "AMP_SUSTAIN"; // 0 to 1
    static constexpr const char* AMP_RELEASE = "AMP_RELEASE"; // 0 to 5s

    // Filter Envelope
    static constexpr const char* FILT_ATTACK = "FILT_ATTACK";
    static constexpr const char* FILT_DECAY = "FILT_DECAY";
    static constexpr const char* FILT_SUSTAIN = "FILT_SUSTAIN";
    static constexpr const char* FILT_RELEASE = "FILT_RELEASE";

    // Mod Envelope
    static constexpr const char* MOD_ATTACK = "MOD_ATTACK";
    static constexpr const char* MOD_DECAY = "MOD_DECAY";
    static constexpr const char* MOD_SUSTAIN = "MOD_SUSTAIN";
    static constexpr const char* MOD_RELEASE = "MOD_RELEASE";

    // LFO 1
    static constexpr const char* LFO1_WAVE = "LFO1_WAVE"; // Sine, Tri, Saw, Square, Random
    static constexpr const char* LFO1_RATE = "LFO1_RATE"; // 0.1 to 50Hz
    static constexpr const char* LFO1_SYNC = "LFO1_SYNC"; // Bool
    static constexpr const char* LFO1_DEPTH = "LFO1_DEPTH"; // 0 to 1

    // LFO 2
    static constexpr const char* LFO2_WAVE = "LFO2_WAVE";
    static constexpr const char* LFO2_RATE = "LFO2_RATE";
    static constexpr const char* LFO2_SYNC = "LFO2_SYNC";
    static constexpr const char* LFO2_DEPTH = "LFO2_DEPTH";
}
