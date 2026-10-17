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

    voiceBuffer.setSize(std::max(2, outputChannels), std::max(512, samplesPerBlock));

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
    latestParamsPtr = &currentParams;

    // If voice is idle, defer heavy parameter/smoothed-value updates until startNote() wakes it up
    if (!ampEnv.isActive())
        return;

    syncVoiceParameters(currentParams, false);
}

void SynthVoice::syncVoiceParameters(const GlobalSynthParams& p, bool snapSmoothedValues)
{
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

    if (snapSmoothedValues)
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

    bool wasIdle = !ampEnv.isActive();
    if (latestParamsPtr != nullptr)
        syncVoiceParameters(*latestParamsPtr, wasIdle);

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

    if (wasIdle)
    {
        osc1.randomizePhase(); osc2.randomizePhase(); osc3.randomizePhase(); osc4.randomizePhase();
        noiseZ1 = 0.0f; noiseZ2 = 0.0f;
        filter.reset();
    }
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

        // Sample LFOs and Envelopes at the start of the 32-sample slice, then advance accurately
        float lfo1Val = lfo1.getNextSample() * baseLfo1Depth;
        float lfo2Val = lfo2.getNextSample() * baseLfo2Depth;
        float modEnvVal = modEnv.getNextSample();
        float filterEnvVal = filterEnv.getNextSample();

        if (processThisTime > 1)
        {
            lfo1.advance(processThisTime - 1);
            lfo2.advance(processThisTime - 1);
            for (int s = 1; s < processThisTime; ++s)
            {
                modEnv.getNextSample();
                filterEnv.getNextSample();
            }
        }

        float velVal = currentVelocity.getCurrentValue();
        float srcVals[5] = {lfo1Val, lfo2Val, modEnvVal, velVal, modWheelValue};

        auto applyMod = [&](int dst, float amount) noexcept {
            switch (dst)
            {
                case 0: o1PitchMod += amount; o2PitchMod += amount; o3PitchMod += amount; o4PitchMod += amount; break;
                case 1: fCutoffMod += amount; break;
                case 2: fResMod += amount; break;
                case 3: fDriveMod += amount; break;
                case 4: o1PwmMod += amount; o2PwmMod += amount; o3PwmMod += amount; o4PwmMod += amount; break;
                case 5: ampMod += amount; break;
                case 6: o1PitchMod += amount; break;
                case 7: o2PitchMod += amount; break;
            }
        };

        // Evaluate pre-parsed active modulation routes (parsed once per block in PluginProcessor)
        const int numRoutes = currentParams.numActiveModRoutes;
        for (int i = 0; i < numRoutes; ++i)
        {
            const auto& r = currentParams.activeModRoutes[i];
            float src = srcVals[r.src];
            if (std::abs(src) > 0.00001f)
                applyMod(r.dst, src * r.amount);
        }

        float pitch = currentMidiPitch.getNextValue();
        if (processThisTime > 1)
            currentMidiPitch.skip (processThisTime - 1);

        float pb = currentPitchBend.getNextValue();
        if (processThisTime > 1)
            currentPitchBend.skip (processThisTime - 1);

        o1PitchMod += pb; o2PitchMod += pb; o3PitchMod += pb; o4PitchMod += pb;

        float envMod = filterEnvVal * baseFEnvAmt;
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

        auto getHz = [](float midiNote) noexcept { return 440.0f * std::pow(2.0f, (midiNote - 69.0f) * (1.0f / 12.0f)); };

        // Pitch bend is +/-2 semitones = +/-200 cents; pitch mod is up to +/-24 semitones = +/-2400 cents
        float pbAndMod1 = (pb * 200.0f) + (o1PitchMod * 2400.0f);
        float pbAndMod2 = (pb * 200.0f) + (o2PitchMod * 2400.0f);
        float pbAndMod3 = (pb * 200.0f) + (o3PitchMod * 2400.0f);
        float pbAndMod4 = (pb * 200.0f) + (o4PitchMod * 2400.0f);

        int numChannels = outputBuffer.getNumChannels();
        if (voiceBuffer.getNumChannels() < numChannels || voiceBuffer.getNumSamples() < processThisTime)
            voiceBuffer.setSize (std::max(2, numChannels), std::max(512, processThisTime), false, false, true);

        voiceBuffer.clear (0, processThisTime);

        float* bufL = voiceBuffer.getWritePointer(0);
        float* bufR = (numChannels > 1) ? voiceBuffer.getWritePointer(1) : bufL;

        // Per-sample smoothed level arrays for contiguous SIMD block rendering
        float l1Arr[32], l2Arr[32], l3Arr[32], l4Arr[32];
        bool active1 = false, active2 = false, active3 = false, active4 = false;

        for (int step = 0; step < processThisTime; ++step)
        {
            l1Arr[step] = baseO1Level.getNextValue();
            if (l1Arr[step] > 0.0001f) active1 = true;

            l2Arr[step] = baseO2Level.getNextValue();
            if (l2Arr[step] > 0.0001f) active2 = true;

            l3Arr[step] = baseO3Level.getNextValue();
            if (l3Arr[step] > 0.0001f) active3 = true;

            l4Arr[step] = baseO4Level.getNextValue();
            if (l4Arr[step] > 0.0001f) active4 = true;
        }

        if (active1)
        {
            osc1.setFrequencyAndDetune(getHz(pitch + baseO1Octave * 12.0f), baseO1Detune + pbAndMod1);
            osc1.setPWM(baseO1Pwm.getNextValue() + o1PwmMod);
            if (processThisTime > 1) baseO1Pwm.skip(processThisTime - 1);
            osc1.renderAddBlock(bufL, bufR, l1Arr, processThisTime);
        }
        else
        {
            baseO1Pwm.skip(processThisTime);
        }

        if (active2)
        {
            osc2.setFrequencyAndDetune(getHz(pitch + baseO2Octave * 12.0f), baseO2Detune + pbAndMod2);
            osc2.setPWM(baseO2Pwm.getNextValue() + o2PwmMod);
            if (processThisTime > 1) baseO2Pwm.skip(processThisTime - 1);
            osc2.renderAddBlock(bufL, bufR, l2Arr, processThisTime);
        }
        else
        {
            baseO2Pwm.skip(processThisTime);
        }

        if (active3)
        {
            osc3.setFrequencyAndDetune(getHz(pitch + baseO3Octave * 12.0f), baseO3Detune + pbAndMod3);
            osc3.setPWM(baseO3Pwm.getNextValue() + o3PwmMod);
            if (processThisTime > 1) baseO3Pwm.skip(processThisTime - 1);
            osc3.renderAddBlock(bufL, bufR, l3Arr, processThisTime);
        }
        else
        {
            baseO3Pwm.skip(processThisTime);
        }

        if (active4)
        {
            osc4.setFrequencyAndDetune(getHz(pitch + baseO4Octave * 12.0f), baseO4Detune + pbAndMod4);
            osc4.setPWM(baseO4Pwm.getNextValue() + o4PwmMod);
            if (processThisTime > 1) baseO4Pwm.skip(processThisTime - 1);
            osc4.renderAddBlock(bufL, bufR, l4Arr, processThisTime);
        }
        else
        {
            baseO4Pwm.skip(processThisTime);
        }

        // If mono output buffer, halve bufL since renderAddBlock added both L and R to bufL
        if (numChannels == 1)
        {
            for (int step = 0; step < processThisTime; ++step)
                bufL[step] *= 0.5f;
        }

        // Noise generation (only evaluated if noiseLevel > 0)
        if (currentParams.noiseLevel > 0.0001f)
        {
            float nColor = currentParams.noiseColor;
            float alpha = 0.0f;
            bool isRed = (nColor < -0.05f);
            bool isBlue = (nColor > 0.05f);

            if (isRed)
            {
                float norm = (nColor + 1.0f) / 0.95f;
                float cutoff = 150.0f * std::pow(50.0f, norm);
                alpha = std::clamp(2.0f * juce::MathConstants<float>::pi * cutoff / (float)currentSampleRate, 0.001f, 0.95f);
            }
            else if (isBlue)
            {
                float norm = (nColor - 0.05f) / 0.95f;
                float cutoff = 500.0f * std::pow(12.0f, norm);
                alpha = std::clamp(2.0f * juce::MathConstants<float>::pi * cutoff / (float)currentSampleRate, 0.001f, 0.95f);
            }

            float nLvl = currentParams.noiseLevel;
            for (int step = 0; step < processThisTime; ++step)
            {
                float rawNoise = getNextRandomFloat();
                float shapedNoise = rawNoise;

                if (isRed)
                {
                    noiseZ1 += alpha * (rawNoise - noiseZ1);
                    noiseZ2 += alpha * (noiseZ1 - noiseZ2);
                    shapedNoise = noiseZ2 * 2.5f;
                }
                else if (isBlue)
                {
                    noiseZ1 += alpha * (rawNoise - noiseZ1);
                    shapedNoise = (rawNoise - noiseZ1) * 1.6f;
                }

                float noise = shapedNoise * nLvl;
                voiceBuffer.addSample(0, step, noise);
                if (numChannels > 1)
                    voiceBuffer.addSample(1, step, noise);
            }
        }
        
        juce::dsp::AudioBlock<float> audioBlock (voiceBuffer.getArrayOfWritePointers(), numChannels, 0, processThisTime);
        juce::dsp::ProcessContextReplacing<float> context (audioBlock);
        filter.process(context);

        // Post-Filter VCA Stage:
        // Amp envelope scales the signal, and amp modulation scales the envelope multiplicatively.
        // Multiplying by currentAmpEnv guarantees:
        // 1. Attack ALWAYS starts at 0.000000f regardless of modulation settings.
        // 2. Release ALWAYS smoothly decays to 0.000000f with zero sudden step.
        // 3. Modulations (tremolo, velocity, mod wheel) naturally shape the volume.
        float ampScale = std::clamp(1.0f + ampMod, 0.0f, 2.0f);
        for (int step = 0; step < processThisTime; ++step)
        {
            float currentAmpEnv = ampEnv.getNextSample();
            float finalAmp = currentAmpEnv * ampScale * 0.25f;

            voiceBuffer.setSample(0, step, voiceBuffer.getSample(0, step) * finalAmp);
            if (numChannels > 1)
                voiceBuffer.setSample(1, step, voiceBuffer.getSample(1, step) * finalAmp);
        }
        
        for (int channel = 0; channel < numChannels; ++channel)
            outputBuffer.addFrom (channel, startSample, voiceBuffer, channel, 0, processThisTime);
        
        // Ensure voice terminates properly
        if (ampEnv.isActive() == false)
        {
            clearCurrentNote();
            break;
        }

        startSample += processThisTime;
        numSamples -= processThisTime;
    }
}
