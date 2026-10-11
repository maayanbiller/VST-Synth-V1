#include "SynthVoice.h"

SynthVoice::SynthVoice() {}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels)
{
    currentSampleRate = sampleRate;
    isPrepared = true;

    voiceBuffer.setSize(std::max(2, outputChannels), std::max(32, samplesPerBlock));

    osc1.prepare(sampleRate);
    osc2.prepare(sampleRate);
    osc3.prepare(sampleRate);
    osc4.prepare(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = outputChannels;
    filter.prepare(spec);

    ampEnv.setSampleRate(sampleRate); filterEnv.setSampleRate(sampleRate); modEnv.setSampleRate(sampleRate);
    lfo1.prepare(sampleRate); lfo2.prepare(sampleRate);

    currentVelocity.reset(sampleRate, 0.01);
    currentPitchBend.reset(sampleRate, 0.01);
    
    baseO1Level.reset(sampleRate, 0.01); baseO2Level.reset(sampleRate, 0.01); baseO3Level.reset(sampleRate, 0.01); baseO4Level.reset(sampleRate, 0.01);
    baseO1Pwm.reset(sampleRate, 0.01); baseO2Pwm.reset(sampleRate, 0.01); baseO3Pwm.reset(sampleRate, 0.01); baseO4Pwm.reset(sampleRate, 0.01);
    
    baseFCutoff.reset(sampleRate, 0.01);
    baseFRes.reset(sampleRate, 0.01);
    baseFDrive.reset(sampleRate, 0.01);

    currentMidiPitch.reset(sampleRate, 0.0);
    currentMidiPitch.setCurrentAndTargetValue(60.0f);
    lastMidiNote = -1.0f;
}

void SynthVoice::updateBaseParameters(const GlobalSynthParams& p)
{
    currentParams = p;
    
    baseO1Octave = p.oscOct[0]; baseO1Detune = p.oscDetune[0]; osc1.setWaveform(static_cast<PolyBLEPOscillator::Waveform>(p.oscWave[0]));
    baseO2Octave = p.oscOct[1]; baseO2Detune = p.oscDetune[1]; osc2.setWaveform(static_cast<PolyBLEPOscillator::Waveform>(p.oscWave[1]));
    baseO3Octave = p.oscOct[2]; baseO3Detune = p.oscDetune[2]; osc3.setWaveform(static_cast<PolyBLEPOscillator::Waveform>(p.oscWave[2]));
    baseO4Octave = p.oscOct[3]; baseO4Detune = p.oscDetune[3]; osc4.setWaveform(static_cast<PolyBLEPOscillator::Waveform>(p.oscWave[3]));
    
    osc1.setVoices(p.oscVoices[0]); osc1.setWidth(p.oscWidth[0]);
    osc2.setVoices(p.oscVoices[1]); osc2.setWidth(p.oscWidth[1]);
    osc3.setVoices(p.oscVoices[2]); osc3.setWidth(p.oscWidth[2]);
    osc4.setVoices(p.oscVoices[3]); osc4.setWidth(p.oscWidth[3]);
    
    filter.setType(static_cast<SynthFilter::Type>(p.filterType));
    baseFEnvAmt = p.filterEnvAmt;
    baseFKeyTrack = p.filterKeyTrack;

    if (!ampEnv.isActive())
    {
        baseO1Level.setCurrentAndTargetValue(p.oscLevel[0]);
        baseO2Level.setCurrentAndTargetValue(p.oscLevel[1]);
        baseO3Level.setCurrentAndTargetValue(p.oscLevel[2]);
        baseO4Level.setCurrentAndTargetValue(p.oscLevel[3]);

        baseO1Pwm.setCurrentAndTargetValue(p.oscPwm[0]);
        baseO2Pwm.setCurrentAndTargetValue(p.oscPwm[1]);
        baseO3Pwm.setCurrentAndTargetValue(p.oscPwm[2]);
        baseO4Pwm.setCurrentAndTargetValue(p.oscPwm[3]);

        baseFCutoff.setCurrentAndTargetValue(p.filterCutoff);
        baseFRes.setCurrentAndTargetValue(p.filterRes);
        baseFDrive.setCurrentAndTargetValue(p.filterDrive);
    }
    else
    {
        baseO1Level.setTargetValue(p.oscLevel[0]);
        baseO2Level.setTargetValue(p.oscLevel[1]);
        baseO3Level.setTargetValue(p.oscLevel[2]);
        baseO4Level.setTargetValue(p.oscLevel[3]);

        baseO1Pwm.setTargetValue(p.oscPwm[0]);
        baseO2Pwm.setTargetValue(p.oscPwm[1]);
        baseO3Pwm.setTargetValue(p.oscPwm[2]);
        baseO4Pwm.setTargetValue(p.oscPwm[3]);

        baseFCutoff.setTargetValue(p.filterCutoff);
        baseFRes.setTargetValue(p.filterRes);
        baseFDrive.setTargetValue(p.filterDrive);
    }
    
    ampEnv.setParameters(p.ampParams); filterEnv.setParameters(p.filtParams); modEnv.setParameters(p.modParams);
    
    lfo1.setFrequency(p.lfoRate[0]); lfo1.setWaveform(static_cast<LFO::Waveform>(p.lfoWave[0])); baseLfo1Depth = p.lfoDepth[0];
    lfo2.setFrequency(p.lfoRate[1]); lfo2.setWaveform(static_cast<LFO::Waveform>(p.lfoWave[1])); baseLfo2Depth = p.lfoDepth[1];
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    juce::ignoreUnused(sound);
    currentMidiNote = midiNoteNumber;
    currentVelocity.setCurrentAndTargetValue(velocity);
    
    float pbVal = ((float)currentPitchWheelPosition / 8192.0f) - 1.0f;
    currentPitchBend.setCurrentAndTargetValue(pbVal);

    float targetNote = (float)midiNoteNumber;
    if (currentParams.playMode != 0 && currentParams.glideTime > 0.001f && lastMidiNote >= 0.0f)
    {
        currentMidiPitch.setCurrentAndTargetValue(lastMidiNote);
        currentMidiPitch.reset(currentSampleRate, currentParams.glideTime);
        currentMidiPitch.setTargetValue(targetNote);
    }
    else
    {
        currentMidiPitch.reset(currentSampleRate, 0.0);
        currentMidiPitch.setCurrentAndTargetValue(targetNote);
    }
    lastMidiNote = targetNote;

    osc1.randomizePhase(); osc2.randomizePhase(); osc3.randomizePhase(); osc4.randomizePhase();
    noiseZ1 = 0.0f; noiseZ2 = 0.0f;
    filter.reset();
    ampEnv.noteOn();
    filterEnv.noteOn();
    modEnv.noteOn();
}

void SynthVoice::retargetNote (int newMidiNote, float newVelocity, bool retriggerEnvelopes)
{
    currentMidiNote = newMidiNote;
    currentVelocity.setTargetValue(newVelocity);

    float targetNote = (float)newMidiNote;
    if (currentParams.glideTime > 0.001f)
    {
        float currentPitch = currentMidiPitch.getCurrentValue();
        currentMidiPitch.setCurrentAndTargetValue(currentPitch);
        currentMidiPitch.reset(currentSampleRate, currentParams.glideTime);
        currentMidiPitch.setTargetValue(targetNote);
    }
    else
    {
        currentMidiPitch.reset(currentSampleRate, 0.0);
        currentMidiPitch.setCurrentAndTargetValue(targetNote);
    }
    lastMidiNote = targetNote;

    if (retriggerEnvelopes)
    {
        ampEnv.noteOn();
        filterEnv.noteOn();
        modEnv.noteOn();
    }
}

bool SynthVoice::isVoiceActive() const
{
    return ampEnv.isActive();
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
    float pbVal = ((float)newPitchWheelValue / 8192.0f) - 1.0f;
    currentPitchBend.setTargetValue(pbVal);
}

void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue)
{
    if (controllerNumber == 1) // Mod Wheel
        modWheelValue = (float)newControllerValue / 127.0f;
}

