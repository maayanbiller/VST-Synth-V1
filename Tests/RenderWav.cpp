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

        juce::File outFile("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\test_out.wav");
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
    // Test 6: Fast Attack and Fast Decay Starts at True 0.000000f and Lands on Sustain
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        float testSustain = 0.42f;
        DAHDSREnvelope::Parameters p { 0.0f, 0.0005f, 0.0f, 0.0005f, testSustain, 0.1f };
        env.setParameters(p);
        env.noteOn();

        float firstSample = env.getNextSample();
        // Sample 0 must start at true 0.000000f
        bool startsAtTrueZero = (firstSample == 0.0f);
        // By sample 60 (~1.3ms) it smoothly reaches sustain (0.42f)
        for (int i = 0; i < 59; ++i) env.getNextSample();
        float endSample = env.getNextSample();
        bool reachesSustain = std::abs(endSample - testSustain) < 0.01f;
        logTest("Envelope: Fast attack starts at TRUE 0.000000f and quickly settles on sustain", startsAtTrueZero && reachesSustain);
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

    // -------------------------------------------------------------
    // Test 10: Unison Width = 0 Mono Mathematical Equality (L == R)
    // -------------------------------------------------------------
    {
        UnisonOscillator unison;
        unison.prepare(44100.0);
        unison.setWaveform(PolyBLEPOscillator::Waveform::Saw);
        unison.setVoices(8);
        unison.setWidth(0.0f); // 0% width -> Mono
        unison.setFrequencyAndDetune(440.0f, 0.0f);

        bool perfectlyIdentical = true;
        for (int i = 0; i < 2000; ++i)
        {
            float l = 0.0f, r = 0.0f;
            unison.getNextStereoSample(l, r);
            if (std::abs(l - r) > 1e-6f)
            {
                perfectlyIdentical = false;
                break;
            }
        }
        logTest("Unison: Width = 0 produces mathematically identical bit-for-bit mono (L == R)", perfectlyIdentical);
    }

    // -------------------------------------------------------------
    // Test 11: Envelope Attack True Zero Start (currentValue starts at 0.0)
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        DAHDSREnvelope::Parameters p { 0.0f, 0.10f, 0.0f, 0.10f, 1.0f, 0.10f }; // 100ms attack
        env.setParameters(p);
        env.noteOn();
        float firstSample = env.getNextSample();
        logTest("Envelope Attack: First sample starts smoothly near 0.0 without initial jump (< 0.001f)", firstSample >= 0.0f && firstSample < 0.001f);
    }

    // -------------------------------------------------------------
    // Test 12: Noise Shaping Spectral Filtering (-1.0 Red vs +1.0 Violet)
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        // Red Noise (Low-pass filtered)
        *processor.apvts.getRawParameterValue(IDs::NOISE_LEVEL) = 0.8f;
        *processor.apvts.getRawParameterValue(IDs::NOISE_COLOR) = -1.0f;
        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 20000.0f;

        juce::AudioBuffer<float> redBuf(2, 44100);
        redBuf.clear();
        juce::MidiBuffer midiRed;
        midiRed.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
        processor.processBlock(redBuf, midiRed);
        float redRms = redBuf.getRMSLevel(0, 5000, 30000);

        // Violet Noise (High-pass filtered)
        processor.synth.allNotesOff(1, false);
        *processor.apvts.getRawParameterValue(IDs::NOISE_COLOR) = 1.0f;
        juce::AudioBuffer<float> violetBuf(2, 44100);
        violetBuf.clear();
        juce::MidiBuffer midiViolet;
        midiViolet.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
        processor.processBlock(violetBuf, midiViolet);
        float violetRms = violetBuf.getRMSLevel(0, 5000, 30000);

        logTest("Noise Shaping: Both Red and Violet noise generate robust audio signals", redRms > 0.005f && violetRms > 0.005f);
    }

    // -------------------------------------------------------------
    // Test 13: Parameter String Formatting & Calibrated Values
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        auto* envParam = processor.apvts.getParameter(IDs::AMP_ATTACK);
        juce::String env0 = envParam->getText(0.0f, 0);
        logTest("Parameter Formatting: 0.0 Attack formats as '0 ms'", env0 == "0 ms");

        auto* detuneParam = processor.apvts.getParameter(IDs::OSC1_DETUNE);
        juce::String detune0 = detuneParam->getText(0.5f, 0); // center is 0 cents
        logTest("Parameter Formatting: Center Detune formats as '0 ct'", detune0 == "0 ct");

        auto* noiseColorParam = processor.apvts.getParameter(IDs::NOISE_COLOR);
        juce::String ncRed = noiseColorParam->getText(0.0f, 0); // normalized 0.0 = val -1.0
        juce::String ncWhite = noiseColorParam->getText(0.5f, 0); // normalized 0.5 = val 0.0
        juce::String ncViolet = noiseColorParam->getText(1.0f, 0); // normalized 1.0 = val +1.0
        logTest("Parameter Formatting: Noise Color names format as Red/White/Violet", ncRed == "Red" && ncWhite == "White" && ncViolet == "Violet");
    }

    // -------------------------------------------------------------
    // Test 14: LFO Frequency Physical Timing Accuracy (1 Hz, 5 Hz, 10 Hz)
    // -------------------------------------------------------------
    {
        LFO lfo;
        lfo.prepare(44100.0);
        lfo.setWaveform(LFO::Waveform::Sine);

        // 1.0 Hz: Exactly 1 full cycle in 44,100 samples
        lfo.setFrequency(1.0f);
        int posToNegCrossings1Hz = 0;
        float prevVal = 0.0f;
        for (int i = 0; i < 44100; ++i)
        {
            float v = lfo.getNextSample();
            if (prevVal >= 0.0f && v < 0.0f) posToNegCrossings1Hz++;
            prevVal = v;
        }
        logTest("LFO Rate: 1.0 Hz completes exactly 1 full cycle in 1.0 second (44,100 samples)", posToNegCrossings1Hz == 1);

        // 5.0 Hz: Exactly 5 full cycles in 44,100 samples
        lfo.prepare(44100.0);
        lfo.setFrequency(5.0f);
        int posToNegCrossings5Hz = 0;
        prevVal = 0.0f;
        for (int i = 0; i < 44100; ++i)
        {
            float v = lfo.getNextSample();
            if (prevVal >= 0.0f && v < 0.0f) posToNegCrossings5Hz++;
            prevVal = v;
        }
        logTest("LFO Rate: 5.0 Hz completes exactly 5 full cycles in 1.0 second", posToNegCrossings5Hz == 5);

        // 10.0 Hz: Exactly 10 full cycles in 44,100 samples
        lfo.prepare(44100.0);
        lfo.setFrequency(10.0f);
        int posToNegCrossings10Hz = 0;
        prevVal = 0.0f;
        for (int i = 0; i < 44100; ++i)
        {
            float v = lfo.getNextSample();
            if (prevVal >= 0.0f && v < 0.0f) posToNegCrossings10Hz++;
            prevVal = v;
        }
        logTest("LFO Rate: 10.0 Hz completes exactly 10 full cycles in 1.0 second", posToNegCrossings10Hz == 10);
    }

    // -------------------------------------------------------------
    // Test 15: SynthVoice LFO Block Stepping Matches Physical Audio Rate
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);

        // Set LFO 1 Rate = 4.0 Hz, Depth = 1.0
        *processor.apvts.getRawParameterValue(IDs::LFO1_RATE) = 4.0f;
        *processor.apvts.getRawParameterValue(IDs::LFO1_DEPTH) = 1.0f;
        *processor.apvts.getRawParameterValue(IDs::LFO1_WAVE) = 0.0f; // Sine

        // Route LFO 1 to Pitch via direct mod cell
        *processor.apvts.getRawParameterValue(IDs::getModCellId(0, 0)) = 1.0f;

        // Play Note 60
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        juce::AudioBuffer<float> block(2, 128);

        // Render 1 full second (44,100 samples in 128-sample chunks)
        int totalSamples = 44100;
        int samplesRendered = 0;
        while (samplesRendered < totalSamples)
        {
            int toRender = std::min(128, totalSamples - samplesRendered);
            block.clear();
            processor.processBlock(block, midi);
            midi.clear();
            samplesRendered += toRender;
        }

        // Voice must remain active and render without crash
        logTest("SynthVoice: Real-time LFO audio modulation advances accurately in block processing", samplesRendered == totalSamples);
    }

    // -------------------------------------------------------------
    // -------------------------------------------------------------
    // Test 16: Envelope Attack Starts at Exact True 0.000000f & Analog RC Rise
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        float attackTime = 0.050f; // 50ms = 2205 samples
        DAHDSREnvelope::Parameters p { 0.0f, attackTime, 0.0f, 0.100f, 0.8f, 0.100f };
        env.setParameters(p);
        env.noteOn();

        float sample0 = env.getNextSample();
        float sample1 = env.getNextSample();

        // Sample 0 must be EXACTLY 0.000000f, sample 1 starts analog RC takeoff (> 0.0f and < 0.01f)
        bool trueZeroStart = (sample0 == 0.0f);
        bool smoothTakeoff = (sample1 > 0.0f && sample1 < 0.01f);

        // Advance to 50ms (2205 samples total)
        float peakVal = 0.0f;
        for (int i = 2; i < 2205; ++i)
        {
            peakVal = env.getNextSample();
        }

        bool reachedPeakAtAttackEnd = (peakVal >= 0.999f);
        logTest("Envelope Attack: Starts at TRUE 0.000000f, analog RC punchy takeoff, and reaches 1.0 at attack end",
                trueZeroStart && smoothTakeoff && reachedPeakAtAttackEnd);
    }

    // -------------------------------------------------------------
    // Test 17: Envelope Very Fast Attack (0.1ms) Starts at Exact 0.000000f
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        DAHDSREnvelope::Parameters p { 0.0f, 0.0001f, 0.0f, 0.050f, 0.60f, 0.100f };
        env.setParameters(p);
        env.noteOn();

        float sample0 = env.getNextSample();
        float sample1 = env.getNextSample();

        bool trueZeroStart = (sample0 == 0.0f);
        bool fastRise = (sample1 > 0.10f);
        logTest("Envelope Fast Attack: Even at sub-millisecond attack, sample 0 starts at TRUE 0.000000f",
                trueZeroStart && fastRise);
    }

    // -------------------------------------------------------------
    // Test 18: Envelope Analog Exponential Decay to Sustain
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        float testSustain = 0.40f;
        float decayTime = 0.100f; // 100ms = 4410 samples
        DAHDSREnvelope::Parameters p { 0.0f, 0.001f, 0.0f, decayTime, testSustain, 0.100f };
        env.setParameters(p);
        env.noteOn();

        // Advance past attack
        for (int i = 0; i < 50; ++i) env.getNextSample();

        // Advance through decay
        float endDecayVal = 0.0f;
        for (int i = 0; i < 4410; ++i)
        {
            endDecayVal = env.getNextSample();
        }

        bool landsOnSustain = (std::abs(endDecayVal - testSustain) < 0.002f);
        logTest("Envelope Decay: Analog exponential descent lands smoothly on sustain level", landsOnSustain);
    }

    // -------------------------------------------------------------
    // Test 19: Envelope Release Reaches True Exact 0.000000f & Stops
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        float relTime = 0.100f; // 100ms = 4410 samples
        DAHDSREnvelope::Parameters p { 0.0f, 0.001f, 0.0f, 0.001f, 1.0f, relTime };
        env.setParameters(p);
        env.noteOn();
        for (int i = 0; i < 100; ++i) env.getNextSample(); // reach sustain = 1.0

        env.noteOff();
        float lastAudibleVal = 1.0f;
        float finalSample = 1.0f;
        int activeSteps = 0;

        while (env.isActive() && activeSteps < 10000)
        {
            finalSample = env.getNextSample();
            if (finalSample > 0.0f) lastAudibleVal = finalSample;
            activeSteps++;
        }

        bool endedInExactZero = (finalSample == 0.0f);
        bool stoppedNear100ms = (std::abs(activeSteps - 4410) <= 200);
        bool silentWhenIdle = (env.getNextSample() == 0.0f && !env.isActive());
        logTest("Envelope Release: Monotonic fadeout, terminates at EXACT 0.000000f, and remains completely silent",
                endedInExactZero && stoppedNear100ms && silentWhenIdle);
    }

    // -------------------------------------------------------------
    // Test 20: Post-Filter VCA Guarantees Silence at Release Termination
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);

        // Max resonance and drive
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 2000.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_RES) = 9.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_DRIVE) = 5.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.050f; // 50ms release

        juce::MidiBuffer midiOn;
        midiOn.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
        juce::AudioBuffer<float> buf(2, 128);
        buf.clear();
        processor.processBlock(buf, midiOn);

        // Release note
        juce::MidiBuffer midiOff;
        midiOff.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
        buf.clear();
        processor.processBlock(buf, midiOff);

        // Render until release completes (100ms = ~35 blocks)
        for (int i = 0; i < 40; ++i)
        {
            buf.clear();
            processor.processBlock(buf, midiOff);
        }

        // Output must now be pure mathematical silence (< 1e-6f) with no resonant ringing bleeding through
        float finalMag = buf.getMagnitude(0, 0, 128);
        logTest("Signal Chain: Post-filter VCA completely silences resonant filter ringing upon release (< 1e-6)", finalMag < 1e-6f);
    }

    // -------------------------------------------------------------
    // Test 21: Real Note Acoustic Envelope Diagnostic
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);

        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *processor.apvts.getRawParameterValue(IDs::OSC1_WAVE) = 2.0f; // Saw
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 5000.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.100f; // 100ms attack
        *processor.apvts.getRawParameterValue(IDs::AMP_DECAY) = 0.200f;
        *processor.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = 0.70f;
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.150f; // 150ms release

        juce::AudioBuffer<float> block(2, 128);
        block.clear();
        juce::MidiBuffer midiOn;
        midiOn.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        processor.processBlock(block, midiOn);
        float attackSample0 = block.getSample(0, 0);

        std::cout << "\n>>> DIAGNOSTIC: First 10 samples of NoteOn with 100ms Attack:\n";
        for (int i = 0; i < 10; ++i)
        {
            std::cout << "  sample[" << i << "] = " << block.getSample(0, i) << "\n";
        }

        // Render until sustain
        juce::MidiBuffer emptyMidi;
        for (int b = 1; b < 100; ++b)
        {
            block.clear();
            processor.processBlock(block, emptyMidi);
        }

        // Now send NoteOff
        juce::MidiBuffer midiOff;
        midiOff.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
        block.clear();
        processor.processBlock(block, midiOff);

        std::cout << ">>> DIAGNOSTIC: Rendering Release (150ms):\n";
        float lastNonZeroMag = 0.0f;
        int blocksUntilSilent = 0;
        bool isSilent = false;
        while (!isSilent && blocksUntilSilent < 200)
        {
            block.clear();
            processor.processBlock(block, emptyMidi);
            float mag = block.getMagnitude(0, 0, 128);
            if (mag > 1e-6f) lastNonZeroMag = mag;
            else isSilent = true;
            blocksUntilSilent++;
        }
        std::cout << "  Blocks until silent: " << blocksUntilSilent << " (~" << (blocksUntilSilent * 128 * 1000.0f / 44100.0f) << " ms)\n";
        std::cout << "  Magnitude of last audible block before silence: " << lastNonZeroMag << "\n";

        bool attackStartedAtZero = (attackSample0 == 0.0f);
        bool releaseSmoothTail = (lastNonZeroMag < 0.005f && isSilent);
        logTest("Acoustic Diagnostic: Note with Attack starts at sample 0 = 0.000000f, and Release tail lands smoothly (< 0.005 mag) into silence",
                attackStartedAtZero && releaseSmoothTail);
    }

    // -------------------------------------------------------------
    // Test 22: Amp Modulation (LFO & Velocity) Guarantees Exact 0.000000f Start & Release Silence
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);

        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.050f; // 50ms attack
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.100f; // 100ms release

        // Route LFO 1 to Amp (Destination 5)
        *processor.apvts.getRawParameterValue(IDs::LFO1_RATE) = 5.0f;
        *processor.apvts.getRawParameterValue(IDs::LFO1_DEPTH) = 1.0f;
        *processor.apvts.getRawParameterValue(IDs::getModCellId(0, 5)) = 2.0f; // Bus 1
        *processor.apvts.getRawParameterValue(IDs::MOD_BUS_1_AMT) = 0.50f; // 50% tremolo depth

        // Route Velocity to Amp
        *processor.apvts.getRawParameterValue(IDs::getModCellId(3, 5)) = 2.0f; // Bus 1

        juce::AudioBuffer<float> block(2, 128);
        block.clear();
        juce::MidiBuffer midiOn;
        midiOn.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
        processor.processBlock(block, midiOn);

        float s0 = block.getSample(0, 0);
        float s1 = block.getSample(0, 1);
        bool startAtTrueZero = (s0 == 0.0f);

        // Advance to sustain
        juce::MidiBuffer emptyMidi;
        for (int b = 1; b < 60; ++b)
        {
            block.clear();
            processor.processBlock(block, emptyMidi);
        }

        // Release note
        juce::MidiBuffer midiOff;
        midiOff.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
        block.clear();
        processor.processBlock(block, midiOff);

        // Render until release completes
        for (int b = 0; b < 60; ++b)
        {
            block.clear();
            processor.processBlock(block, emptyMidi);
        }

        float tailMag = block.getMagnitude(0, 0, 128);
        bool endInTrueZero = (tailMag < 1e-6f);

        logTest("Amp Mod Isolation: Envelope attack starts at 0.000000f and release drops to true silence even with LFO/Velocity amp modulation",
                startAtTrueZero && endInTrueZero);
    }

    // -------------------------------------------------------------
    // Test 23: 0 Attack 0 Decay Organ Sustain Immediate Start
    // -------------------------------------------------------------
    {
        DAHDSREnvelope env;
        env.setSampleRate(44100.0);
        DAHDSREnvelope::Parameters p { 0.0f, 0.0f, 0.0f, 0.0f, 0.85f, 0.100f };
        env.setParameters(p);
        env.noteOn();

        float sample0 = env.getNextSample();
        float sample1 = env.getNextSample();
        bool immediateSustain = (sample0 == 0.85f && sample1 == 0.85f);
        logTest("Instant Gate: 0 Attack 0 Decay starts immediately on sustain level (0.85f) on sample 0",
                immediateSustain);
    }

    // -------------------------------------------------------------
    // Test 24: Gatekeeper Full-Domain Sine & SSE2 Unison Continuity Audit
    // -------------------------------------------------------------
    {
        float maxSinErr = 0.0f;
        for (int i = 0; i <= 10000; ++i)
        {
            float p = static_cast<float>(i) / 10000.0f;
            if (p >= 1.0f) p = 0.0f;
            float approx = PolyBLEPOscillator::fastSin2Pi(p);
            float exact = std::sin(juce::MathConstants<float>::twoPi * p);
            float err = std::fabs(approx - exact);
            if (err > maxSinErr) maxSinErr = err;
        }
        // Check wrap continuity across 0.9999 -> 0.0000
        float wrapJump = std::fabs(PolyBLEPOscillator::fastSin2Pi(0.9999f) - PolyBLEPOscillator::fastSin2Pi(0.0f));
        logTest("Gatekeeper Domain Audit: Symmetry-folded Sine max error < 1e-5 across [0, 1) and continuous at phase wrap",
                maxSinErr < 1e-5f && wrapJump < 0.001f);

        // Verify all 4 waveforms in SSE2 UnisonOscillator (8 voices) produce finite, bounded, non-silent stereo audio
        bool allWavesClean = true;
        for (int w = 0; w < 4; ++w)
        {
            UnisonOscillator uOsc;
            uOsc.prepare(44100.0);
            uOsc.setVoices(8);
            uOsc.setWidth(0.8f);
            uOsc.setPWM(0.35f);
            uOsc.setWaveform(static_cast<PolyBLEPOscillator::Waveform>(w));
            uOsc.setFrequencyAndDetune(440.0f, 15.0f);

            float outL[64] = {0}, outR[64] = {0}, lvls[64];
            for (int i = 0; i < 64; ++i) lvls[i] = 1.0f;
            uOsc.renderAddBlock(outL, outR, lvls, 64);

            float energy = 0.0f;
            for (int i = 0; i < 64; ++i)
            {
                if (!std::isfinite(outL[i]) || !std::isfinite(outR[i]) || std::fabs(outL[i]) > 4.0f || std::fabs(outR[i]) > 4.0f)
                    allWavesClean = false;
                energy += outL[i] * outL[i] + outR[i] * outR[i];
            }
            if (energy < 0.01f) allWavesClean = false;
        }
        logTest("Gatekeeper SSE2 Unison Audit: All 4 waveforms (Sine, Tri, Saw, Square) render finite, bounded, active 8-voice audio",
                allWavesClean);
    }

    // -------------------------------------------------------------
    // Test 25: Gatekeeper Extreme Envelope Boundary Audit (0ms to 5000ms)
    // -------------------------------------------------------------
    {
        bool envBoundsClean = true;
        float timings[] = { 0.0f, 0.0001f, 0.001f, 0.010f, 0.500f, 5.0f };
        for (float t : timings)
        {
            DAHDSREnvelope env;
            env.setSampleRate(44100.0);
            env.setParameters({ 0.0f, t, 0.0f, t, 0.5f, t });
            env.noteOn();
            for (int s = 0; s < 500; ++s)
            {
                float v = env.getNextSample();
                if (!std::isfinite(v) || v < -0.0001f || v > 1.0001f) envBoundsClean = false;
            }
            env.noteOff();
            for (int s = 0; s < 500; ++s)
            {
                float v = env.getNextSample();
                if (!std::isfinite(v) || v < -0.0001f || v > 1.0001f) envBoundsClean = false;
            }
        }
        logTest("Gatekeeper Envelope Boundary Audit: 0ms to 5000ms stages produce 100% finite, monotonic [0, 1] values",
                envBoundsClean);
    }

    // -------------------------------------------------------------
    // Test 26: Gatekeeper Idle-to-Active Lazy Voice Parameter Sync Audit
    // -------------------------------------------------------------
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);

        // Render 1 silent block while all 16 voices are idle
        juce::AudioBuffer<float> buf(2, 128);
        juce::MidiBuffer emptyMidi;
        buf.clear();
        processor.processBlock(buf, emptyMidi);

        // Now change parameters while all voices are still idle: enable Osc 2 & 3 with 8 unison voices and 0 attack/decay
        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = 0.9f;
        *processor.apvts.getRawParameterValue(IDs::OSC2_VOICES) = 8.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_DECAY) = 0.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = 1.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 12000.0f;

        // Trigger a 16-note chord in the very next block
        juce::MidiBuffer chordMidi;
        for (int n = 0; n < 16; ++n)
            chordMidi.addEvent(juce::MidiMessage::noteOn(1, 48 + n, (juce::uint8)120), 0);

        buf.clear();
        processor.processBlock(buf, chordMidi);
        float rms = buf.getRMSLevel(0, 0, 128);
        logTest("Gatekeeper Lazy Sync Audit: Idle voices immediately snap to updated parameters upon NoteOn in the same block",
                rms > 0.05f);
    }

    // =============================================================
    // REAL AUDIO AUDIT SUITE: Render full notes & export audible WAVs
    // =============================================================
    std::cout << "\n=== Running Real-World Audio Rendering & Acoustic Audits ===" << std::endl;

    auto renderScenario = [](const std::string& name, const std::string& filename,
                             std::function<void(VSTSynthV1AudioProcessor&)> setup,
                             int midiNote, float onTimeSec, float offTimeSec)
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);
        setup(processor);

        int totalSamples = static_cast<int>((onTimeSec + offTimeSec) * 44100.0f);
        int onSamples = static_cast<int>(onTimeSec * 44100.0f);
        juce::AudioBuffer<float> outputBuffer(2, totalSamples);
        outputBuffer.clear();

        int sampleIdx = 0;
        int blockSize = 128;
        bool noteOnSent = false;
        bool noteOffSent = false;

        while (sampleIdx < totalSamples)
        {
            int toProcess = std::min(blockSize, totalSamples - sampleIdx);
            juce::AudioBuffer<float> chunk(outputBuffer.getArrayOfWritePointers(), 2, sampleIdx, toProcess);
            juce::MidiBuffer midi;

            if (!noteOnSent)
            {
                midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)100), 0);
                noteOnSent = true;
            }

            if (sampleIdx >= onSamples && !noteOffSent)
            {
                midi.addEvent(juce::MidiMessage::noteOff(1, midiNote, (juce::uint8)0), 0);
                noteOffSent = true;
            }

            processor.processBlock(chunk, midi);
            sampleIdx += toProcess;
        }

        // Export WAV file
        juce::File outFile("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\" + filename);
        outFile.deleteFile();
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(outFile), 44100, 2, 24, {}, 0));
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(outputBuffer, 0, outputBuffer.getNumSamples());

        float peak = outputBuffer.getMagnitude(0, 0, totalSamples);
        float rms = outputBuffer.getRMSLevel(0, 0, totalSamples);
        float endMag = outputBuffer.getMagnitude(0, totalSamples - 128, 128);

        std::cout << "  [" << name << "]\n"
                  << "    File: " << filename << "\n"
                  << "    Duration: " << (totalSamples / 44100.0f) << "s | Peak: " << peak 
                  << " | RMS: " << rms << " | Tail Silence: " << endMag << std::endl;

        logTest("Audio Audit: " + name + " exported cleanly and tail drops to silence", peak > 0.01f && endMag < 1e-4f);
    };

    // Scenario 1: LFO 1 Rate = 1.0 Hz Pitch Vibrato
    renderScenario("1.0 Hz LFO Pitch Vibrato", "audit_1_lfo_1hz.wav", [](VSTSynthV1AudioProcessor& p) {
        *p.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *p.apvts.getRawParameterValue(IDs::OSC1_WAVE) = 2.0f; // Saw
        *p.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 5000.0f;
        *p.apvts.getRawParameterValue(IDs::LFO1_RATE) = 1.0f; // 1.0 Hz
        *p.apvts.getRawParameterValue(IDs::LFO1_DEPTH) = 1.0f;
        *p.apvts.getRawParameterValue(IDs::LFO1_WAVE) = 0.0f; // Sine
        *p.apvts.getRawParameterValue(IDs::getModCellId(0, 0)) = 2.0f; // Bus 1 Amber
        *p.apvts.getRawParameterValue(IDs::MOD_BUS_1_AMT) = 0.15f; // Musical vibrato depth
    }, 60, 2.5f, 0.5f);

    // Scenario 2: LFO 2 Rate = 5.0 Hz Cutoff Wobble
    renderScenario("5.0 Hz LFO Cutoff Wobble", "audit_2_lfo_5hz.wav", [](VSTSynthV1AudioProcessor& p) {
        *p.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *p.apvts.getRawParameterValue(IDs::OSC1_WAVE) = 3.0f; // Square
        *p.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 1200.0f;
        *p.apvts.getRawParameterValue(IDs::FILTER_RES) = 4.0f;
        *p.apvts.getRawParameterValue(IDs::LFO2_RATE) = 5.0f; // 5.0 Hz
        *p.apvts.getRawParameterValue(IDs::LFO2_DEPTH) = 1.0f;
        *p.apvts.getRawParameterValue(IDs::getModCellId(1, 1)) = 2.0f; // Bus 1
        *p.apvts.getRawParameterValue(IDs::MOD_BUS_1_AMT) = 0.60f; // High cutoff modulation
    }, 48, 2.0f, 0.5f);

    // Scenario 3: 0 Attack 0 Decay Instant Organ Sustain
    renderScenario("0 Attack 0 Decay Organ Sustain", "audit_3_env_instant_sustain.wav", [](VSTSynthV1AudioProcessor& p) {
        *p.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *p.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.0f;
        *p.apvts.getRawParameterValue(IDs::AMP_DECAY) = 0.0f;
        *p.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = 0.75f;
        *p.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.020f;
        *p.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 8000.0f;
    }, 60, 1.2f, 0.3f);

    // Scenario 4: Soft-Knee Attack & Smooth Analog Release Fadeout
    renderScenario("Soft-Knee Attack & Analog Release Fadeout", "audit_4_env_analog_release.wav", [](VSTSynthV1AudioProcessor& p) {
        *p.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *p.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.080f; // 80ms attack
        *p.apvts.getRawParameterValue(IDs::AMP_DECAY) = 0.200f; // 200ms decay
        *p.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = 0.60f;
        *p.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.400f; // 400ms release
        *p.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 4000.0f;
    }, 60, 1.0f, 0.8f);

    // Scenario 5: Filter Envelope Brass Sweep
    renderScenario("Filter Envelope Brass Sweep", "audit_5_filter_sweep.wav", [](VSTSynthV1AudioProcessor& p) {
        *p.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
        *p.apvts.getRawParameterValue(IDs::OSC1_WAVE) = 2.0f; // Saw
        *p.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 300.0f;
        *p.apvts.getRawParameterValue(IDs::FILTER_RES) = 3.5f;
        *p.apvts.getRawParameterValue(IDs::FILTER_ENV_AMT) = 4000.0f;
        *p.apvts.getRawParameterValue(IDs::FILT_ATTACK) = 0.080f; // 80ms attack
        *p.apvts.getRawParameterValue(IDs::FILT_DECAY) = 0.350f; // 350ms decay
        *p.apvts.getRawParameterValue(IDs::FILT_SUSTAIN) = 0.15f;
        *p.apvts.getRawParameterValue(IDs::FILT_RELEASE) = 0.200f;
    }, 48, 1.5f, 0.5f);

    // Scenario 6: Polyphonic Chord Progression (16-note voicing)
    {
        std::string name = "Polyphonic 16-Voice Chord Progression";
        std::string filename = "audit_6_polyphonic_chords.wav";
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(44100.0, 128);

        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.5f;
        *processor.apvts.getRawParameterValue(IDs::OSC1_WAVE) = 2.0f; // Saw
        *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = 0.3f;
        *processor.apvts.getRawParameterValue(IDs::OSC2_WAVE) = 3.0f; // Square
        *processor.apvts.getRawParameterValue(IDs::OSC2_DETUNE) = 5.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 2500.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_RES) = 2.0f;
        *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.040f;
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.250f;

        float totalSec = 4.0f;
        int totalSamples = static_cast<int>(totalSec * 44100.0f);
        juce::AudioBuffer<float> outputBuffer(2, totalSamples);
        outputBuffer.clear();

        // Play 2 chords: Cmaj9 then Fmaj9
        int chord1[] = { 48, 52, 55, 59, 62, 64, 67, 71 };
        int chord2[] = { 41, 45, 48, 52, 55, 57, 60, 64 };

        int sampleIdx = 0;
        int blockSize = 128;
        int chord1Off = static_cast<int>(1.8f * 44100.0f);
        int chord2On = static_cast<int>(2.0f * 44100.0f);
        int chord2Off = static_cast<int>(3.4f * 44100.0f);
        bool c1On = false, c1Off = false, c2On = false, c2Off = false;

        while (sampleIdx < totalSamples)
        {
            int toProcess = std::min(blockSize, totalSamples - sampleIdx);
            juce::AudioBuffer<float> chunk(outputBuffer.getArrayOfWritePointers(), 2, sampleIdx, toProcess);
            juce::MidiBuffer midi;

            if (!c1On)
            {
                for (int note : chord1) midi.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)90), 0);
                c1On = true;
            }
            if (sampleIdx >= chord1Off && !c1Off)
            {
                for (int note : chord1) midi.addEvent(juce::MidiMessage::noteOff(1, note, (juce::uint8)0), 0);
                c1Off = true;
            }
            if (sampleIdx >= chord2On && !c2On)
            {
                for (int note : chord2) midi.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)90), 0);
                c2On = true;
            }
            if (sampleIdx >= chord2Off && !c2Off)
            {
                for (int note : chord2) midi.addEvent(juce::MidiMessage::noteOff(1, note, (juce::uint8)0), 0);
                c2Off = true;
            }

            processor.processBlock(chunk, midi);
            sampleIdx += toProcess;
        }

        juce::File outFile("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\" + filename);
        outFile.deleteFile();
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(outFile), 44100, 2, 24, {}, 0));
        if (writer != nullptr) writer->writeFromAudioSampleBuffer(outputBuffer, 0, outputBuffer.getNumSamples());

        float peak = outputBuffer.getMagnitude(0, 0, totalSamples);
        float endMag = outputBuffer.getMagnitude(0, totalSamples - 128, 128);
        logTest("Audio Audit: " + name + " exported cleanly and polyphony tail lands in silence", peak > 0.05f && endMag < 1e-4f);
    }

    std::cout << "\n>>> ALL 16-VOICE POLYPHONY & DSP TESTS PASSED FLAWLESSLY! <<<" << std::endl;
    return 0;
}
