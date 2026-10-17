#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=================================================================\n";
    std::cout << "Ultra-Low Latency (3ms / 128 Samples & 64 Samples) Torture Benchmark\n";
    std::cout << "Target 1: 128 samples @ 44.1 kHz -> Hardware Budget is 2.902 ms\n";
    std::cout << "Target 2:  64 samples @ 44.1 kHz -> Hardware Budget is 1.451 ms\n";
    std::cout << "=================================================================\n\n";

    const double sampleRate = 44100.0;

    auto runTorture = [&](int blockSize, const std::string& bufferDesc, bool enableFullModMatrix, bool mixedAllWaveforms = false)
    {
        double budgetMs = (static_cast<double>(blockSize) / sampleRate) * 1000.0;
        std::cout << "--- Testing " << bufferDesc << " (BlockSize: " << blockSize 
                  << " samples, Hardware Deadline: " << budgetMs << " ms) ---\n";

        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(sampleRate, blockSize);

        // Turn all 4 oscillators ON with 8 Unison Voices
        for (int o = 0; o < 4; ++o)
        {
            if (o == 0) { 
                *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f; 
                *processor.apvts.getRawParameterValue(IDs::OSC1_VOICES) = 8.0f; 
                *processor.apvts.getRawParameterValue(IDs::OSC1_WIDTH) = 0.8f; 
                *processor.apvts.getRawParameterValue(IDs::OSC1_WAVE) = mixedAllWaveforms ? 0.0f : 3.0f; // 0=Sine or 3=Square
                *processor.apvts.getRawParameterValue(IDs::OSC1_PWM) = 0.35f;
            }
            if (o == 1) { 
                *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = 0.7f; 
                *processor.apvts.getRawParameterValue(IDs::OSC2_VOICES) = 8.0f; 
                *processor.apvts.getRawParameterValue(IDs::OSC2_WIDTH) = 0.8f; 
                *processor.apvts.getRawParameterValue(IDs::OSC2_WAVE) = mixedAllWaveforms ? 1.0f : 3.0f; // 1=Triangle or 3=Square
                *processor.apvts.getRawParameterValue(IDs::OSC2_PWM) = 0.45f;
            }
            if (o == 2) { 
                *processor.apvts.getRawParameterValue(IDs::OSC3_LEVEL) = 0.7f; 
                *processor.apvts.getRawParameterValue(IDs::OSC3_VOICES) = 8.0f; 
                *processor.apvts.getRawParameterValue(IDs::OSC3_WIDTH) = 0.8f; 
                *processor.apvts.getRawParameterValue(IDs::OSC3_WAVE) = mixedAllWaveforms ? 2.0f : 3.0f; // 2=Saw or 3=Square
                *processor.apvts.getRawParameterValue(IDs::OSC3_PWM) = 0.25f;
            }
            if (o == 3) { 
                *processor.apvts.getRawParameterValue(IDs::OSC4_LEVEL) = 0.7f; 
                *processor.apvts.getRawParameterValue(IDs::OSC4_VOICES) = 8.0f; 
                *processor.apvts.getRawParameterValue(IDs::OSC4_WIDTH) = 0.8f; 
                *processor.apvts.getRawParameterValue(IDs::OSC4_WAVE) = 3.0f; // 3=Square+PWM
                *processor.apvts.getRawParameterValue(IDs::OSC4_PWM) = 0.60f;
            }
        }

        // Noise with color filter
        *processor.apvts.getRawParameterValue(IDs::NOISE_LEVEL) = 0.3f;
        *processor.apvts.getRawParameterValue(IDs::NOISE_COLOR) = -0.4f;

        // Filter: 24dB LP with heavy saturation drive
        *processor.apvts.getRawParameterValue(IDs::FILTER_TYPE) = 1.0f; // LP24
        *processor.apvts.getRawParameterValue(IDs::FILTER_DRIVE) = 5.0f; // Active heavy tanh drive
        *processor.apvts.getRawParameterValue(IDs::FILTER_RES) = 5.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 2200.0f;
        *processor.apvts.getRawParameterValue(IDs::FILTER_ENV_AMT) = 0.6f;

        // Long release so voices linger and force violent voice stealing
        *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 2.0f;

        // Mod Matrix: Enable all 5x8 cells if requested
        if (enableFullModMatrix)
        {
            std::cout << "  [ModMatrix] Activating all 40 matrix routes (5 sources x 8 destinations)...\n";
            for (int r = 0; r < 5; ++r)
            {
                for (int c = 0; c < 8; ++c)
                {
                    auto id = IDs::getModCellId(r, c);
                    if (auto* p = processor.apvts.getRawParameterValue(id))
                        *p = (float)(1 + ((r + c) % 5)); // Cycle through colors
                }
            }
            *processor.apvts.getRawParameterValue(IDs::MOD_BUS_1_AMT) = 0.7f;
            *processor.apvts.getRawParameterValue(IDs::MOD_BUS_2_AMT) = -0.5f;
            *processor.apvts.getRawParameterValue(IDs::MOD_BUS_3_AMT) = 0.8f;
            *processor.apvts.getRawParameterValue(IDs::MOD_BUS_4_AMT) = -0.6f;
        }

        // Run 1000 blocks of continuous dynamic playing with violent voice stealing
        const int numBlocks = 1000;
        int overruns = 0;
        double totalTimeMs = 0.0;
        double peakTimeMs = 0.0;
        std::vector<double> blockTimes;
        blockTimes.reserve(numBlocks);

        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::MidiBuffer midi;

        for (int b = 0; b < numBlocks; ++b)
        {
            midi.clear();
            // Every 15 blocks (~43ms @ 128 samples), strike a new 16-note chord!
            // This forces 16 active voices to be stolen simultaneously while their release tails play!
            if (b % 15 == 0)
            {
                int chordRoot = 36 + ((b / 15) % 4) * 5;
                for (int n = 0; n < 16; ++n)
                    midi.addEvent(juce::MidiMessage::noteOn(1, chordRoot + (n * 2) % 36, (juce::uint8)110), 0);
            }

            // Real-time modulation: automate cutoff & pitch bend every block
            *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 1000.0f + 1500.0f * std::sin((float)b * 0.03f);
            midi.addEvent(juce::MidiMessage::pitchWheel(1, 8192 + (int)(3000.0f * std::sin((float)b * 0.05f))), 0);

            buffer.clear();
            auto t0 = std::chrono::high_resolution_clock::now();
            processor.processBlock(buffer, midi);
            auto t1 = std::chrono::high_resolution_clock::now();

            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            blockTimes.push_back(ms);
            totalTimeMs += ms;
            if (ms > peakTimeMs) peakTimeMs = ms;
            if (ms > budgetMs) overruns++;
        }

        std::sort(blockTimes.begin(), blockTimes.end());
        double avgTimeMs = totalTimeMs / numBlocks;
        double p95Ms = blockTimes[(size_t)(numBlocks * 0.95)];
        double p99Ms = blockTimes[(size_t)(numBlocks * 0.99)];
        double cpuPercent = (avgTimeMs / budgetMs) * 100.0;

        std::cout << "  Average Callback Time: " << avgTimeMs << " ms (" << cpuPercent << "% CPU)\n";
        std::cout << "  p95 Callback Time:     " << p95Ms << " ms\n";
        std::cout << "  p99 Callback Time:     " << p99Ms << " ms\n";
        std::cout << "  Peak Callback Time:    " << peakTimeMs << " ms\n";
        std::cout << "  Buffer Overruns:       " << overruns << " / " << numBlocks 
                  << " (" << (overruns * 100.0 / numBlocks) << "% failure rate)\n";

        if (overruns > 0)
        {
            std::cout << "  >>> CRITICAL: DEADLINE VIOLATION AT " << blockSize 
                      << " SAMPLES! WILL CAUSE REAL-TIME AUDIO STUTTER!\n";
        }
        else
        {
            std::cout << "  >>> SUCCESS: 0 Overruns! Completely within real-time budget.\n";
        }
        std::cout << "\n";

        return overruns;
    };

    int totalFails = 0;

    // 1. 128 samples without mod matrix (Baseline 3ms)
    totalFails += runTorture(128, "3ms Latency (128 samples) - 16 Notes, 4 Square+PWM OSCs, 8 Unison", false, false);

    // 2. 128 samples with FULL 5x8 mod matrix (Torture 3ms - All Square+PWM)
    totalFails += runTorture(128, "3ms Latency (128 samples) - 16 Notes, 4 Square+PWM OSCs, 8 Unison + FULL MOD MATRIX", true, false);

    // 3. 128 samples with FULL 5x8 mod matrix + Mixed Waveforms (Sine + Tri + Saw + Square)
    totalFails += runTorture(128, "3ms Latency (128 samples) - 16 Notes, Mixed (Sine/Tri/Saw/Sq), 8 Unison + FULL MOD MATRIX", true, true);

    // 4. 64 samples with FULL 5x8 mod matrix (Extreme 1.5ms - All Square+PWM)
    totalFails += runTorture(64, "1.5ms Latency (64 samples) - 16 Notes, 4 Square+PWM OSCs, 8 Unison + FULL MOD MATRIX", true, false);

    // 5. 64 samples with FULL 5x8 mod matrix + Mixed Waveforms (Sine + Tri + Saw + Square)
    totalFails += runTorture(64, "1.5ms Latency (64 samples) - 16 Notes, Mixed (Sine/Tri/Saw/Sq), 8 Unison + FULL MOD MATRIX", true, true);

    std::cout << "=================================================================\n";
    std::cout << "Torture Benchmark Complete. Total Overrun Blocks: " << totalFails << "\n";
    std::cout << "=================================================================\n";

    return totalFails > 0 ? 1 : 0;
}
