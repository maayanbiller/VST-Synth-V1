#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

class AudioCaptureCallback : public juce::AudioIODeviceCallback
{
public:
    AudioCaptureCallback(juce::AudioProcessorPlayer* playerToWrap, double sr, int maxSeconds, double budgetMs)
        : player(playerToWrap), sampleRate(sr), maxSamples(static_cast<int>(sr * maxSeconds)), bufferBudgetMs(budgetMs)
    {
        recordedSamples.reserve(maxSamples);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        if (player != nullptr)
            player->audioDeviceAboutToStart(device);

        if (device != nullptr)
        {
            std::cout << "[Device] Started: " << device->getName() 
                      << ", SampleRate: " << device->getCurrentSampleRate() 
                      << ", BufferSize: " << device->getCurrentBufferSizeSamples() 
                      << " (" << (device->getCurrentBufferSizeSamples() * 1000.0 / device->getCurrentSampleRate()) << " ms)\n";
        }
    }

    void audioDeviceStopped() override
    {
        if (player != nullptr)
            player->audioDeviceStopped();
        std::cout << "[Device] Stopped.\n";
    }

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        if (player != nullptr)
        {
            player->audioDeviceIOCallbackWithContext(inputChannelData, numInputChannels,
                                                    outputChannelData, numOutputChannels,
                                                    numSamples, context);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        totalCallbackMs += ms;
        if (ms > maxCallbackMs) maxCallbackMs = ms;
        if (ms > bufferBudgetMs) overrunCount++;
        totalCallbacks++;

        if (numOutputChannels > 0 && outputChannelData[0] != nullptr)
        {
            const float* ch0 = outputChannelData[0];
            std::lock_guard<std::mutex> lock(dataMutex);
            for (int i = 0; i < numSamples && (int)recordedSamples.size() < maxSamples; ++i)
            {
                recordedSamples.push_back(ch0[i]);
            }
        }
    }

    std::vector<float> getRecordedData()
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        return recordedSamples;
    }

    int getOverrunCount() const { return overrunCount; }
    int getTotalCallbacks() const { return totalCallbacks; }
    double getMaxCallbackMs() const { return maxCallbackMs; }
    double getAvgCallbackMs() const { return totalCallbacks > 0 ? (totalCallbackMs / totalCallbacks) : 0.0; }

