#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>
#include <cmath>

// ==============================================================================
// StressReproductionTest: Real-Time Diagnostic Test Harness
// Built to reproduce and isolate the intermittent ~30 Hz audio stutter.
// ==============================================================================

struct AnomalyStats
{
    int deadlineViolations = 0;
    int maxBlockTimeMicroseconds = 0;
    int stutterBurst30HzDetected = 0;
    int nanOrInfDetected = 0;
    int unexpectedSilenceGaps = 0;
    int prematureVoiceKills = 0;
    int maxActiveVoicesObserved = 0;
};

// Check a block of audio for the exact ~30 Hz burst pattern (3ms audio + ~29ms silence)
class StutterPatternDetector
{
public:
    StutterPatternDetector(double sampleRate) : sr(sampleRate) {}

    void processBuffer(const juce::AudioBuffer<float>& buffer, bool isNoteActive)
    {
        if (!isNoteActive)
        {
            activeSamplesInRun = 0;
            silentSamplesInRun = 0;
            return;
        }

        const float* left = buffer.getReadPointer(0);
        int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            float mag = std::abs(left[i]);
            if (mag > 0.015f)
            {
                if (silentSamplesInRun > 0)
                {
                    float silenceMs = (float)silentSamplesInRun * 1000.0f / (float)sr;
                    float activeMs = (float)activeSamplesInRun * 1000.0f / (float)sr;

                    // If we had active audio for 1-6 ms, followed by 25-35 ms of silence, that matches 30 Hz stutter!
                    if (activeMs >= 1.0f && activeMs <= 8.0f && silenceMs >= 24.0f && silenceMs <= 35.0f)
                    {
                        consecutiveStutters++;
                        if (consecutiveStutters >= 2)
                        {
                            detected30HzCount++;
                        }
                    }
                    else if (silenceMs > 15.0f)
                    {
                        // Some other significant silence gap during an active note
                        consecutiveStutters = 0;
                        generalSilenceGapCount++;
                    }
                    else
                    {
                        consecutiveStutters = 0;
                    }
                    activeSamplesInRun = 0;
                    silentSamplesInRun = 0;
                }
                activeSamplesInRun++;
            }
            else
            {
                silentSamplesInRun++;
            }
        }
    }

    int get30HzStutterCount() const { return detected30HzCount; }
    int getGeneralSilenceGapCount() const { return generalSilenceGapCount; }

