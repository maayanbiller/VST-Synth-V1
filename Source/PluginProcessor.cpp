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
    for (int i = 0; i < 6; ++i) // 6 voices
    {
        synth.addVoice(new SynthVoice());
    }

    masterVol = apvts.getRawParameterValue(IDs::MASTER_VOL);
}

VSTSynthV1AudioProcessor::~VSTSynthV1AudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VSTSynthV1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::MASTER_VOL, "Master Volume", 0.0f, 1.0f, 0.5f));
    
    // Osc 1
    params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::OSC1_WAVE, "Osc1 Wave", juce::StringArray{"Sine", "Triangle", "Saw", "Square"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC1_SEMI, "Osc1 Semi", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC1_DETUNE, "Osc1 Detune", -100.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC1_LEVEL, "Osc1 Level", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC1_PWM, "Osc1 PWM", 0.05f, 0.95f, 0.5f));

    // Osc 2
    params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::OSC2_WAVE, "Osc2 Wave", juce::StringArray{"Sine", "Triangle", "Saw", "Square"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC2_SEMI, "Osc2 Semi", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC2_DETUNE, "Osc2 Detune", -100.0f, 100.0f, 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC2_LEVEL, "Osc2 Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::OSC2_PWM, "Osc2 PWM", 0.05f, 0.95f, 0.5f));

    // Sub
    params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::SUB_WAVE, "Sub Wave", juce::StringArray{"Sine", "Square"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::SUB_OCT, "Sub Octave", -2.0f, -1.0f, -1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::SUB_LEVEL, "Sub Level", 0.0f, 1.0f, 0.0f));

    // Noise
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::NOISE_LEVEL, "Noise Level", 0.0f, 1.0f, 0.0f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::FILTER_TYPE, "Filter Type", juce::StringArray{"LP12", "LP24", "HP12", "HP24", "BP12", "Notch"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILTER_CUTOFF, "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILTER_RES, "Resonance", 0.0f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILTER_DRIVE, "Drive", 1.0f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILTER_ENV_AMT, "Filter Env Amt", -1.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILTER_KEYTRACK, "Filter Keytrack", 0.0f, 1.0f, 0.0f));

    // Amp Env
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::AMP_ATTACK, "Amp Attack", 0.001f, 5.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::AMP_DECAY, "Amp Decay", 0.001f, 5.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::AMP_SUSTAIN, "Amp Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::AMP_RELEASE, "Amp Release", 0.001f, 5.0f, 0.5f));

    // Filt Env
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILT_ATTACK, "Filt Attack", 0.001f, 5.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILT_DECAY, "Filt Decay", 0.001f, 5.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILT_SUSTAIN, "Filt Sustain", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::FILT_RELEASE, "Filt Release", 0.001f, 5.0f, 0.5f));

    // Mod Env
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::MOD_ATTACK, "Mod Attack", 0.001f, 5.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::MOD_DECAY, "Mod Decay", 0.001f, 5.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::MOD_SUSTAIN, "Mod Sustain", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::MOD_RELEASE, "Mod Release", 0.001f, 5.0f, 0.5f));

    // LFO 1
    params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::LFO1_WAVE, "LFO1 Wave", juce::StringArray{"Sine", "Triangle", "Saw", "Square", "Random"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::LFO1_RATE, "LFO1 Rate", 0.1f, 50.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(IDs::LFO1_SYNC, "LFO1 Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::LFO1_DEPTH, "LFO1 Depth", 0.0f, 1.0f, 1.0f));

    // LFO 2
    params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::LFO2_WAVE, "LFO2 Wave", juce::StringArray{"Sine", "Triangle", "Saw", "Square", "Random"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::LFO2_RATE, "LFO2 Rate", 0.1f, 50.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(IDs::LFO2_SYNC, "LFO2 Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::LFO2_DEPTH, "LFO2 Depth", 0.0f, 1.0f, 1.0f));

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

    std::array<ModRoute, 8> emptyRoutes; // We'll hook up actual mod matrix from APVTS later

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->updateBaseParameters(
                apvts.getRawParameterValue(IDs::OSC1_SEMI)->load(),
                apvts.getRawParameterValue(IDs::OSC1_DETUNE)->load(),
                apvts.getRawParameterValue(IDs::OSC1_LEVEL)->load(),
                apvts.getRawParameterValue(IDs::OSC1_PWM)->load(),
                static_cast<PolyBLEPOscillator::Waveform>(apvts.getRawParameterValue(IDs::OSC1_WAVE)->load()),
                
                apvts.getRawParameterValue(IDs::OSC2_SEMI)->load(),
                apvts.getRawParameterValue(IDs::OSC2_DETUNE)->load(),
                apvts.getRawParameterValue(IDs::OSC2_LEVEL)->load(),
                apvts.getRawParameterValue(IDs::OSC2_PWM)->load(),
                static_cast<PolyBLEPOscillator::Waveform>(apvts.getRawParameterValue(IDs::OSC2_WAVE)->load()),
                
                apvts.getRawParameterValue(IDs::SUB_LEVEL)->load(),
                apvts.getRawParameterValue(IDs::SUB_OCT)->load(),
                static_cast<PolyBLEPOscillator::Waveform>(apvts.getRawParameterValue(IDs::SUB_WAVE)->load()),
                
                apvts.getRawParameterValue(IDs::NOISE_LEVEL)->load(),
                
                static_cast<SynthFilter::Type>(apvts.getRawParameterValue(IDs::FILTER_TYPE)->load()),
                apvts.getRawParameterValue(IDs::FILTER_CUTOFF)->load(),
                apvts.getRawParameterValue(IDs::FILTER_RES)->load(),
                apvts.getRawParameterValue(IDs::FILTER_DRIVE)->load(),
                apvts.getRawParameterValue(IDs::FILTER_ENV_AMT)->load(),
                apvts.getRawParameterValue(IDs::FILTER_KEYTRACK)->load(),
                
                ampParams, filtParams, modParams,
                
                apvts.getRawParameterValue(IDs::LFO1_RATE)->load(),
                static_cast<LFO::Waveform>(apvts.getRawParameterValue(IDs::LFO1_WAVE)->load()),
                apvts.getRawParameterValue(IDs::LFO1_DEPTH)->load(),
                
                apvts.getRawParameterValue(IDs::LFO2_RATE)->load(),
                static_cast<LFO::Waveform>(apvts.getRawParameterValue(IDs::LFO2_WAVE)->load()),
                apvts.getRawParameterValue(IDs::LFO2_DEPTH)->load(),
                
                emptyRoutes
            );
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
