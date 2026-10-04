#include "SynthVoice.h"

SynthVoice::SynthVoice() {}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    juce::ignoreUnused(sound);
    currentVelocity = velocity;
    currentMidiNote = midiNoteNumber;
    pitchWheelMoved(currentPitchWheelPosition);

    ampEnv.noteOn();
    filterEnv.noteOn();
    modEnv.noteOn();
}

void SynthVoice::stopNote (float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity);
    if (allowTailOff)
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
        modEnv.noteOff();
    }
    else
    {
        clearCurrentNote();
        ampEnv.reset();
        filterEnv.reset();
        modEnv.reset();
    }
}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue)
{
    // Pitch bend from -2 to +2 semitones
    currentPitchBend = ((newPitchWheelValue - 8192.0f) / 8192.0f) * 2.0f;
}

void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue)
{
    if (controllerNumber == 1) // Mod Wheel
    {
        modWheelValue = newControllerValue / 127.0f;
    }
}

void SynthVoice::prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = outputChannels;

    osc1.prepare(sampleRate);
    osc2.prepare(sampleRate);
    subOsc.prepare(sampleRate);
    
    filter.prepare(spec);
    
    ampEnv.setSampleRate(sampleRate);
    filterEnv.setSampleRate(sampleRate);
    modEnv.setSampleRate(sampleRate);
    
    lfo1.prepare(sampleRate);
    lfo2.prepare(sampleRate);
    
    isPrepared = true;
}

void SynthVoice::updateBaseParameters(
    float o1Semi, float o1Detune, float o1Level, float o1Pwm, PolyBLEPOscillator::Waveform o1Wave,
    float o2Semi, float o2Detune, float o2Level, float o2Pwm, PolyBLEPOscillator::Waveform o2Wave,
    float subLevel, float subOct, PolyBLEPOscillator::Waveform subWave,
    float noiseLvl,
    SynthFilter::Type fType, float fCutoff, float fRes, float fDrive, float fEnvAmt, float fKeyTrack,
    const DAHDSREnvelope::Parameters& ampParams,
    const DAHDSREnvelope::Parameters& filtParams,
    const DAHDSREnvelope::Parameters& modParams,
    float lfo1Rate, LFO::Waveform lfo1Wave, float lfo1Depth,
    float lfo2Rate, LFO::Waveform lfo2Wave, float lfo2Depth,
    const std::array<ModRoute, 8>& modRoutes
)
{
    baseO1Semi = o1Semi; baseO1Detune = o1Detune; baseO1Level = o1Level; baseO1Pwm = o1Pwm; osc1.setWaveform(o1Wave);
    baseO2Semi = o2Semi; baseO2Detune = o2Detune; baseO2Level = o2Level; baseO2Pwm = o2Pwm; osc2.setWaveform(o2Wave);
    baseSubLevel = subLevel; baseSubOct = subOct; subOsc.setWaveform(subWave);
    baseNoiseLvl = noiseLvl;

    filter.setType(fType);
    baseFCutoff = fCutoff; baseFRes = fRes; baseFDrive = fDrive; baseFEnvAmt = fEnvAmt; baseFKeyTrack = fKeyTrack;

    ampEnv.setParameters(ampParams);
    filterEnv.setParameters(filtParams);
    modEnv.setParameters(modParams);

    lfo1.setFrequency(lfo1Rate); lfo1.setWaveform(lfo1Wave); baseLfo1Depth = lfo1Depth;
    lfo2.setFrequency(lfo2Rate); lfo2.setWaveform(lfo2Wave); baseLfo2Depth = lfo2Depth;

    routes = modRoutes;
}

