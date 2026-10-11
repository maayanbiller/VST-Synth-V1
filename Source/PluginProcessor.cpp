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
        synth.addVoice(new SynthVoice());
    }
    synth.setNoteStealingEnabled(true);

    masterVol = apvts.getRawParameterValue(IDs::MASTER_VOL);
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
    params.push_back(floatParam(IDs::MASTER_VOL, "Master Volume", 0.0f, 1.0f, 0.01f, 0.7f));
    params.push_back(floatParam(IDs::NOISE_LEVEL, "Noise Level", 0.0f, 1.0f, 0.01f, 0.0f));
    params.push_back(floatParam(IDs::NOISE_COLOR, "Noise Color", -1.0f, 1.0f, 0.01f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::PLAY_MODE, 1), "Play Mode", juce::StringArray{"Poly", "Mono", "Legato"}, 0));
    params.push_back(floatParam(IDs::GLIDE_TIME, "Glide Time", 0.0f, 2.0f, 0.01f, 0.0f));

    auto addOsc = [&](int idx, const char* waveId, const char* octId, const char* detuneId, const char* levelId, const char* pwmId, const char* voicesId, const char* widthId) {
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(waveId, 1), juce::String("Osc ") + juce::String(idx) + " Wave", 0, 3, 2));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(octId, 1), juce::String("Osc ") + juce::String(idx) + " Octave", -2, 2, 0));
        
        // Detune in cents (-100 to +100 cents, +/- 1 semitone)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(detuneId, 1), juce::String("Osc ") + juce::String(idx) + " Detune",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float val, int) {
                    int c = (int)std::round(val);
                    return (c > 0 ? "+" : "") + juce::String(c) + " ct";
                }
            )
        ));

        params.push_back(floatParam(levelId, juce::String("Osc ") + juce::String(idx) + " Level", 0.0f, 1.0f, 0.01f, idx == 1 ? 0.7f : 0.0f));
        params.push_back(floatParam(pwmId, juce::String("Osc ") + juce::String(idx) + " PWM", 0.01f, 0.99f, 0.01f, 0.5f));
        
        // Unison Voices (1 to 8)
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(voicesId, 1), juce::String("Osc ") + juce::String(idx) + " Voices", 1, 8, 1
        ));

        // Unison Width (0 to 100%)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(widthId, 1), juce::String("Osc ") + juce::String(idx) + " Width",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float val, int) { return juce::String((int)std::round(val * 100.0f)) + "%"; }
            )
        ));
    };

    addOsc(1, IDs::OSC1_WAVE, IDs::OSC1_OCTAVE, IDs::OSC1_DETUNE, IDs::OSC1_LEVEL, IDs::OSC1_PWM, IDs::OSC1_VOICES, IDs::OSC1_WIDTH);
    addOsc(2, IDs::OSC2_WAVE, IDs::OSC2_OCTAVE, IDs::OSC2_DETUNE, IDs::OSC2_LEVEL, IDs::OSC2_PWM, IDs::OSC2_VOICES, IDs::OSC2_WIDTH);
    addOsc(3, IDs::OSC3_WAVE, IDs::OSC3_OCTAVE, IDs::OSC3_DETUNE, IDs::OSC3_LEVEL, IDs::OSC3_PWM, IDs::OSC3_VOICES, IDs::OSC3_WIDTH);
    addOsc(4, IDs::OSC4_WAVE, IDs::OSC4_OCTAVE, IDs::OSC4_DETUNE, IDs::OSC4_LEVEL, IDs::OSC4_PWM, IDs::OSC4_VOICES, IDs::OSC4_WIDTH);

    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(IDs::FILTER_TYPE, 1), "Filter Type", 0, 2, 0));
    params.push_back(floatParam(IDs::FILTER_CUTOFF, "Cutoff", 20.0f, 20000.0f, 1.0f, 2000.0f));
    params.push_back(floatParam(IDs::FILTER_RES, "Resonance", 0.1f, 10.0f, 0.01f, 1.0f));
    params.push_back(floatParam(IDs::FILTER_DRIVE, "Drive", 1.0f, 10.0f, 0.01f, 1.0f));
    params.push_back(floatParam(IDs::FILTER_ENV_AMT, "Env Amt", -10000.0f, 10000.0f, 1.0f, 0.0f));
    params.push_back(floatParam(IDs::FILTER_KEYTRACK, "Keytrack", 0.0f, 1.0f, 0.01f, 0.0f));

    auto addEnv = [&](const char* prefix, const char* a, const char* d, const char* s, const char* r) {
        // Skewed range so 50% fader is 0.25s (250ms), giving high precision for fast times
        juce::NormalisableRange<float> timeRange(0.001f, 5.0f, 0.001f);
        timeRange.setSkewForCentre(0.25f);

        auto timeAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float val, int) {
                if (val < 1.0f)
                    return juce::String((int)std::round(val * 1000.0f)) + " ms";
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
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float val, int) { return juce::String((int)std::round(val * 100.0f)) + "%"; }
            )
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
        params.push_back(floatParam(r, juce::String("LFO") + juce::String(idx) + " Rate", 0.1f, 50.0f, 0.1f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(s, 1), juce::String("LFO") + juce::String(idx) + " Sync", false));
        params.push_back(floatParam(d, juce::String("LFO") + juce::String(idx) + " Depth", 0.0f, 1.0f, 0.01f, 1.0f));
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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Pull parameters
    float mVol = masterVol->load();

    DAHDSREnvelope::Parameters ampParams { 
        0.0f, apvts.getRawParameterValue(IDs::AMP_ATTACK)->load(), 0.0f,
        apvts.getRawParameterValue(IDs::AMP_DECAY)->load(), apvts.getRawParameterValue(IDs::AMP_SUSTAIN)->load(),
        apvts.getRawParameterValue(IDs::AMP_RELEASE)->load()
    };
    
    DAHDSREnvelope::Parameters filtParams { 
        0.0f, apvts.getRawParameterValue(IDs::FILT_ATTACK)->load(), 0.0f,
        apvts.getRawParameterValue(IDs::FILT_DECAY)->load(), apvts.getRawParameterValue(IDs::FILT_SUSTAIN)->load(),
        apvts.getRawParameterValue(IDs::FILT_RELEASE)->load()
    };
    
    DAHDSREnvelope::Parameters modParams { 
        0.0f, apvts.getRawParameterValue(IDs::MOD_ATTACK)->load(), 0.0f,
        apvts.getRawParameterValue(IDs::MOD_DECAY)->load(), apvts.getRawParameterValue(IDs::MOD_SUSTAIN)->load(),
        apvts.getRawParameterValue(IDs::MOD_RELEASE)->load()
    };

    GlobalSynthParams params;
    params.oscOct[0] = apvts.getRawParameterValue(IDs::OSC1_OCTAVE)->load();
    params.oscDetune[0] = apvts.getRawParameterValue(IDs::OSC1_DETUNE)->load();
    params.oscLevel[0] = apvts.getRawParameterValue(IDs::OSC1_LEVEL)->load();
    params.oscPwm[0] = apvts.getRawParameterValue(IDs::OSC1_PWM)->load();
    params.oscWave[0] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC1_WAVE)->load());

    params.oscOct[1] = apvts.getRawParameterValue(IDs::OSC2_OCTAVE)->load();
    params.oscDetune[1] = apvts.getRawParameterValue(IDs::OSC2_DETUNE)->load();
    params.oscLevel[1] = apvts.getRawParameterValue(IDs::OSC2_LEVEL)->load();
    params.oscPwm[1] = apvts.getRawParameterValue(IDs::OSC2_PWM)->load();
    params.oscWave[1] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC2_WAVE)->load());

    params.oscOct[2] = apvts.getRawParameterValue(IDs::OSC3_OCTAVE)->load();
    params.oscDetune[2] = apvts.getRawParameterValue(IDs::OSC3_DETUNE)->load();
    params.oscLevel[2] = apvts.getRawParameterValue(IDs::OSC3_LEVEL)->load();
    params.oscPwm[2] = apvts.getRawParameterValue(IDs::OSC3_PWM)->load();
    params.oscWave[2] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC3_WAVE)->load());

    params.oscOct[3] = apvts.getRawParameterValue(IDs::OSC4_OCTAVE)->load();
    params.oscDetune[3] = apvts.getRawParameterValue(IDs::OSC4_DETUNE)->load();
    params.oscLevel[3] = apvts.getRawParameterValue(IDs::OSC4_LEVEL)->load();
    params.oscPwm[3] = apvts.getRawParameterValue(IDs::OSC4_PWM)->load();
    params.oscWave[3] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC4_WAVE)->load());

    params.oscVoices[0] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC1_VOICES)->load());
    params.oscWidth[0] = apvts.getRawParameterValue(IDs::OSC1_WIDTH)->load();
    params.oscVoices[1] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC2_VOICES)->load());
    params.oscWidth[1] = apvts.getRawParameterValue(IDs::OSC2_WIDTH)->load();
    params.oscVoices[2] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC3_VOICES)->load());
    params.oscWidth[2] = apvts.getRawParameterValue(IDs::OSC3_WIDTH)->load();
    params.oscVoices[3] = static_cast<int>(apvts.getRawParameterValue(IDs::OSC4_VOICES)->load());
    params.oscWidth[3] = apvts.getRawParameterValue(IDs::OSC4_WIDTH)->load();

    params.modBusAmts[0] = apvts.getRawParameterValue(IDs::MOD_BUS_1_AMT)->load();
    params.modBusAmts[1] = apvts.getRawParameterValue(IDs::MOD_BUS_2_AMT)->load();
    params.modBusAmts[2] = apvts.getRawParameterValue(IDs::MOD_BUS_3_AMT)->load();
    params.modBusAmts[3] = apvts.getRawParameterValue(IDs::MOD_BUS_4_AMT)->load();

    for (int r = 0; r < 5; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            if (auto* val = apvts.getRawParameterValue(IDs::getModCellId(r, c)))
                params.modMatrix[r][c] = static_cast<int>(val->load());
            else
                params.modMatrix[r][c] = 0;
        }
    }

    params.noiseLevel = apvts.getRawParameterValue(IDs::NOISE_LEVEL)->load();
    params.noiseColor = apvts.getRawParameterValue(IDs::NOISE_COLOR)->load();
    params.playMode = static_cast<int>(apvts.getRawParameterValue(IDs::PLAY_MODE)->load());
    params.glideTime = apvts.getRawParameterValue(IDs::GLIDE_TIME)->load();

    params.filterType = static_cast<int>(apvts.getRawParameterValue(IDs::FILTER_TYPE)->load());
    params.filterCutoff = apvts.getRawParameterValue(IDs::FILTER_CUTOFF)->load();
    params.filterRes = apvts.getRawParameterValue(IDs::FILTER_RES)->load();
    params.filterDrive = apvts.getRawParameterValue(IDs::FILTER_DRIVE)->load();
    params.filterEnvAmt = apvts.getRawParameterValue(IDs::FILTER_ENV_AMT)->load();
    params.filterKeyTrack = apvts.getRawParameterValue(IDs::FILTER_KEYTRACK)->load();

    params.ampParams = ampParams;
    params.filtParams = filtParams;
    params.modParams = modParams;

    params.lfoRate[0] = apvts.getRawParameterValue(IDs::LFO1_RATE)->load();
    params.lfoDepth[0] = apvts.getRawParameterValue(IDs::LFO1_DEPTH)->load();
    params.lfoWave[0] = static_cast<int>(apvts.getRawParameterValue(IDs::LFO1_WAVE)->load());

    params.lfoRate[1] = apvts.getRawParameterValue(IDs::LFO2_RATE)->load();
    params.lfoDepth[1] = apvts.getRawParameterValue(IDs::LFO2_DEPTH)->load();
    params.lfoWave[1] = static_cast<int>(apvts.getRawParameterValue(IDs::LFO2_WAVE)->load());

    params.modSrc[0] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_SRC_1)->load());
    params.modDst[0] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_DST_1)->load());
    params.modAmts[0] = apvts.getRawParameterValue(IDs::MOD_AMT_1)->load();

    params.modSrc[1] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_SRC_2)->load());
    params.modDst[1] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_DST_2)->load());
    params.modAmts[1] = apvts.getRawParameterValue(IDs::MOD_AMT_2)->load();

    params.modSrc[2] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_SRC_3)->load());
    params.modDst[2] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_DST_3)->load());
    params.modAmts[2] = apvts.getRawParameterValue(IDs::MOD_AMT_3)->load();

    params.modSrc[3] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_SRC_4)->load());
    params.modDst[3] = static_cast<int>(apvts.getRawParameterValue(IDs::MOD_DST_4)->load());
    params.modAmts[3] = apvts.getRawParameterValue(IDs::MOD_AMT_4)->load();

    synth.setPlayMode(params.playMode);

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->updateBaseParameters(params);
        }
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume
    buffer.applyGain(mVol);
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

