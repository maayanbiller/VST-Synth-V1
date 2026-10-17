#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <chrono>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=================================================================\n";
    std::cout << "Deep CPU & Latency Profiling across Parameter Configurations\n";
    std::cout << "Target: 441 samples @ 44.1 kHz -> Hardware Budget is EXACTLY 10.0 ms\n";
    std::cout << "=================================================================\n\n";

    const double sampleRate = 44100.0;
    const int blockSize = 441; // Exact Windows WASAPI Shared Mode buffer size
    const double budgetMs = 10.0;

    auto testConfig = [&](const std::string& name, int numNotes, int numOscs, int unisonVoices, float drive, int filterType)
    {
        VSTSynthV1AudioProcessor processor;
        processor.prepareToPlay(sampleRate, blockSize);

        // Configure oscillators
        for (int o = 0; o < 4; ++o)
        {
            float level = (o < numOscs) ? 0.7f : 0.0f;
            if (o == 0) *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = level;
            if (o == 1) *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = level;
            if (o == 2) *processor.apvts.getRawParameterValue(IDs::OSC3_LEVEL) = level;
            if (o == 3) *processor.apvts.getRawParameterValue(IDs::OSC4_LEVEL) = level;

            if (o == 0) *processor.apvts.getRawParameterValue(IDs::OSC1_VOICES) = unisonVoices;
            if (o == 1) *processor.apvts.getRawParameterValue(IDs::OSC2_VOICES) = unisonVoices;
            if (o == 2) *processor.apvts.getRawParameterValue(IDs::OSC3_VOICES) = unisonVoices;
            if (o == 3) *processor.apvts.getRawParameterValue(IDs::OSC4_VOICES) = unisonVoices;
        }

        *processor.apvts.getRawParameterValue(IDs::FILTER_TYPE) = (float)filterType;
        *processor.apvts.getRawParameterValue(IDs::FILTER_DRIVE) = drive;
        *processor.apvts.getRawParameterValue(IDs::FILTER_RES) = 3.0f;

        // Start notes
        juce::MidiBuffer midiStart;
        for (int n = 0; n < numNotes; ++n)
            midiStart.addEvent(juce::MidiMessage::noteOn(1, 48 + n * 2, (juce::uint8)100), 0);

        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        processor.processBlock(buffer, midiStart);

        // Profile 200 blocks (~2 seconds of continuous audio)
        juce::MidiBuffer emptyMidi;
        double totalTimeMs = 0.0;
        double maxTimeMs = 0.0;
        int overruns = 0;
        int iterations = 200;

        for (int i = 0; i < iterations; ++i)
        {
            buffer.clear();
            auto t0 = std::chrono::high_resolution_clock::now();
            processor.processBlock(buffer, emptyMidi);
            auto t1 = std::chrono::high_resolution_clock::now();

            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            totalTimeMs += ms;
            if (ms > maxTimeMs) maxTimeMs = ms;
            if (ms > budgetMs) overruns++;
        }

        double avgMs = totalTimeMs / iterations;
        double cpuPct = (avgMs / budgetMs) * 100.0;

        std::cout << name << ":\n"
                  << "  Avg: " << avgMs << " ms (" << cpuPct << "% CPU), "
                  << "Peak: " << maxTimeMs << " ms, "
                  << "Overruns (>10ms): " << overruns << " / " << iterations << "\n";

        if (overruns > 0)
        {
            std::cout << "  >>> WARNING: HARDWARE BUFFER DEADLINE VIOLATED! Will cause stutter in WASAPI!\n";
        }
        std::cout << "\n";
    };

    // Test configurations
    testConfig("1 Note, 1 OSC, 1 Unison Voice (Baseline)", 1, 1, 1, 1.0f, 0);
    testConfig("4 Notes, 1 OSC, 1 Unison Voice", 4, 1, 1, 1.0f, 0);
    testConfig("4 Notes, 1 OSC, 4 Unison Voices", 4, 1, 4, 1.0f, 0);
    testConfig("4 Notes, 1 OSC, 8 Unison Voices (Max Unison)", 4, 1, 8, 1.0f, 0);
    testConfig("4 Notes, 4 OSCs, 1 Unison Voice", 4, 4, 1, 1.0f, 0);
    testConfig("4 Notes, 4 OSCs, 8 Unison Voices", 4, 4, 8, 1.0f, 0);
    testConfig("8 Notes (Chord), 4 OSCs, 8 Unison Voices", 8, 4, 8, 1.0f, 0);
    testConfig("16 Notes (Polyphony Max), 4 OSCs, 8 Unison Voices (Extreme Saturation)", 16, 4, 8, 1.0f, 1);

    return 0;
}