void SynthVoice::calculateModulation(float& o1PitchMod, float& o2PitchMod, float& o1PwmMod, float& o2PwmMod, float& fCutoffMod, float& fResMod)
{
    // Generate mod sources once per block/sub-block
    float l1 = lfo1.getNextSample() * baseLfo1Depth;
    float l2 = lfo2.getNextSample() * baseLfo2Depth;
    float eF = filterEnv.getNextSample();
    float eM = modEnv.getNextSample();

    for (const auto& route : routes)
    {
        if (route.source == ModSource::None || route.destination == ModDestination::None || route.amount == 0.0f)
            continue;

        float srcVal = 0.0f;
        switch (route.source)
        {
            case ModSource::LFO1: srcVal = l1; break;
            case ModSource::LFO2: srcVal = l2; break;
            case ModSource::EnvFilter: srcVal = eF; break;
            case ModSource::EnvMod: srcVal = eM; break;
            case ModSource::Velocity: srcVal = currentVelocity; break;
            case ModSource::KeyTrack: srcVal = (currentMidiNote - 60.0f) / 60.0f; break; // -1 to 1
            case ModSource::PitchBend: srcVal = currentPitchBend; break;
            case ModSource::ModWheel: srcVal = modWheelValue; break;
            default: break;
        }

        float mod = srcVal * route.amount;
        switch (route.destination)
        {
            case ModDestination::Osc1Pitch: o1PitchMod += mod; break;
            case ModDestination::Osc2Pitch: o2PitchMod += mod; break;
            case ModDestination::Osc1PWM: o1PwmMod += mod; break;
            case ModDestination::Osc2PWM: o2PwmMod += mod; break;
            case ModDestination::FilterCutoff: fCutoffMod += mod; break;
            case ModDestination::FilterRes: fResMod += mod; break;
            default: break;
        }
    }
    
    // Inherent Pitch bend and keytrack
    o1PitchMod += currentPitchBend;
    o2PitchMod += currentPitchBend;
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPrepared) return;

    // Fast processing loop (block rate modulation to save CPU)
    // As per critic's suggestion, we will evaluate mod matrix every 16 samples
    const int SUB_BLOCK_SIZE = 16;
    int samplesProcessed = 0;

    juce::AudioBuffer<float> subBlockBuffer;
    subBlockBuffer.setSize(1, SUB_BLOCK_SIZE, false, false, true);

    while (samplesProcessed < numSamples)
    {
        int processThisTime = juce::jmin(SUB_BLOCK_SIZE, numSamples - samplesProcessed);

        // --- SUB-BLOCK MODULATION CALCULATION ---
        float o1PitchMod = 0.0f, o2PitchMod = 0.0f, o1PwmMod = 0.0f, o2PwmMod = 0.0f, fCutoffMod = 0.0f, fResMod = 0.0f;
        calculateModulation(o1PitchMod, o2PitchMod, o1PwmMod, o2PwmMod, fCutoffMod, fResMod);

        // Apply pitch mods (in semitones)
        float o1Midi = currentMidiNote + baseO1Semi + (baseO1Detune / 100.0f) + (o1PitchMod * 24.0f); // mod amount 1.0 = 2 octaves
        float o2Midi = currentMidiNote + baseO2Semi + (baseO2Detune / 100.0f) + (o2PitchMod * 24.0f);
        float subMidi = currentMidiNote + (baseSubOct * 12.0f) + (o1PitchMod * 24.0f); // sub tracks osc1 pitch bend

        osc1.setFrequency(juce::MidiMessage::getMidiNoteInHertz(o1Midi));
        osc2.setFrequency(juce::MidiMessage::getMidiNoteInHertz(o2Midi));
        subOsc.setFrequency(juce::MidiMessage::getMidiNoteInHertz(subMidi));

        osc1.setPWM(baseO1Pwm + o1PwmMod);
        osc2.setPWM(baseO2Pwm + o2PwmMod);
        subOsc.setPWM(0.5f); // sub is usually perfect square

        // Filter Cutoff (exponential frequency mapping)
        // Base is in Hz, we modulate in semitones (octaves)
        // Plus Filter Env amount
        float eF = filterEnv.getNextSample(); // get latest for the block
        float totalCutoffMod = fCutoffMod * 60.0f; // 1.0 = 5 octaves
        totalCutoffMod += (eF * baseFEnvAmt * 60.0f);
        totalCutoffMod += ((currentMidiNote - 60.0f) * baseFKeyTrack); // Keytracking in semitones

        float finalCutoff = baseFCutoff * std::pow(2.0f, totalCutoffMod / 12.0f);
        filter.setCutoff(finalCutoff);
        filter.setResonance(juce::jlimit(0.0f, 10.0f, baseFRes + fResMod * 10.0f));
        filter.setDrive(baseFDrive);

        // --- SAMPLE RENDERING ---
        float ampArray[16] = {0.0f};

        for (int i = 0; i < processThisTime; ++i)
        {
            ampArray[i] = ampEnv.getNextSample() * currentVelocity;
            if (ampEnv.isActive() == false)
            {
                clearCurrentNote();
                return;
            }
            
            // Mix oscillators
            float s1 = osc1.getNextSample() * baseO1Level;
            float s2 = osc2.getNextSample() * baseO2Level;
            float sub = subOsc.getNextSample() * baseSubLevel;
            
            float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * baseNoiseLvl;

            float mixed = s1 + s2 + sub + noise;
            
            subBlockBuffer.setSample(0, i, mixed);
        }

        // --- FILTERING ---
        juce::dsp::AudioBlock<float> block(subBlockBuffer);
        juce::dsp::AudioBlock<float> subBlock(block.getSubBlock(0, processThisTime));
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        
        filter.process(context);

        // --- AMP AND PAN ---
        auto* writePtr = subBlockBuffer.getWritePointer(0);
        for (int i = 0; i < processThisTime; ++i)
        {
            writePtr[i] *= ampArray[i];
        }

        // Copy to output buffer (assuming stereo output)
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            auto* outPtr = outputBuffer.getWritePointer(channel, startSample + samplesProcessed);
            for (int i = 0; i < processThisTime; ++i)
            {
                outPtr[i] += writePtr[i] * 0.15f; // apply master level or amp later
            }
        }

        samplesProcessed += processThisTime;
    }
}