private:
    double sr;
    int activeSamplesInRun = 0;
    int silentSamplesInRun = 0;
    int consecutiveStutters = 0;
    int detected30HzCount = 0;
    int generalSilenceGapCount = 0;
};

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=================================================================\n";
    std::cout << "Starting Deep Stress & Reproduction Test for VST Synth V1\n";
    std::cout << "=================================================================\n\n";

    AnomalyStats stats;
    const double sampleRate = 44100.0;

    // --------------------------------------------------------------------------
    // Test Scenario 1: CPU Deadline & Unison Stress Profiling (Variable Buffers)
    // Testing if 16-voice polyphony with Unison active exceeds the hardware buffer budget!
    // --------------------------------------------------------------------------
    std::cout << "--- [SCENARIO 1] CPU Execution Time vs Buffer Duration Profiling ---\n";
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(sampleRate, 512);

        // Configure realistic rich patch: OSC 1 & OSC 2 active with Unison (Voices = 4 to 8, Width = 50%)
        *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.7f;
        *processor.apvts.getRawParameterValue(IDs::OSC1_VOICES) = 4; // 4 unison voices
        *processor.apvts.getRawParameterValue(IDs::OSC1_WIDTH) = 0.5f;

        *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = 0.5f;
        *processor.apvts.getRawParameterValue(IDs::OSC2_VOICES) = 4; // 4 unison voices
        *processor.apvts.getRawParameterValue(IDs::OSC2_WIDTH) = 0.5f;

        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 2500.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_RES) = 3.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_DRIVE) = 2.0f;

        // Long sustain
        *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = 0.05f;
        *processor.apvts.getRawParameterValue(IDs::AMP_DECAY) = 0.10f;
        *processor.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = 0.8f;
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 0.5f;

        // Play 16 notes (saturating all 16 voices)
        juce::MidiBuffer midiStart;
        for (int n = 0; n < 16; ++n)
            midiStart.addEvent(juce::MidiMessage::noteOn(1, 40 + n * 2, (juce::uint8)100), 0);

        // Variable block sizes: 64, 128, 256, 441 (WASAPI shared mode 10ms!), 512
        int testBlockSizes[] = { 64, 128, 256, 441, 512 };

        for (int blockSize : testBlockSizes)
        {
            float bufferBudgetMs = (float)blockSize * 1000.0f / (float)sampleRate;
            juce::AudioBuffer<float> buffer(2, blockSize);
            
            // First block triggers notes
            buffer.clear();
            processor.processBlock(buffer, midiStart);
            midiStart.clear();

            int deadlineMissesForThisSize = 0;
            double maxDurationUs = 0.0;
            double avgDurationUs = 0.0;
            int totalBlocksTested = 100; // ~1-2 seconds of rendering

            juce::MidiBuffer emptyMidi;
            for (int b = 0; b < totalBlocksTested; ++b)
            {
                buffer.clear();
                auto t0 = std::chrono::high_resolution_clock::now();
                processor.processBlock(buffer, emptyMidi);
                auto t1 = std::chrono::high_resolution_clock::now();

                double durUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
                avgDurationUs += durUs;
                if (durUs > maxDurationUs) maxDurationUs = durUs;

                double budgetUs = bufferBudgetMs * 1000.0;
                if (durUs > budgetUs)
                {
                    deadlineMissesForThisSize++;
                    stats.deadlineViolations++;
                }

                // Check for NaN or Inf
                for (int ch = 0; ch < 2; ++ch)
                {
                    for (int s = 0; s < blockSize; ++s)
                    {
                        float val = buffer.getSample(ch, s);
                        if (!std::isfinite(val)) stats.nanOrInfDetected++;
                    }
                }
            }
            avgDurationUs /= totalBlocksTested;

            std::cout << "  BlockSize: " << blockSize << " samples (" << bufferBudgetMs << " ms budget) -> "
                      << "Avg: " << (avgDurationUs / 1000.0) << " ms (" << (avgDurationUs / (bufferBudgetMs * 1000.0) * 100.0) << "% CPU), "
                      << "Peak: " << (maxDurationUs / 1000.0) << " ms, "
                      << "Deadline Misses: " << deadlineMissesForThisSize << "\n";
        }
    }

    // --------------------------------------------------------------------------
    // Test Scenario 2: Long-Duration Performance Stress with Chaotic MIDI Fuzzing
    // (Simulating 30 seconds of non-stop rapid playing, voice stealing, pitch bends)
    // --------------------------------------------------------------------------
    std::cout << "\n--- [SCENARIO 2] Long-Duration Chaotic MIDI Stress (30 seconds) ---\n";
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(sampleRate, 128); // 128 samples is ~2.9ms

        StutterPatternDetector detector(sampleRate);
        std::mt19937 rng(42); // deterministic seed
        std::uniform_int_distribution<int> noteDist(36, 84);
        std::uniform_int_distribution<int> velDist(40, 127);
        std::uniform_int_distribution<int> durationDist(50, 800); // 50ms to 800ms
        std::uniform_int_distribution<int> blockSizeDist(32, 256);

        struct ActiveNote {
            int note;
            int remainingSamples;
        };
        std::vector<ActiveNote> activeNotes;

        int totalBlocks = 10000; // ~30 seconds of audio
        int samplesRendered = 0;

        for (int b = 0; b < totalBlocks; ++b)
        {
            int blockSize = 128; // standard low latency buffer
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();
            juce::MidiBuffer midi;

            // Trigger new note randomly every ~150-300ms
            if (rng() % 30 == 0)
            {
                int newNote = noteDist(rng);
                int vel = velDist(rng);
                int durSamples = static_cast<int>((durationDist(rng) * sampleRate) / 1000.0);
                activeNotes.push_back({ newNote, durSamples });
                midi.addEvent(juce::MidiMessage::noteOn(1, newNote, (juce::uint8)vel), rng() % blockSize);
            }

            // Decrement active notes and send noteOff
            for (auto it = activeNotes.begin(); it != activeNotes.end();)
            {
                it->remainingSamples -= blockSize;
                if (it->remainingSamples <= 0)
                {
                    midi.addEvent(juce::MidiMessage::noteOff(1, it->note, (juce::uint8)0), rng() % blockSize);
                    it = activeNotes.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            processor.processBlock(buffer, midi);
            detector.processBuffer(buffer, !activeNotes.empty());

            // Count active voices in synth
            int activeVoices = 0;
            for (int v = 0; v < processor.synth.getNumVoices(); ++v)
                if (processor.synth.getVoice(v)->isVoiceActive()) activeVoices++;
            if (activeVoices > stats.maxActiveVoicesObserved) stats.maxActiveVoicesObserved = activeVoices;

            samplesRendered += blockSize;
        }

        std::cout << "  Rendered " << (samplesRendered / sampleRate) << " seconds of chaotic performance.\n";
        std::cout << "  Max active voices: " << stats.maxActiveVoicesObserved << "\n";
        std::cout << "  30 Hz Stutter Patterns Detected: " << detector.get30HzStutterCount() << "\n";
        std::cout << "  General Silence Gaps (> 15ms): " << detector.getGeneralSilenceGapCount() << "\n";
    }

    // --------------------------------------------------------------------------
    // Test Scenario 3: Monophonic / Legato Rapid Trills & Retrigger Edge Cases
    // (Testing rapid 1ms - 5ms note alternations with mode switching)
    // --------------------------------------------------------------------------
    std::cout << "\n--- [SCENARIO 3] Rapid Legato/Mono Trill & Retrigger Boundary Test ---\n";
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(sampleRate, 128);

        for (int mode = 1; mode <= 2; ++mode) // 1=Mono, 2=Legato
        {
            processor.synth.setPlayMode(mode);
            *processor.apvts.getRawParameterValue(IDs::PLAY_MODE) = (float)mode;
            *processor.apvts.getRawParameterValue(IDs::GLIDE_TIME) = 0.05f; // 50ms glide

            juce::AudioBuffer<float> buffer(2, 128);
            int trillViolations = 0;

            // Play Note 60, then rapidly alternate Note 62 every 2-5ms (100-200 samples)
            for (int rep = 0; rep < 500; ++rep)
            {
                juce::MidiBuffer midiOn;
                midiOn.addEvent(juce::MidiMessage::noteOn(1, (rep % 2 == 0) ? 60 : 62, (juce::uint8)100), 0);
                buffer.clear();
                processor.processBlock(buffer, midiOn);

                // Check if voice died prematurely (magnitude is zero right after NoteOn)
                float mag = buffer.getMagnitude(0, 0, 128);
                if (mag == 0.0f)
                {
                    trillViolations++;
                }

                // Short release of one note
                if (rep % 3 == 0)
                {
                    juce::MidiBuffer midiOff;
                    midiOff.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
                    buffer.clear();
                    processor.processBlock(buffer, midiOff);
                }
            }

            std::cout << "  Mode " << (mode == 1 ? "Mono" : "Legato") 
                      << " Trill: Premature silence drops: " << trillViolations << "\n";
        }
    }

    // --------------------------------------------------------------------------
    // Test Scenario 4: Concurrent UI / Parameter Automation Thread Simulation
    // (Simulating the 30 Hz UI thread repaints and APVTS adjustments during audio rendering)
    // --------------------------------------------------------------------------
    std::cout << "\n--- [SCENARIO 4] Concurrent 30 Hz UI / Parameter Thread Contention ---\n";
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(sampleRate, 128);

        std::atomic<bool> isRunning { true };
        std::atomic<int> uiIterations { 0 };

        // Play sustained chord
        juce::MidiBuffer chordMidi;
        for (int n = 0; n < 6; ++n)
            chordMidi.addEvent(juce::MidiMessage::noteOn(1, 48 + n * 4, (juce::uint8)100), 0);
        juce::AudioBuffer<float> initBuf(2, 128);
        processor.processBlock(initBuf, chordMidi);

        // Spawn simulated UI thread running at 30 Hz (every 33ms)
        std::thread uiThread([&]() {
            std::mt19937 uiRng(123);
            std::uniform_real_distribution<float> cutDist(200.0f, 8000.0f);
            std::uniform_real_distribution<float> resDist(0.5f, 8.0f);

            while (isRunning.load())
            {
                // Simulate APVTS parameter changes and LookAndFeel / state query
                if (auto* cut = processor.apvts.getParameter(IDs::FILTER_CUTOFF))
                    cut->setValueNotifyingHost(uiRng() % 1000 / 1000.0f);
                if (auto* res = processor.apvts.getParameter(IDs::FILTER_RES))
                    res->setValueNotifyingHost(uiRng() % 1000 / 1000.0f);

                uiIterations++;
                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30 Hz refresh
            }
        });

        // Run audio thread for 3 seconds while UI thread is firing
        juce::AudioBuffer<float> audioBuf(2, 128);
        juce::MidiBuffer emptyMidi;
        int blocksWithSpike = 0;
        double maxTimeUs = 0.0;

        for (int i = 0; i < 1000; ++i)
        {
            audioBuf.clear();
            auto t0 = std::chrono::high_resolution_clock::now();
            processor.processBlock(audioBuf, emptyMidi);
            auto t1 = std::chrono::high_resolution_clock::now();

            double durUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (durUs > maxTimeUs) maxTimeUs = durUs;
            // 128 samples budget at 44.1kHz is 2902 microseconds
            if (durUs > 2902.0)
            {
                blocksWithSpike++;
            }
        }

        isRunning.store(false);
        if (uiThread.joinable()) uiThread.join();

        std::cout << "  UI thread completed " << uiIterations.load() << " cycles at 30 Hz.\n";
        std::cout << "  Peak Audio Callback Time: " << (maxTimeUs / 1000.0) << " ms\n";
        std::cout << "  Audio Blocks exceeding 128-sample budget (> 2.9ms): " << blocksWithSpike << " / 1000\n";
    }

    // --------------------------------------------------------------------------
    // Test Scenario 5: DC Blocker 5 Hz Pole Time-Constant & Relaxation Oscillation
    // (Testing if the DC blocker can latch into a 31.4 Hz periodic state)
    // --------------------------------------------------------------------------
    std::cout << "\n--- [SCENARIO 5] DC Blocker 5 Hz Pole & Nonlinear Threshold Test ---\n";
    {
        // Standalone DC blocker test
        struct DCBlockerTest
        {
            float x1 = 0.0f;
            float y1 = 0.0f;
            float R = 0.9993f;

            void prepare(double sr)
            {
                R = 1.0f - (2.0f * juce::MathConstants<float>::pi * 5.0f / static_cast<float>(sr));
            }

            float processSample(float x)
            {
                float y = x - x1 + R * y1;
                x1 = x;
                if (std::abs(y) < 1e-8f || !std::isfinite(y))
                {
                    y = 0.0f;
                    y1 = 0.0f;
                    x1 = 0.0f;
                }
                else
                {
                    y1 = y;
                }
                return y;
            }
        } dc;

        dc.prepare(sampleRate);

        // Inject sudden step discontinuity (e.g. Note onset or unison DC offset)
        int numSamples = 44100;
        std::vector<float> output(numSamples);

        // Step of 0.5 at sample 100, held for 10000 samples, then released
        for (int i = 0; i < numSamples; ++i)
        {
            float in = (i >= 100 && i < 10000) ? 0.5f : 0.0f;
            output[i] = dc.processSample(in);
        }

        // Check if output exhibits oscillations or stable decay
        bool hasOscillation = false;
        int zeroCrossings = 0;
        for (int i = 1000; i < 9000; ++i)
        {
            if ((output[i-1] >= 0.0f && output[i] < 0.0f) || (output[i-1] <= 0.0f && output[i] > 0.0f))
                zeroCrossings++;
        }

        std::cout << "  DC Blocker step response zero crossings during hold: " << zeroCrossings 
                  << " (Expect 0 for smooth exponential decay)\n";
    }

    std::cout << "\n=================================================================\n";
    std::cout << "Summary of Stress Test Findings:\n";
    std::cout << "  - Total Deadline Violations: " << stats.deadlineViolations << "\n";
    std::cout << "  - NaN / Inf Anomalies: " << stats.nanOrInfDetected << "\n";
    std::cout << "=================================================================\n";

    return 0;
}
