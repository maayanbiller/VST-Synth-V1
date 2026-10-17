#include "PluginProcessor.h"
#include "UI/PluginEditor.h"

VSTSynthV1AudioProcessor::VSTSynthV1AudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    synth.addSound(new SynthSound());
    for (int i = 0; i < 16; ++i) // 16 voices
    {
        auto* v = new SynthVoice();
        synth.addVoice(v);
        cachedVoicePointers.push_back(v);
    }
    synth.setNoteStealingEnabled(true);

    cachedParams.masterVol = apvts.getRawParameterValue(IDs::MASTER_VOL);
    cachedParams.noiseLevel = apvts.getRawParameterValue(IDs::NOISE_LEVEL);
    cachedParams.noiseColor = apvts.getRawParameterValue(IDs::NOISE_COLOR);
    cachedParams.playMode = apvts.getRawParameterValue(IDs::PLAY_MODE);
    cachedParams.glideTime = apvts.getRawParameterValue(IDs::GLIDE_TIME);

    cachedParams.oscWave[0] = apvts.getRawParameterValue(IDs::OSC1_WAVE);
    cachedParams.oscOct[0] = apvts.getRawParameterValue(IDs::OSC1_OCTAVE);
    cachedParams.oscDetune[0] = apvts.getRawParameterValue(IDs::OSC1_DETUNE);
    cachedParams.oscLevel[0] = apvts.getRawParameterValue(IDs::OSC1_LEVEL);
    cachedParams.oscPwm[0] = apvts.getRawParameterValue(IDs::OSC1_PWM);
    cachedParams.oscVoices[0] = apvts.getRawParameterValue(IDs::OSC1_VOICES);
    cachedParams.oscWidth[0] = apvts.getRawParameterValue(IDs::OSC1_WIDTH);

    cachedParams.oscWave[1] = apvts.getRawParameterValue(IDs::OSC2_WAVE);
    cachedParams.oscOct[1] = apvts.getRawParameterValue(IDs::OSC2_OCTAVE);
    cachedParams.oscDetune[1] = apvts.getRawParameterValue(IDs::OSC2_DETUNE);
    cachedParams.oscLevel[1] = apvts.getRawParameterValue(IDs::OSC2_LEVEL);
    cachedParams.oscPwm[1] = apvts.getRawParameterValue(IDs::OSC2_PWM);
    cachedParams.oscVoices[1] = apvts.getRawParameterValue(IDs::OSC2_VOICES);
    cachedParams.oscWidth[1] = apvts.getRawParameterValue(IDs::OSC2_WIDTH);

    cachedParams.oscWave[2] = apvts.getRawParameterValue(IDs::OSC3_WAVE);
    cachedParams.oscOct[2] = apvts.getRawParameterValue(IDs::OSC3_OCTAVE);
    cachedParams.oscDetune[2] = apvts.getRawParameterValue(IDs::OSC3_DETUNE);
    cachedParams.oscLevel[2] = apvts.getRawParameterValue(IDs::OSC3_LEVEL);
    cachedParams.oscPwm[2] = apvts.getRawParameterValue(IDs::OSC3_PWM);
    cachedParams.oscVoices[2] = apvts.getRawParameterValue(IDs::OSC3_VOICES);
    cachedParams.oscWidth[2] = apvts.getRawParameterValue(IDs::OSC3_WIDTH);

    cachedParams.oscWave[3] = apvts.getRawParameterValue(IDs::OSC4_WAVE);
    cachedParams.oscOct[3] = apvts.getRawParameterValue(IDs::OSC4_OCTAVE);
    cachedParams.oscDetune[3] = apvts.getRawParameterValue(IDs::OSC4_DETUNE);
    cachedParams.oscLevel[3] = apvts.getRawParameterValue(IDs::OSC4_LEVEL);
    cachedParams.oscPwm[3] = apvts.getRawParameterValue(IDs::OSC4_PWM);
    cachedParams.oscVoices[3] = apvts.getRawParameterValue(IDs::OSC4_VOICES);
    cachedParams.oscWidth[3] = apvts.getRawParameterValue(IDs::OSC4_WIDTH);

    cachedParams.filterType = apvts.getRawParameterValue(IDs::FILTER_TYPE);
    cachedParams.filterCutoff = apvts.getRawParameterValue(IDs::FILTER_CUTOFF);
    cachedParams.filterRes = apvts.getRawParameterValue(IDs::FILTER_RES);
    cachedParams.filterDrive = apvts.getRawParameterValue(IDs::FILTER_DRIVE);
    cachedParams.filterEnvAmt = apvts.getRawParameterValue(IDs::FILTER_ENV_AMT);
    cachedParams.filterKeyTrack = apvts.getRawParameterValue(IDs::FILTER_KEYTRACK);

    cachedParams.ampAttack = apvts.getRawParameterValue(IDs::AMP_ATTACK);
    cachedParams.ampDecay = apvts.getRawParameterValue(IDs::AMP_DECAY);
    cachedParams.ampSustain = apvts.getRawParameterValue(IDs::AMP_SUSTAIN);
    cachedParams.ampRelease = apvts.getRawParameterValue(IDs::AMP_RELEASE);

    cachedParams.filtAttack = apvts.getRawParameterValue(IDs::FILT_ATTACK);
    cachedParams.filtDecay = apvts.getRawParameterValue(IDs::FILT_DECAY);
    cachedParams.filtSustain = apvts.getRawParameterValue(IDs::FILT_SUSTAIN);
    cachedParams.filtRelease = apvts.getRawParameterValue(IDs::FILT_RELEASE);

    cachedParams.modAttack = apvts.getRawParameterValue(IDs::MOD_ATTACK);
    cachedParams.modDecay = apvts.getRawParameterValue(IDs::MOD_DECAY);
    cachedParams.modSustain = apvts.getRawParameterValue(IDs::MOD_SUSTAIN);
    cachedParams.modRelease = apvts.getRawParameterValue(IDs::MOD_RELEASE);

    cachedParams.lfoWave[0] = apvts.getRawParameterValue(IDs::LFO1_WAVE);
    cachedParams.lfoRate[0] = apvts.getRawParameterValue(IDs::LFO1_RATE);
    cachedParams.lfoDepth[0] = apvts.getRawParameterValue(IDs::LFO1_DEPTH);

    cachedParams.lfoWave[1] = apvts.getRawParameterValue(IDs::LFO2_WAVE);
    cachedParams.lfoRate[1] = apvts.getRawParameterValue(IDs::LFO2_RATE);
    cachedParams.lfoDepth[1] = apvts.getRawParameterValue(IDs::LFO2_DEPTH);

    cachedParams.modBusAmts[0] = apvts.getRawParameterValue(IDs::MOD_BUS_1_AMT);
    cachedParams.modBusAmts[1] = apvts.getRawParameterValue(IDs::MOD_BUS_2_AMT);
    cachedParams.modBusAmts[2] = apvts.getRawParameterValue(IDs::MOD_BUS_3_AMT);
    cachedParams.modBusAmts[3] = apvts.getRawParameterValue(IDs::MOD_BUS_4_AMT);

    for (int r = 0; r < 5; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            cachedParams.modMatrix[r][c] = apvts.getRawParameterValue(IDs::getModCellId(r, c));
        }
    }

    cachedParams.modSrc[0] = apvts.getRawParameterValue(IDs::MOD_SRC_1);
    cachedParams.modDst[0] = apvts.getRawParameterValue(IDs::MOD_DST_1);
    cachedParams.modAmts[0] = apvts.getRawParameterValue(IDs::MOD_AMT_1);

    cachedParams.modSrc[1] = apvts.getRawParameterValue(IDs::MOD_SRC_2);
    cachedParams.modDst[1] = apvts.getRawParameterValue(IDs::MOD_DST_2);
    cachedParams.modAmts[1] = apvts.getRawParameterValue(IDs::MOD_AMT_2);

    cachedParams.modSrc[2] = apvts.getRawParameterValue(IDs::MOD_SRC_3);
    cachedParams.modDst[2] = apvts.getRawParameterValue(IDs::MOD_DST_3);
    cachedParams.modAmts[2] = apvts.getRawParameterValue(IDs::MOD_AMT_3);

    cachedParams.modSrc[3] = apvts.getRawParameterValue(IDs::MOD_SRC_4);
    cachedParams.modDst[3] = apvts.getRawParameterValue(IDs::MOD_DST_4);
    cachedParams.modAmts[3] = apvts.getRawParameterValue(IDs::MOD_AMT_4);
}

