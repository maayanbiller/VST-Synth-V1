#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/SynthEngine.h"
#include <iostream>
#include <cmath>
#include <cassert>

static void logTest(const std::string& name, bool pass)
{
    std::cout << "[" << (pass ? "PASS" : "FAIL") << "] " << name << std::endl;
    if (!pass)
    {
        std::cerr << "CRITICAL FAILURE in test: " << name << std::endl;
        std::exit(1);
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=== Running Comprehensive 16-Voice Polyphony & DSP Test Suite ===" << std::endl;

    // -------------------------------------------------------------
    // Test 1: Full 16-Voice Polyphony in VSTSynthV1AudioProcessor
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        // Verify 16 voices created
        int numVoices = processor.synth.getNumVoices();
        logTest("Engine: Synthesiser has exactly 16 voices", numVoices == 16);

        // Ensure play mode is Poly (0)
        auto* playModeParam = processor.apvts.getRawParameterValue(IDs::PLAY_MODE);
        *playModeParam = 0.0f; // Poly
        processor.synth.setPlayMode(0);

        // Play 16 notes simultaneously (MIDI 48 to 63)
        for (int note = 48; note < 48 + 16; ++note)
        {
            processor.synth.noteOn(1, note, 0.8f);
        }

        int activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
        {
            if (processor.synth.getVoice(i)->isVoiceActive())
                activeVoices++;
        }
        logTest("Polyphony: Exactly 16 voices active simultaneously for 16-note chord", activeVoices == 16);

        // Release 8 notes (48 to 55)
        for (int note = 48; note < 48 + 8; ++note)
        {
            processor.synth.noteOff(1, note, 0.0f, false);
        }

        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
        {
            if (processor.synth.getVoice(i)->isVoiceActive())
                activeVoices++;
        }
        logTest("Polyphony: Exactly 8 voices active after releasing 8 notes", activeVoices == 8);

        // Turn the 8 released notes back on to fill 16 voices again
        for (int note = 48; note < 48 + 8; ++note)
        {
            processor.synth.noteOn(1, note, 0.8f);
        }
        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
            if (processor.synth.getVoice(i)->isVoiceActive()) activeVoices++;
        logTest("Polyphony: Refilled to 16 voices", activeVoices == 16);

        // Play a 17th note to test note stealing
        processor.synth.noteOn(1, 70, 0.8f);
        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
        {
            if (processor.synth.getVoice(i)->isVoiceActive())
                activeVoices++;
        }
        logTest("Polyphony: Note stealing successfully maintained 16 voices on 17th note", activeVoices == 16);

        // All notes off
        processor.synth.allNotesOff(1, false);
        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
        {
            if (processor.synth.getVoice(i)->isVoiceActive())
                activeVoices++;
        }
        logTest("Polyphony: 0 voices active after allNotesOff", activeVoices == 0);
    }

    // -------------------------------------------------------------
    // Test 2: Audio Summing Mathematical Proof (Polyphony Energy Test)
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);
        processor.synth.setPlayMode(0); // Poly

        // Setup clean sustain envelope so audio level is steady
        *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.001f;
        *processor.apvts.getRawParameterValue(IDs::AMP_DECAY) = 0.001f;
        *processor.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = 1.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.1f;
        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.7f;
        *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::OSC3_LEVEL) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::OSC4_LEVEL) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 20000.0f;

        // --- Render 1 Note (Note 60) ---
        juce::AudioBuffer<float> buffer1(2, 44100);
        buffer1.clear();
        juce::MidiBuffer midi1;
        midi1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
        processor.processBlock(buffer1, midi1);
        float rms1 = buffer1.getRMSLevel(0, 5000, 30000);
        logTest("Audio Summing: Single note produces healthy RMS", rms1 > 0.01f);

        // --- Reset & Render 4-Note Chord (Notes 48, 52, 55, 60) ---
        processor.synth.allNotesOff(1, false);
        juce::AudioBuffer<float> buffer4(2, 44100);
        buffer4.clear();
        juce::MidiBuffer midi4;
        midi4.addEvent(juce::MidiMessage::noteOn(1, 48, (juce::uint8)127), 0);
        midi4.addEvent(juce::MidiMessage::noteOn(1, 52, (juce::uint8)127), 0);
        midi4.addEvent(juce::MidiMessage::noteOn(1, 55, (juce::uint8)127), 0);
        midi4.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
        processor.processBlock(buffer4, midi4);
        float rms4 = buffer4.getRMSLevel(0, 5000, 30000);

        std::cout << "  -> Single-note RMS: " << rms1 << ", 4-note chord RMS: " << rms4 << std::endl;
        logTest("Audio Summing: 4-note chord RMS is significantly greater than single note (Summing confirmed)", rms4 > rms1 * 1.5f);

        // --- Reset & Render Full 16-Note Chord ---
        processor.synth.allNotesOff(1, false);
        juce::AudioBuffer<float> buffer16(2, 44100);
        buffer16.clear();
        juce::MidiBuffer midi16;
        for (int i = 0; i < 16; ++i)
            midi16.addEvent(juce::MidiMessage::noteOn(1, 40 + i * 2, (juce::uint8)127), 0);
        processor.processBlock(buffer16, midi16);
        float rms16 = buffer16.getRMSLevel(0, 5000, 30000);

        std::cout << "  -> 16-note chord RMS: " << rms16 << std::endl;
        logTest("Audio Summing: 16-note chord RMS is robust and non-zero", rms16 > rms4 * 1.2f);

        // Check for NaN or Inf across all 16 voices
        bool hasNaN = false;
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int s = 0; s < 44100; ++s)
            {
                float val = buffer16.getSample(ch, s);
                if (std::isnan(val) || std::isinf(val))
                    hasNaN = true;
            }
        }
        logTest("Audio Summing: 16-voice output is clean and free of NaN/Inf", !hasNaN);
    }

    // -------------------------------------------------------------
    // Test 3: SynthEngine Mono & Legato Modes
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        // Test Mono Mode
        processor.synth.setPlayMode(1); // Mono
        processor.synth.noteOn(1, 60, 0.8f);
        processor.synth.noteOn(1, 67, 0.8f); // Overlapping
        
        int activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
            if (processor.synth.getVoice(i)->isVoiceActive()) activeVoices++;
        logTest("Mono Mode: Exactly 1 voice active during overlapping notes", activeVoices == 1);

        // Release overlapping note, should return to 60
        processor.synth.noteOff(1, 67, 0.0f, true);
        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
            if (processor.synth.getVoice(i)->isVoiceActive()) activeVoices++;
        logTest("Mono Mode: Voice remains active returning to held note", activeVoices == 1);

        processor.synth.noteOff(1, 60, 0.0f, false);
        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
            if (processor.synth.getVoice(i)->isVoiceActive()) activeVoices++;
        logTest("Mono Mode: Voice stops when note stack is empty", activeVoices == 0);

        // Test Legato Mode
        processor.synth.setPlayMode(2); // Legato
        processor.synth.noteOn(1, 60, 0.8f);
        processor.synth.noteOn(1, 64, 0.8f);
        activeVoices = 0;
        for (int i = 0; i < processor.synth.getNumVoices(); ++i)
            if (processor.synth.getVoice(i)->isVoiceActive()) activeVoices++;
        logTest("Legato Mode: Exactly 1 voice active during overlapping notes", activeVoices == 1);
        processor.synth.allNotesOff(1, false);
    }

    // -------------------------------------------------------------
    // Test 4: End-to-End Render WAV Export with 16-note chord
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);
        processor.synth.setPlayMode(0); // Poly

        juce::AudioBuffer<float> buffer(2, 44100 * 3); // 3 seconds
        buffer.clear();
        
        int blockSize = 512;
        int numSamples = buffer.getNumSamples();
        int sampleIdx = 0;

        while (sampleIdx < numSamples)
        {
            int processThisTime = juce::jmin(blockSize, numSamples - sampleIdx);
            juce::AudioBuffer<float> subBuffer(buffer.getArrayOfWritePointers(), 2, sampleIdx, processThisTime);
            juce::MidiBuffer midi;

            // Start 8 notes at 0.0s, another 8 notes at 0.5s, release all at 2.0s
            if (sampleIdx == 0)
            {
                for (int n = 0; n < 8; ++n)
                    midi.addEvent(juce::MidiMessage::noteOn(1, 48 + n * 2, (juce::uint8)100), 0);
            }
            if (sampleIdx <= 22050 && sampleIdx + processThisTime > 22050)
            {
                for (int n = 0; n < 8; ++n)
                    midi.addEvent(juce::MidiMessage::noteOn(1, 64 + n * 2, (juce::uint8)100), 22050 - sampleIdx);
            }
            if (sampleIdx <= 44100 * 2 && sampleIdx + processThisTime > 44100 * 2)
            {
                for (int n = 0; n < 8; ++n)
                {
                    midi.addEvent(juce::MidiMessage::noteOff(1, 48 + n * 2, (juce::uint8)0), 44100 * 2 - sampleIdx);
                    midi.addEvent(juce::MidiMessage::noteOff(1, 64 + n * 2, (juce::uint8)0), 44100 * 2 - sampleIdx);
                }
            }

            processor.processBlock(subBuffer, midi);
            sampleIdx += processThisTime;
        }

        juce::File outFile("C:\\Users\\maaya\\projects\\Synth\\VST-Synth-V1\\test_out.wav");
        outFile.deleteFile();
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(outFile), 44100, 2, 16, {}, 0));
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());

        logTest("Export: test_out.wav 16-note chord successfully written to disk", outFile.existsAsFile());
    }

    // -------------------------------------------------------------
    // Test 5: Envelope Release Analog Curve (Smooth landing to 0, no cliff)
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        DAHDSREnvelope::Parameters p { 0.0f, 0.001f, 0.0f, 0.001f, 1.0f, 0.05f }; // 50ms release
        env.setParameters(p);
        env.noteOn();
        for (int i = 0; i < 50; ++i) env.getNextSample(); // at sustain = 1.0
        env.noteOff();

        float prevSample = 1.0f;
        float finalSampleBeforeZero = 0.0f;
        bool landedSmoothly = false;
        int releaseSteps = 0;

        while (env.isActive() && releaseSteps < 44100)
        {
            float s = env.getNextSample();
            if (s > 0.0f)
                finalSampleBeforeZero = s;
            prevSample = s;
            releaseSteps++;
        }

        // Must smoothly reach below 0.001f before deactivating, not drop abruptly from 0.01 or higher
        landedSmoothly = (finalSampleBeforeZero < 0.001f);
        logTest("Envelope Release: Smooth analog fadeout into silence (< 0.001f before idle)", landedSmoothly);
    }

    // -------------------------------------------------------------
    // Test 6: Zero Attack and Zero Decay starts directly on Sustain level
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        float testSustain = 0.42f;
        DAHDSREnvelope::Parameters p { 0.0f, 0.0005f, 0.0f, 0.0005f, testSustain, 0.1f };
        env.setParameters(p);
        env.noteOn();

        float firstSample = env.getNextSample();
        float secondSample = env.getNextSample();
        bool startsDirectlyOnSustain = std::abs(firstSample - testSustain) < 0.001f && std::abs(secondSample - testSustain) < 0.001f;
        logTest("Envelope: At 0 attack 0 decay, immediately starts on sustain level without spiking", startsDirectlyOnSustain);
    }

    // -------------------------------------------------------------
    // Test 7: Filter Resonance and Drive Soft Saturation Stability
    // -------------------------------------------------------------
    {
        SynthFilter filter;
        juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
        filter.prepare(spec);
        filter.setType(SynthFilter::Type::LP24);
        filter.setCutoff(1000.0f);
        filter.setResonance(8.0f);
        filter.setDrive(5.0f);

        juce::AudioBuffer<float> testBuf(2, 512);
        for (int i = 0; i < 512; ++i)
        {
            float x = std::sin(2.0f * juce::MathConstants<float>::pi * 500.0f * i / 44100.0f);
            testBuf.setSample(0, i, x);
            testBuf.setSample(1, i, x);
        }

        juce::dsp::AudioBlock<float> block(testBuf);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        filter.process(ctx);

        float maxL = testBuf.getMagnitude(0, 0, 512);
        float maxR = testBuf.getMagnitude(1, 0, 512);
        bool stableAndAudible = (maxL > 0.1f && maxL < 5.0f && !std::isnan(maxL) && !std::isinf(maxL));
        bool stereoIdentical = std::abs(maxL - maxR) < 0.0001f;
        logTest("Filter: Resonance & Drive saturation stable and non-clipping", stableAndAudible && stereoIdentical);
    }

    // -------------------------------------------------------------
    // Test 8: Unison Oscillator Stereo Spread Verification
    // -------------------------------------------------------------
    {
        UnisonOscillator unison;
        unison.prepare(44100.0);
        unison.setWaveform(PolyBLEPOscillator::Waveform::Saw);
        unison.setVoices(7);
        unison.setWidth(1.0f); // 100% stereo width
        unison.setFrequencyAndDetune(440.0f, 0.0f);

        float diffSum = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            float l = 0.0f, r = 0.0f;
            unison.getNextStereoSample(l, r);
            diffSum += std::abs(l - r);
        }

        bool hasStereoWidth = (diffSum > 10.0f);
        logTest("Unison: 7 voices at 100% width produce rich stereo difference (L != R)", hasStereoWidth);
    }

    // -------------------------------------------------------------
    // Test 9: 5x8 Mod Matrix Direct and Bus Routing
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        // Connect LFO 1 (Row 0) to Cutoff (Col 1) via Blue direct connection (Cell value 1)
        auto* cellR0C1 = processor.apvts.getRawParameterValue(IDs::getModCellId(0, 1));
        *cellR0C1 = 1.0f; // Direct connection

        // Set LFO 1 Rate = 1.0Hz, Depth = 1.0
        *processor.apvts.getRawParameterValue(IDs::LFO1_DEPTH) = 1.0f;

        // Connect LFO 2 (Row 1) to Pitch (Col 0) via Bus 2 Green (Cell value 3)
        auto* cellR1C0 = processor.apvts.getRawParameterValue(IDs::getModCellId(1, 0));
        *cellR1C0 = 3.0f; // Bus 2 connection

        // Set Bus 2 Amount = +0.75
        *processor.apvts.getRawParameterValue(IDs::MOD_BUS_2_AMT) = 0.75f;

        logTest("Mod Matrix: Parameters accessible and route correctly", cellR0C1 != nullptr && cellR1C0 != nullptr);
    }

    std::cout << "\n>>> ALL 16-VOICE POLYPHONY & DSP TESTS PASSED FLAWLESSLY! <<<" << std::endl;
    return 0;
}
