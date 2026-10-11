#pragma once
#include <juce_core/juce_core.h>

namespace IDs
{
    // Master
    static constexpr const char* MASTER_VOL = "MASTER_VOL";
    static constexpr const char* NOISE_LEVEL = "NOISE_LEVEL";
    static constexpr const char* NOISE_COLOR = "NOISE_COLOR";

    // Play Mode
    static constexpr const char* PLAY_MODE = "PLAY_MODE"; 
    static constexpr const char* GLIDE_TIME = "GLIDE_TIME"; 

    // Osc 1
    static constexpr const char* OSC1_WAVE = "OSC1_WAVE";
    static constexpr const char* OSC1_OCTAVE = "OSC1_OCTAVE";
    static constexpr const char* OSC1_DETUNE = "OSC1_DETUNE";
    static constexpr const char* OSC1_LEVEL = "OSC1_LEVEL";
    static constexpr const char* OSC1_PWM = "OSC1_PWM";
    static constexpr const char* OSC1_VOICES = "OSC1_VOICES";
    static constexpr const char* OSC1_WIDTH = "OSC1_WIDTH";

    // Osc 2
    static constexpr const char* OSC2_WAVE = "OSC2_WAVE";
    static constexpr const char* OSC2_OCTAVE = "OSC2_OCTAVE";
    static constexpr const char* OSC2_DETUNE = "OSC2_DETUNE";
    static constexpr const char* OSC2_LEVEL = "OSC2_LEVEL";
    static constexpr const char* OSC2_PWM = "OSC2_PWM";
    static constexpr const char* OSC2_VOICES = "OSC2_VOICES";
    static constexpr const char* OSC2_WIDTH = "OSC2_WIDTH";

    // Osc 3
    static constexpr const char* OSC3_WAVE = "OSC3_WAVE";
    static constexpr const char* OSC3_OCTAVE = "OSC3_OCTAVE";
    static constexpr const char* OSC3_DETUNE = "OSC3_DETUNE";
    static constexpr const char* OSC3_LEVEL = "OSC3_LEVEL";
    static constexpr const char* OSC3_PWM = "OSC3_PWM";
    static constexpr const char* OSC3_VOICES = "OSC3_VOICES";
    static constexpr const char* OSC3_WIDTH = "OSC3_WIDTH";

    // Osc 4
    static constexpr const char* OSC4_WAVE = "OSC4_WAVE";
    static constexpr const char* OSC4_OCTAVE = "OSC4_OCTAVE";
    static constexpr const char* OSC4_DETUNE = "OSC4_DETUNE";
    static constexpr const char* OSC4_LEVEL = "OSC4_LEVEL";
    static constexpr const char* OSC4_PWM = "OSC4_PWM";
    static constexpr const char* OSC4_VOICES = "OSC4_VOICES";
    static constexpr const char* OSC4_WIDTH = "OSC4_WIDTH";

    // Filter
    static constexpr const char* FILTER_TYPE = "FILTER_TYPE"; 
    static constexpr const char* FILTER_CUTOFF = "FILTER_CUTOFF"; 
    static constexpr const char* FILTER_RES = "FILTER_RES"; 
    static constexpr const char* FILTER_DRIVE = "FILTER_DRIVE"; 
    static constexpr const char* FILTER_ENV_AMT = "FILTER_ENV_AMT"; 
    static constexpr const char* FILTER_KEYTRACK = "FILTER_KEYTRACK"; 

    // Amp Envelope
    static constexpr const char* AMP_ATTACK = "AMP_ATTACK"; 
    static constexpr const char* AMP_DECAY = "AMP_DECAY"; 
    static constexpr const char* AMP_SUSTAIN = "AMP_SUSTAIN"; 
    static constexpr const char* AMP_RELEASE = "AMP_RELEASE"; 

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
    static constexpr const char* LFO1_WAVE = "LFO1_WAVE"; 
    static constexpr const char* LFO1_RATE = "LFO1_RATE"; 
    static constexpr const char* LFO1_SYNC = "LFO1_SYNC"; 
    static constexpr const char* LFO1_DEPTH = "LFO1_DEPTH"; 

    // LFO 2
    static constexpr const char* LFO2_WAVE = "LFO2_WAVE";
    static constexpr const char* LFO2_RATE = "LFO2_RATE";
    static constexpr const char* LFO2_SYNC = "LFO2_SYNC";
    static constexpr const char* LFO2_DEPTH = "LFO2_DEPTH";

    // Mod Matrix Side Bus Amounts (Amber, Green, Red, Purple)
    static constexpr const char* MOD_BUS_1_AMT = "MOD_BUS_1_AMT";
    static constexpr const char* MOD_BUS_2_AMT = "MOD_BUS_2_AMT";
    static constexpr const char* MOD_BUS_3_AMT = "MOD_BUS_3_AMT";
    static constexpr const char* MOD_BUS_4_AMT = "MOD_BUS_4_AMT";

    // Legacy matrix slots for backwards compatibility
    static constexpr const char* MOD_AMT_1 = "MOD_AMT_1";
    static constexpr const char* MOD_AMT_2 = "MOD_AMT_2";
    static constexpr const char* MOD_AMT_3 = "MOD_AMT_3";
    static constexpr const char* MOD_AMT_4 = "MOD_AMT_4";
    static constexpr const char* MOD_SRC_1 = "MOD_SRC_1";
    static constexpr const char* MOD_DST_1 = "MOD_DST_1";
    static constexpr const char* MOD_SRC_2 = "MOD_SRC_2";
    static constexpr const char* MOD_DST_2 = "MOD_DST_2";
    static constexpr const char* MOD_SRC_3 = "MOD_SRC_3";
    static constexpr const char* MOD_DST_3 = "MOD_DST_3";
    static constexpr const char* MOD_SRC_4 = "MOD_SRC_4";
    static constexpr const char* MOD_DST_4 = "MOD_DST_4";

    // Helper for 5x8 Matrix cell IDs: "MOD_R0_C0" through "MOD_R4_C7"
    inline juce::String getModCellId(int row, int col)
    {
        return "MOD_R" + juce::String(row) + "_C" + juce::String(col);
    }
}