void SynthVoice::calculateModulation(float& o1PitchMod, float& o2PitchMod, float& o3PitchMod, float& o4PitchMod, float& o1PwmMod, float& o2PwmMod, float& o3PwmMod, float& o4PwmMod, float& fCutoffMod, float& fResMod)
{
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!ampEnv.isActive())
        return;
        
    while (numSamples > 0)
    {
        int processThisTime = std::min(numSamples, 32);

        float o1PitchMod = 0.0f, o2PitchMod = 0.0f, o3PitchMod = 0.0f, o4PitchMod = 0.0f;
        float fCutoffMod = 0.0f, fResMod = 0.0f, fDriveMod = 0.0f;
        float o1PwmMod = 0.0f, o2PwmMod = 0.0f, o3PwmMod = 0.0f, o4PwmMod = 0.0f;
        float ampMod = 0.0f;

        float lfo1Val = lfo1.getNextSample() * baseLfo1Depth;
        float lfo2Val = lfo2.getNextSample() * baseLfo2Depth;
        float modEnvVal = modEnv.getNextSample();
        float velVal = currentVelocity.getCurrentValue();

        float srcVals[5] = {lfo1Val, lfo2Val, modEnvVal, velVal, modWheelValue};

        auto applyMod = [&](int dst, float amount) {
            if (dst == 0) { o1PitchMod += amount; o2PitchMod += amount; o3PitchMod += amount; o4PitchMod += amount; }
            else if (dst == 1) { fCutoffMod += amount; }
            else if (dst == 2) { fResMod += amount; }
            else if (dst == 3) { fDriveMod += amount; }
            else if (dst == 4) { o1PwmMod += amount; o2PwmMod += amount; o3PwmMod += amount; o4PwmMod += amount; }
            else if (dst == 5) { ampMod += amount; }
            else if (dst == 6) { o1PitchMod += amount; }
            else if (dst == 7) { o2PitchMod += amount; }
        };

        // 5x8 Matrix routing
        for (int r = 0; r < 5; ++r)
        {
            float src = srcVals[r];
            if (std::abs(src) < 0.00001f) continue;

            for (int c = 0; c < 8; ++c)
            {
                int cell = currentParams.modMatrix[r][c];
                if (cell == 0) continue; // Off

                float amt = 0.0f;
                if (cell == 1) // Blue: direct fixed connection
                {
                    amt = 1.0f;
                }
                else if (cell >= 2 && cell <= 5) // Amber, Green, Red, Purple bus amounts
                {
                    amt = currentParams.modBusAmts[cell - 2];
                }

                if (amt != 0.0f)
                {
                    applyMod(c, src * amt);
                }
            }
        }

        // Legacy 4-slot routing fallback
        for (int i = 0; i < 4; ++i) {
            int src = currentParams.modSrc[i];
            int dst = currentParams.modDst[i];
            float amt = currentParams.modAmts[i];
            if (src >= 0 && src < 5 && amt != 0.0f) {
                applyMod(dst, srcVals[src] * amt);
            }
        }

        float pitch = currentMidiPitch.getNextValue();
        if (processThisTime > 1)
            currentMidiPitch.skip (processThisTime - 1);

        float pb = currentPitchBend.getNextValue();
        if (processThisTime > 1)
            currentPitchBend.skip (processThisTime - 1);

        o1PitchMod += pb; o2PitchMod += pb; o3PitchMod += pb; o4PitchMod += pb;

        float envMod = filterEnv.getNextSample() * baseFEnvAmt;
        float keyTrackMod = (pitch - 60.0f) * baseFKeyTrack;
        float finalCutoff = baseFCutoff.getNextValue() + envMod + (fCutoffMod * 5000.0f) + (keyTrackMod * 100.0f);
        float finalRes = baseFRes.getNextValue() + (fResMod * 5.0f);
        float finalDrive = baseFDrive.getNextValue() + (fDriveMod * 5.0f);
        if (processThisTime > 1)
        {
            baseFCutoff.skip (processThisTime - 1);
            baseFRes.skip (processThisTime - 1);
            baseFDrive.skip (processThisTime - 1);
        }
        
        filter.setCutoff(finalCutoff);
        filter.setResonance(finalRes);
        filter.setDrive(finalDrive);

        auto getHz = [](float midiNote) { return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f); };

        // Pitch bend is +/-2 semitones = +/-200 cents; pitch mod is up to +/-24 semitones = +/-2400 cents
        float pbAndMod1 = (pb * 200.0f) + (o1PitchMod * 2400.0f);
        float pbAndMod2 = (pb * 200.0f) + (o2PitchMod * 2400.0f);
        float pbAndMod3 = (pb * 200.0f) + (o3PitchMod * 2400.0f);
        float pbAndMod4 = (pb * 200.0f) + (o4PitchMod * 2400.0f);

        osc1.setFrequencyAndDetune(getHz(pitch + baseO1Octave * 12.0f), baseO1Detune + pbAndMod1);
        osc2.setFrequencyAndDetune(getHz(pitch + baseO2Octave * 12.0f), baseO2Detune + pbAndMod2);
        osc3.setFrequencyAndDetune(getHz(pitch + baseO3Octave * 12.0f), baseO3Detune + pbAndMod3);
        osc4.setFrequencyAndDetune(getHz(pitch + baseO4Octave * 12.0f), baseO4Detune + pbAndMod4);

        osc1.setPWM(baseO1Pwm.getNextValue() + o1PwmMod);
        osc2.setPWM(baseO2Pwm.getNextValue() + o2PwmMod);
        osc3.setPWM(baseO3Pwm.getNextValue() + o3PwmMod);
        osc4.setPWM(baseO4Pwm.getNextValue() + o4PwmMod);

        int numChannels = outputBuffer.getNumChannels();
        if (voiceBuffer.getNumChannels() < numChannels || voiceBuffer.getNumSamples() < 32)
            voiceBuffer.setSize (numChannels, 32, false, false, true);

        voiceBuffer.clear (0, processThisTime);

        for (int step = 0; step < processThisTime; ++step)
        {
            float s1L = 0.0f, s1R = 0.0f;
            float s2L = 0.0f, s2R = 0.0f;
            float s3L = 0.0f, s3R = 0.0f;
            float s4L = 0.0f, s4R = 0.0f;

            osc1.getNextStereoSample(s1L, s1R);
            osc2.getNextStereoSample(s2L, s2R);
            osc3.getNextStereoSample(s3L, s3R);
            osc4.getNextStereoSample(s4L, s4R);

            float l1 = baseO1Level.getNextValue();
            float l2 = baseO2Level.getNextValue();
            float l3 = baseO3Level.getNextValue();
            float l4 = baseO4Level.getNextValue();

            float mixL = (s1L * l1) + (s2L * l2) + (s3L * l3) + (s4L * l4);
            float mixR = (s1R * l1) + (s2R * l2) + (s3R * l3) + (s4R * l4);

            float rawNoise = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f);
            float shapedNoise = rawNoise;
            float nColor = currentParams.noiseColor;

            if (nColor < -0.05f)
            {
                // Brown / Red lowpass noise (cutoff sweeps from 150 Hz to 8 kHz)
                float norm = (nColor + 1.0f) / 0.95f;
                float cutoff = 150.0f * std::pow(50.0f, norm);
                float alpha = std::clamp(2.0f * juce::MathConstants<float>::pi * cutoff / (float)currentSampleRate, 0.001f, 0.95f);
                noiseZ1 += alpha * (rawNoise - noiseZ1);
                noiseZ2 += alpha * (noiseZ1 - noiseZ2);
                shapedNoise = noiseZ2 * 2.5f;
            }
            else if (nColor > 0.05f)
            {
                // Blue / Violet highpass noise (cutoff sweeps from 500 Hz to 6 kHz)
                float norm = (nColor - 0.05f) / 0.95f;
                float cutoff = 500.0f * std::pow(12.0f, norm);
                float alpha = std::clamp(2.0f * juce::MathConstants<float>::pi * cutoff / (float)currentSampleRate, 0.001f, 0.95f);
                noiseZ1 += alpha * (rawNoise - noiseZ1);
                shapedNoise = (rawNoise - noiseZ1) * 1.6f;
            }

            float noise = shapedNoise * currentParams.noiseLevel;
            mixL += noise;
            mixR += noise;

            float currentAmpEnv = ampEnv.getNextSample();
            float finalAmp = std::clamp(currentAmpEnv + ampMod, 0.0f, 1.0f) * 0.25f;
            
            voiceBuffer.setSample(0, step, mixL * finalAmp);
            if (numChannels > 1)
                voiceBuffer.setSample(1, step, mixR * finalAmp);
        }
        
        juce::dsp::AudioBlock<float> audioBlock (voiceBuffer.getArrayOfWritePointers(), numChannels, 0, processThisTime);
        juce::dsp::ProcessContextReplacing<float> context (audioBlock);
        filter.process(context);
        
        for (int channel = 0; channel < numChannels; ++channel)
            outputBuffer.addFrom (channel, startSample, voiceBuffer, channel, 0, processThisTime);
        
        // Ensure voice terminates properly
        if (ampEnv.isActive() == false)
        {
            clearCurrentNote();
        }

        startSample += processThisTime;
        numSamples -= processThisTime;
    }
}