VSTSynthV1AudioProcessor::~VSTSynthV1AudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VSTSynthV1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto floatParam = [](const char* id, juce::String name, float min, float max, float step, float defaultVal) {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name, juce::NormalisableRange<float>(min, max, step), defaultVal,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float value, int) {
                    if (value >= 10.0f || std::abs(value - std::round(value)) < 0.001f) 
                        return juce::String((int)std::round(value));
                    return juce::String(value, 2);
                }
            )
        );
    };

    // Master
    auto volAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) { return juce::String((int)std::round(val * 100.0f)) + "%"; }
    );
    juce::NormalisableRange<float> masterRange(0.0f, 1.0f, 0.005f);
    masterRange.setSkewForCentre(0.50f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::MASTER_VOL, 1), "Master Volume", masterRange, 0.7f, volAttrs));

    juce::NormalisableRange<float> noiseLevelRange(0.0f, 1.0f, 0.005f);
    noiseLevelRange.setSkewForCentre(0.50f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::NOISE_LEVEL, 1), "Noise Level", noiseLevelRange, 0.0f, volAttrs));

    auto noiseColorAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) {
            if (val <= -0.60f) return juce::String("Red");
            if (val <= -0.20f) return juce::String("Pink");
            if (val <=  0.20f) return juce::String("White");
            if (val <=  0.60f) return juce::String("Blue");
            return juce::String("Violet");
        }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::NOISE_COLOR, 1), "Noise Color", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f, noiseColorAttrs));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::PLAY_MODE, 1), "Play Mode", juce::StringArray{"Poly", "Mono", "Legato"}, 0));

    juce::NormalisableRange<float> glideRange(0.0f, 2.0f, 0.001f);
    glideRange.setSkewForCentre(0.12f);
    auto glideAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) {
            if (val < 0.001f) return juce::String("Off");
            if (val < 0.100f) return juce::String(val * 1000.0f, 1) + " ms";
            if (val < 1.0f)   return juce::String((int)std::round(val * 1000.0f)) + " ms";
            return juce::String(val, 2) + " s";
        }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::GLIDE_TIME, 1), "Glide Time", glideRange, 0.0f, glideAttrs));

    auto addOsc = [&](int idx, const char* waveId, const char* octId, const char* detuneId, const char* levelId, const char* pwmId, const char* voicesId, const char* widthId) {
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(waveId, 1), juce::String("Osc ") + juce::String(idx) + " Wave", 0, 3, 2));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(octId, 1), juce::String("Osc ") + juce::String(idx) + " Octave", -2, 2, 0));
        
        // Detune: -100 to +100 cents with symmetric fine control around 0
        juce::NormalisableRange<float> detuneRange(-100.0f, 100.0f,
            [](float, float, float norm) {
                float c = (norm - 0.5f) * 2.0f;
                return (c * c * c) * 100.0f;
            },
            [](float, float, float val) {
                float normVal = std::clamp(val / 100.0f, -1.0f, 1.0f);
                float c = std::cbrt(normVal);
                return (c * 0.5f) + 0.5f;
            }
        );
        auto detuneAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float val, int) {
                float r = std::round(val * 10.0f) / 10.0f;
                if (std::abs(r) < 0.05f) return juce::String("0 ct");
                if (r > 0.0f) return "+" + juce::String(r, 1) + " ct";
                return juce::String(r, 1) + " ct";
            }
        );
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(detuneId, 1), juce::String("Osc ") + juce::String(idx) + " Detune",
            detuneRange, 0.0f, detuneAttrs
        ));

        juce::NormalisableRange<float> oscLevelRange(0.0f, 1.0f, 0.005f);
        oscLevelRange.setSkewForCentre(0.50f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(levelId, 1), juce::String("Osc ") + juce::String(idx) + " Level",
            oscLevelRange, idx == 1 ? 0.7f : 0.0f, volAttrs
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(pwmId, 1), juce::String("Osc ") + juce::String(idx) + " PWM",
            juce::NormalisableRange<float>(0.05f, 0.95f, 0.005f), 0.5f, volAttrs
        ));
        
        // Unison Voices (1 to 8)
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(voicesId, 1), juce::String("Osc ") + juce::String(idx) + " Voices", 1, 8, 1
        ));

        // Unison Width (0 to 100%, skewed for fine control at lower values)
        juce::NormalisableRange<float> widthRange(0.0f, 1.0f, 0.005f);
        widthRange.setSkewForCentre(0.25f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(widthId, 1), juce::String("Osc ") + juce::String(idx) + " Width",
            widthRange, 0.5f, volAttrs
        ));
    };

    addOsc(1, IDs::OSC1_WAVE, IDs::OSC1_OCTAVE, IDs::OSC1_DETUNE, IDs::OSC1_LEVEL, IDs::OSC1_PWM, IDs::OSC1_VOICES, IDs::OSC1_WIDTH);
    addOsc(2, IDs::OSC2_WAVE, IDs::OSC2_OCTAVE, IDs::OSC2_DETUNE, IDs::OSC2_LEVEL, IDs::OSC2_PWM, IDs::OSC2_VOICES, IDs::OSC2_WIDTH);
    addOsc(3, IDs::OSC3_WAVE, IDs::OSC3_OCTAVE, IDs::OSC3_DETUNE, IDs::OSC3_LEVEL, IDs::OSC3_PWM, IDs::OSC3_VOICES, IDs::OSC3_WIDTH);
    addOsc(4, IDs::OSC4_WAVE, IDs::OSC4_OCTAVE, IDs::OSC4_DETUNE, IDs::OSC4_LEVEL, IDs::OSC4_PWM, IDs::OSC4_VOICES, IDs::OSC4_WIDTH);

    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(IDs::FILTER_TYPE, 1), "Filter Type", 0, 2, 0));

    juce::NormalisableRange<float> cutoffRange(20.0f, 20000.0f, 0.1f);
    cutoffRange.setSkewForCentre(1000.0f);
    auto cutoffAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) {
            if (val < 1000.0f) return juce::String((int)std::round(val)) + " Hz";
            return juce::String(val / 1000.0f, 2) + " kHz";
        }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::FILTER_CUTOFF, 1), "Cutoff", cutoffRange, 2000.0f, cutoffAttrs));

    juce::NormalisableRange<float> resRange(0.1f, 10.0f, 0.01f);
    resRange.setSkewForCentre(2.0f);
    auto resAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) { return juce::String(val, 2); }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::FILTER_RES, 1), "Resonance", resRange, 1.0f, resAttrs));

    juce::NormalisableRange<float> driveRange(1.0f, 10.0f, 0.01f);
    driveRange.setSkewForCentre(2.5f);
    auto driveAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) { return juce::String(val, 1) + "x"; }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::FILTER_DRIVE, 1), "Drive", driveRange, 1.0f, driveAttrs));

    juce::NormalisableRange<float> envAmtRange(-10000.0f, 10000.0f,
        [](float, float, float norm) {
            float c = (norm - 0.5f) * 2.0f;
            return (c * c * c) * 10000.0f;
        },
        [](float, float, float val) {
            float normVal = std::clamp(val / 10000.0f, -1.0f, 1.0f);
            float c = std::cbrt(normVal);
            return (c * 0.5f) + 0.5f;
        }
    );
    auto envAmtAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) {
            int v = (int)std::round(val);
            if (v == 0) return juce::String("0 Hz");
            return (v > 0 ? "+" : "") + juce::String(v) + " Hz";
        }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::FILTER_ENV_AMT, 1), "Env Amt", envAmtRange, 0.0f, envAmtAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(IDs::FILTER_KEYTRACK, 1), "Keytrack",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f, volAttrs
    ));

    auto addEnv = [&](const char* prefix, const char* a, const char* d, const char* s, const char* r) {
        // Skewed range from 0.000s to 5.000s with 0.1ms precision, 50% fader is 200ms
        juce::NormalisableRange<float> timeRange(0.000f, 5.0f, 0.0001f);
        timeRange.setSkewForCentre(0.20f);

        auto timeAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float val, int) {
                if (val < 0.0005f) return juce::String("0 ms");
                if (val < 0.100f)  return juce::String(val * 1000.0f, 1) + " ms";
                if (val < 1.0f)    return juce::String((int)std::round(val * 1000.0f)) + " ms";
                return juce::String(val, 2) + " s";
            }
        );

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(a, 1), juce::String(prefix) + " Attack", timeRange, 0.05f, timeAttrs
        ));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(d, 1), juce::String(prefix) + " Decay", timeRange, 0.2f, timeAttrs
        ));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(s, 1), juce::String(prefix) + " Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.005f), 0.8f, volAttrs
        ));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(r, 1), juce::String(prefix) + " Release", timeRange, 0.2f, timeAttrs
        ));
    };
    addEnv("Amp", IDs::AMP_ATTACK, IDs::AMP_DECAY, IDs::AMP_SUSTAIN, IDs::AMP_RELEASE);
    addEnv("Filt", IDs::FILT_ATTACK, IDs::FILT_DECAY, IDs::FILT_SUSTAIN, IDs::FILT_RELEASE);
    addEnv("Mod", IDs::MOD_ATTACK, IDs::MOD_DECAY, IDs::MOD_SUSTAIN, IDs::MOD_RELEASE);

    auto addLFO = [&](int idx, const char* w, const char* r, const char* s, const char* d) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(w, 1), juce::String("LFO") + juce::String(idx) + " Wave", juce::StringArray{"Sine", "Triangle", "Saw", "Square"}, 0));
        
        juce::NormalisableRange<float> lfoRateRange(0.01f, 100.0f, 0.01f);
        lfoRateRange.setSkewForCentre(2.0f);
        auto lfoRateAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float val, int) {
                if (val < 1.0f)  return juce::String(val, 2) + " Hz";
                if (val < 10.0f) return juce::String(val, 1) + " Hz";
                return juce::String((int)std::round(val)) + " Hz";
            }
        );
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(r, 1), juce::String("LFO") + juce::String(idx) + " Rate",
            lfoRateRange, 1.0f, lfoRateAttrs
        ));

        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(s, 1), juce::String("LFO") + juce::String(idx) + " Sync", false));
        
        juce::NormalisableRange<float> lfoDepthRange(0.0f, 2.0f, 0.005f);
        lfoDepthRange.setSkewForCentre(0.50f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(d, 1), juce::String("LFO") + juce::String(idx) + " Depth",
            lfoDepthRange, 1.0f, volAttrs
        ));
    };
    addLFO(1, IDs::LFO1_WAVE, IDs::LFO1_RATE, IDs::LFO1_SYNC, IDs::LFO1_DEPTH);
    addLFO(2, IDs::LFO2_WAVE, IDs::LFO2_RATE, IDs::LFO2_SYNC, IDs::LFO2_DEPTH);

    // Mod Matrix Side Bus Amounts (Amber, Green, Red, Purple)
    auto busAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float val, int) {
            int pct = (int)std::round(val * 100.0f);
            return (pct > 0 ? "+" : "") + juce::String(pct) + "%";
        }
    );
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::MOD_BUS_1_AMT, 1), "Bus 1 Amount", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f, busAttr));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::MOD_BUS_2_AMT, 1), "Bus 2 Amount", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f, busAttr));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::MOD_BUS_3_AMT, 1), "Bus 3 Amount", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f, busAttr));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::MOD_BUS_4_AMT, 1), "Bus 4 Amount", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f, busAttr));

    // 5x8 Mod Matrix Cells: 5 rows (sources) x 8 columns (destinations)
    for (int r = 0; r < 5; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            params.push_back(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(IDs::getModCellId(r, c), 1),
                "Matrix R" + juce::String(r) + " C" + juce::String(c),
                0, 5, 0
            ));
        }
    }

    // Legacy 4 slots
    auto addMod = [&](int idx, const char* src, const char* amt, const char* dst) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(src, 1), juce::String("Mod ") + juce::String(idx) + " Src", juce::StringArray{"LFO 1", "LFO 2", "Mod Env", "Velocity", "Aftertouch", "Mod Wheel", "Pitch Bend"}, 0));
        params.push_back(floatParam(amt, juce::String("Mod ") + juce::String(idx) + " Amt", -1.0f, 1.0f, 0.01f, 0.0f));
        params.push_back(floatParam(dst, juce::String("Mod ") + juce::String(idx) + " Dst", 0.0f, 8.0f, 1.0f, 0.0f));
    };
    addMod(1, IDs::MOD_SRC_1, IDs::MOD_AMT_1, IDs::MOD_DST_1);
    addMod(2, IDs::MOD_SRC_2, IDs::MOD_AMT_2, IDs::MOD_DST_2);
    addMod(3, IDs::MOD_SRC_3, IDs::MOD_AMT_3, IDs::MOD_DST_3);
    addMod(4, IDs::MOD_SRC_4, IDs::MOD_AMT_4, IDs::MOD_DST_4);

    return { params.begin(), params.end() };
}