private:
    juce::AudioProcessorPlayer* player = nullptr;
    double sampleRate;
    int maxSamples;
    double bufferBudgetMs = 10.0;
    std::atomic<int> overrunCount { 0 };
    std::atomic<int> totalCallbacks { 0 };
    double maxCallbackMs = 0.0;
    double totalCallbackMs = 0.0;
    std::mutex dataMutex;
    std::vector<float> recordedSamples;
};

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=================================================================\n";
    std::cout << "Running Real Audio Device & Driver Test with VSTSynthV1\n";
    std::cout << "=================================================================\n";

    juce::AudioDeviceManager deviceManager;
    juce::String error = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (error.isNotEmpty())
    {
        std::cerr << "Error initializing audio device: " << error << "\n";
        return 1;
    }

    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    if (currentDevice == nullptr)
    {
        std::cerr << "No audio device available!\n";
        return 1;
    }

    double sr = currentDevice->getCurrentSampleRate();
    int bufferSize = currentDevice->getCurrentBufferSizeSamples();
    std::cout << "Active Audio Device: " << currentDevice->getName() << "\n";
    std::cout << "Active Device Type: " << currentDevice->getTypeName() << "\n";
    std::cout << "Sample Rate: " << sr << " Hz\n";
    std::cout << "Buffer Size: " << bufferSize << " samples (" << (bufferSize * 1000.0 / sr) << " ms)\n\n";

    VSTSynthV1AudioProcessor processor;
    juce::AudioProcessorPlayer player;
    player.setProcessor(&processor);

    AudioCaptureCallback capture(&player, sr, 15, bufferSize * 1000.0 / sr); // record up to 15 seconds
    deviceManager.addAudioCallback(&capture);

    std::cout << "Playing note 60 for 2 seconds (Baseline)...\n";
    player.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    player.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 60, (juce::uint8)0));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << "Configuring 8 Unison Voices across ALL 4 Oscillators with 1.5s Amp Release...\n";
    *processor.apvts.getRawParameterValue(IDs::OSC1_VOICES) = 8.0f;
    *processor.apvts.getRawParameterValue(IDs::OSC1_WIDTH) = 0.8f;
    *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
    *processor.apvts.getRawParameterValue(IDs::OSC2_LEVEL) = 0.7f;
    *processor.apvts.getRawParameterValue(IDs::OSC2_VOICES) = 8.0f;
    *processor.apvts.getRawParameterValue(IDs::OSC2_WIDTH) = 0.8f;
    *processor.apvts.getRawParameterValue(IDs::OSC3_LEVEL) = 0.7f;
    *processor.apvts.getRawParameterValue(IDs::OSC3_VOICES) = 8.0f;
    *processor.apvts.getRawParameterValue(IDs::OSC3_WIDTH) = 0.8f;
    *processor.apvts.getRawParameterValue(IDs::OSC4_LEVEL) = 0.7f;
    *processor.apvts.getRawParameterValue(IDs::OSC4_VOICES) = 8.0f;
    *processor.apvts.getRawParameterValue(IDs::OSC4_WIDTH) = 0.8f;
    *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = 1.5f;

    std::cout << "Playing 16-note polyphonic saturation chord for 5 seconds on hardware device...\n";
    for (int n = 0; n < 16; ++n)
        player.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 36 + n * 2, (juce::uint8)100));
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int n = 0; n < 16; ++n)
        player.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 36 + n * 2, (juce::uint8)0));
    std::this_thread::sleep_for(std::chrono::seconds(2));

    deviceManager.removeAudioCallback(&capture);
    player.setProcessor(nullptr);

    std::cout << "\n=================================================================\n";
    std::cout << "Audio Hardware Callback Telemetry:\n";
    std::cout << "  Total Callbacks Processed: " << capture.getTotalCallbacks() << "\n";
    std::cout << "  Hardware Buffer Deadline:  " << (bufferSize * 1000.0 / sr) << " ms\n";
    std::cout << "  Average Callback Time:     " << capture.getAvgCallbackMs() << " ms ("
              << (capture.getAvgCallbackMs() / (bufferSize * 1000.0 / sr) * 100.0) << "% CPU)\n";
    std::cout << "  Peak Callback Time:        " << capture.getMaxCallbackMs() << " ms\n";
    std::cout << "  Buffer Overruns / Glitches:" << capture.getOverrunCount() << " / " << capture.getTotalCallbacks() << "\n";
    std::cout << "=================================================================\n";

    // Analyze recorded audio
    auto recorded = capture.getRecordedData();
    std::cout << "\nCaptured " << recorded.size() << " samples (" << (recorded.size() / sr) << " seconds) from real audio device.\n";

    // Write to Tests/output/real_device_recording.wav
    juce::File outFile("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\real_device_recording.wav");
    outFile.deleteFile();
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(outFile), sr, 1, 16, {}, 0));
    if (writer != nullptr)
    {
        juce::AudioBuffer<float> tempBuf(1, (int)recorded.size());
        tempBuf.copyFrom(0, 0, recorded.data(), (int)recorded.size());
        writer->writeFromAudioSampleBuffer(tempBuf, 0, tempBuf.getNumSamples());
        std::cout << "Wrote real device capture to: Tests/output/real_device_recording.wav\n";
    }

    // Check for stutter bursts (active 2-5ms, silent 25-32ms)
    int stutterBurstCount = 0;
    int activeRun = 0;
    int silentRun = 0;
    for (float s : recorded)
    {
        if (std::abs(s) > 0.015f)
        {
            if (silentRun > 0)
            {
                float silenceMs = silentRun * 1000.0f / (float)sr;
                float activeMs = activeRun * 1000.0f / (float)sr;
                if (activeMs >= 1.0f && activeMs <= 8.0f && silenceMs >= 24.0f && silenceMs <= 35.0f)
                {
                    stutterBurstCount++;
                }
                activeRun = 0;
                silentRun = 0;
            }
            activeRun++;
        }
        else
        {
            silentRun++;
        }
    }

    std::cout << "Analysis Result:\n";
    std::cout << "  30 Hz Stutter Bursts Detected: " << stutterBurstCount << "\n";
    if (stutterBurstCount > 5)
    {
        std::cout << "  >>> CRITICAL: 30 Hz STUTTER REPRODUCED ON REAL AUDIO DEVICE!\n";
    }
    else
    {
        std::cout << "  Playback on this device was clean (no 30 Hz stutter).\n";
    }

    return 0;
}