void VSTSynthV1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    dcBlockerL.prepare(sampleRate);
    dcBlockerR.prepare(sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void VSTSynthV1AudioProcessor::releaseResources()
{
}

bool VSTSynthV1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
  #endif
}

void VSTSynthV1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    buffer.clear();

    // Pull parameters atomically (zero heap allocations, zero mutex locks)
    float mVol = cachedParams.masterVol ? cachedParams.masterVol->load() : 0.7f;

    DAHDSREnvelope::Parameters ampParams { 
        0.0f,
        cachedParams.ampAttack ? cachedParams.ampAttack->load() : 0.05f,
        0.0f,
        cachedParams.ampDecay ? cachedParams.ampDecay->load() : 0.2f,
        cachedParams.ampSustain ? cachedParams.ampSustain->load() : 1.0f,
        cachedParams.ampRelease ? cachedParams.ampRelease->load() : 0.2f
    };
    
    DAHDSREnvelope::Parameters filtParams { 
        0.0f,
        cachedParams.filtAttack ? cachedParams.filtAttack->load() : 0.05f,
        0.0f,
        cachedParams.filtDecay ? cachedParams.filtDecay->load() : 0.2f,
        cachedParams.filtSustain ? cachedParams.filtSustain->load() : 1.0f,
        cachedParams.filtRelease ? cachedParams.filtRelease->load() : 0.2f
    };
    
    DAHDSREnvelope::Parameters modParams { 
        0.0f,
        cachedParams.modAttack ? cachedParams.modAttack->load() : 0.05f,
        0.0f,
        cachedParams.modDecay ? cachedParams.modDecay->load() : 0.2f,
        cachedParams.modSustain ? cachedParams.modSustain->load() : 1.0f,
        cachedParams.modRelease ? cachedParams.modRelease->load() : 0.2f
    };

    GlobalSynthParams params;
    for (int i = 0; i < 4; ++i)
    {
        params.oscOct[i]    = cachedParams.oscOct[i]    ? cachedParams.oscOct[i]->load() : 0.0f;
        params.oscDetune[i] = cachedParams.oscDetune[i] ? cachedParams.oscDetune[i]->load() : 0.0f;
        params.oscLevel[i]  = cachedParams.oscLevel[i]  ? cachedParams.oscLevel[i]->load() : (i == 0 ? 1.0f : 0.0f);
        params.oscPwm[i]    = cachedParams.oscPwm[i]    ? cachedParams.oscPwm[i]->load() : 0.5f;
        params.oscWave[i]   = cachedParams.oscWave[i]   ? static_cast<int>(cachedParams.oscWave[i]->load()) : 2;
        params.oscVoices[i] = cachedParams.oscVoices[i] ? static_cast<int>(cachedParams.oscVoices[i]->load()) : 1;
        params.oscWidth[i]  = cachedParams.oscWidth[i]  ? cachedParams.oscWidth[i]->load() : 0.5f;
    }

    for (int b = 0; b < 4; ++b)
        params.modBusAmts[b] = cachedParams.modBusAmts[b] ? cachedParams.modBusAmts[b]->load() : 0.0f;

    for (int r = 0; r < 5; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            if (auto* val = cachedParams.modMatrix[r][c])
                params.modMatrix[r][c] = static_cast<int>(val->load());
            else
                params.modMatrix[r][c] = 0;
        }
    }

    params.noiseLevel = cachedParams.noiseLevel ? cachedParams.noiseLevel->load() : 0.0f;
    params.noiseColor = cachedParams.noiseColor ? cachedParams.noiseColor->load() : 0.0f;
    params.playMode   = cachedParams.playMode   ? static_cast<int>(cachedParams.playMode->load()) : 0;
    params.glideTime  = cachedParams.glideTime  ? cachedParams.glideTime->load() : 0.0f;

    params.filterType     = cachedParams.filterType     ? static_cast<int>(cachedParams.filterType->load()) : 0;
    params.filterCutoff   = cachedParams.filterCutoff   ? cachedParams.filterCutoff->load() : 1000.0f;
    params.filterRes      = cachedParams.filterRes      ? cachedParams.filterRes->load() : 1.0f;
    params.filterDrive    = cachedParams.filterDrive    ? cachedParams.filterDrive->load() : 1.0f;
    params.filterEnvAmt   = cachedParams.filterEnvAmt   ? cachedParams.filterEnvAmt->load() : 0.0f;
    params.filterKeyTrack = cachedParams.filterKeyTrack ? cachedParams.filterKeyTrack->load() : 0.0f;

    params.ampParams = ampParams;
    params.filtParams = filtParams;
    params.modParams = modParams;

    for (int l = 0; l < 2; ++l)
    {
        params.lfoRate[l]  = cachedParams.lfoRate[l]  ? cachedParams.lfoRate[l]->load() : 1.0f;
        params.lfoDepth[l] = cachedParams.lfoDepth[l] ? cachedParams.lfoDepth[l]->load() : 1.0f;
        params.lfoWave[l]  = cachedParams.lfoWave[l]  ? static_cast<int>(cachedParams.lfoWave[l]->load()) : 0;
    }

    for (int m = 0; m < 4; ++m)
    {
        params.modSrc[m]  = cachedParams.modSrc[m]  ? static_cast<int>(cachedParams.modSrc[m]->load()) : 0;
        params.modDst[m]  = cachedParams.modDst[m]  ? static_cast<int>(cachedParams.modDst[m]->load()) : 0;
        params.modAmts[m] = cachedParams.modAmts[m] ? cachedParams.modAmts[m]->load() : 0.0f;
    }

    // Parse active modulation routes ONCE per block (instead of 64 times per block across 16 voices)
    params.numActiveModRoutes = 0;
    for (int r = 0; r < 5; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            int cell = params.modMatrix[r][c];
            if (cell <= 0) continue;
            float amt = 0.0f;
            if (cell == 1) amt = 1.0f;
            else if (cell >= 2 && cell <= 5) amt = params.modBusAmts[cell - 2];

            if (amt != 0.0f && params.numActiveModRoutes < 44)
            {
                params.activeModRoutes[params.numActiveModRoutes++] = {
                    static_cast<uint8_t>(r),
                    static_cast<uint8_t>(c),
                    amt
                };
            }
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        int sIdx = params.modSrc[i];
        int dIdx = params.modDst[i];
        float amt = params.modAmts[i];
        if (sIdx > 0 && sIdx <= 5 && dIdx > 0 && dIdx <= 8 && amt != 0.0f && params.numActiveModRoutes < 44)
        {
            params.activeModRoutes[params.numActiveModRoutes++] = {
                static_cast<uint8_t>(sIdx - 1),
                static_cast<uint8_t>(dIdx - 1),
                amt
            };
        }
    }

    synth.setPlayMode(params.playMode);

    for (auto* voice : cachedVoicePointers)
    {
        voice->updateBaseParameters(params);
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume
    buffer.applyGain(mVol);

    // Apply transparent 5Hz DC blocking to guarantee pure zero-voltage center at audio interface
    if (totalNumOutputChannels > 0)
    {
        auto* ch0 = buffer.getWritePointer(0);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
            ch0[s] = dcBlockerL.processSample(ch0[s]);
    }
    if (totalNumOutputChannels > 1)
    {
        auto* ch1 = buffer.getWritePointer(1);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
            ch1[s] = dcBlockerR.processSample(ch1[s]);
    }
}

bool VSTSynthV1AudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* VSTSynthV1AudioProcessor::createEditor()
{
    return new VSTSynthV1AudioProcessorEditor (*this);
}

const juce::String VSTSynthV1AudioProcessor::getName() const { return "VST Synth V1"; }
bool VSTSynthV1AudioProcessor::acceptsMidi() const { return true; }
bool VSTSynthV1AudioProcessor::producesMidi() const { return false; }
bool VSTSynthV1AudioProcessor::isMidiEffect() const { return false; }
double VSTSynthV1AudioProcessor::getTailLengthSeconds() const { return 0.0; }
int VSTSynthV1AudioProcessor::getNumPrograms() { return 1; }
int VSTSynthV1AudioProcessor::getCurrentProgram() { return 0; }
void VSTSynthV1AudioProcessor::setCurrentProgram (int) {}
const juce::String VSTSynthV1AudioProcessor::getProgramName (int) { return {}; }
void VSTSynthV1AudioProcessor::changeProgramName (int, const juce::String&) {}

void VSTSynthV1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VSTSynthV1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VSTSynthV1AudioProcessor();
}

