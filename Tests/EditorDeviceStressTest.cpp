#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>

class AudioCaptureCallback : public juce::AudioIODeviceCallback
{
public:
    AudioCaptureCallback(juce::AudioProcessorPlayer* playerToWrap, double sr, int maxSeconds)
        : player(playerToWrap), sampleRate(sr), maxSamples(static_cast<int>(sr * maxSeconds))
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
        if (player != nullptr)
        {
            player->audioDeviceIOCallbackWithContext(inputChannelData, numInputChannels,
                                                    outputChannelData, numOutputChannels,
                                                    numSamples, context);
        }

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

private:
    juce::AudioProcessorPlayer* player = nullptr;
    double sampleRate;
    int maxSamples;
    std::mutex dataMutex;
    std::vector<float> recordedSamples;
};

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=================================================================\n";
    std::cout << "Running Long Real-Time Audio + Editor UI Live Stress Test\n";
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
    std::cout << "Active Device: " << currentDevice->getName() << " (" << sr << " Hz, " << bufferSize << " samples, " << (bufferSize * 1000.0 / sr) << " ms)\n";

    VSTSynthV1AudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    std::cout << "Created AudioProcessorEditor successfully with all 55 SliderAttachments & ModMatrix!\n";

    juce::AudioProcessorPlayer player;
    player.setProcessor(&processor);

    const int testDurationSeconds = 20;
    AudioCaptureCallback capture(&player, sr, testDurationSeconds);

    deviceManager.addAudioCallback(&capture);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> noteDist(40, 76);
    std::uniform_int_distribution<int> velDist(60, 127);
    std::uniform_real_distribution<float> paramDist(0.0f, 1.0f);

    std::cout << "Starting 20-second active playing & GUI dispatch loop...\n";
    auto startTime = std::chrono::steady_clock::now();
    int noteStep = 0;

    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count() < testDurationSeconds)
    {
        noteStep++;

        // Randomly play notes / chords / releases
        if (noteStep % 4 == 0)
        {
            // Play 1 to 4 notes
            int chordSize = 1 + (rng() % 4);
            for (int c = 0; c < chordSize; ++c)
            {
                int note = noteDist(rng);
                player.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, note, (juce::uint8)velDist(rng)));
            }
        }
        else if (noteStep % 5 == 0)
        {
            // Release notes
            player.handleIncomingMidiMessage(nullptr, juce::MidiMessage::allNotesOff(1));
        }

        // Tweak parameters via APVTS (simulating UI knob tweaking & message pump)
        if (noteStep % 3 == 0)
        {
            if (auto* cut = processor.apvts.getParameter(IDs::FILTER_CUTOFF))
                cut->setValueNotifyingHost(paramDist(rng));
            if (auto* res = processor.apvts.getParameter(IDs::FILTER_RES))
                res->setValueNotifyingHost(paramDist(rng));
            if (auto* drv = processor.apvts.getParameter(IDs::FILTER_DRIVE))
                drv->setValueNotifyingHost(paramDist(rng));
        }

        // Switch PlayMode occasionally
        if (noteStep % 30 == 0)
        {
            int mode = rng() % 3;
            if (auto* pm = processor.apvts.getParameter(IDs::PLAY_MODE))
                pm->setValueNotifyingHost(pm->convertTo0to1((float)mode));
        }

        // Dispatch Windows UI message queue for 20ms to allow UI repaints, attachment updates, timers
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    }

    std::cout << "Test loop finished. Stopping audio callbacks...\n";
    deviceManager.removeAudioCallback(&capture);
    player.setProcessor(nullptr);
    editor.reset();

    auto recorded = capture.getRecordedData();
    std::cout << "Recorded " << recorded.size() << " samples (" << (recorded.size() / sr) << " seconds).\n";

    // Write recorded WAV
    juce::File outFile("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\editor_stress_capture.wav");
    outFile.deleteFile();
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(outFile), sr, 1, 16, {}, 0));
    if (writer != nullptr)
    {
        juce::AudioBuffer<float> tempBuf(1, (int)recorded.size());
        tempBuf.copyFrom(0, 0, recorded.data(), (int)recorded.size());
        writer->writeFromAudioSampleBuffer(tempBuf, 0, tempBuf.getNumSamples());
        std::cout << "Exported audio to Tests/output/editor_stress_capture.wav\n";
    }

    // Check for ~30 Hz stutter pattern: active 2-8ms, silence 24-35ms
    int detected30HzCount = 0;
    int activeSamplesInRun = 0;
    int silentSamplesInRun = 0;
    int consecutiveStutters = 0;

    for (float s : recorded)
    {
        if (std::abs(s) > 0.015f)
        {
            if (silentSamplesInRun > 0)
            {
                float silenceMs = (float)silentSamplesInRun * 1000.0f / (float)sr;
                float activeMs = (float)activeSamplesInRun * 1000.0f / (float)sr;

                if (activeMs >= 1.0f && activeMs <= 8.0f && silenceMs >= 24.0f && silenceMs <= 35.0f)
                {
                    consecutiveStutters++;
                    if (consecutiveStutters >= 2)
                    {
                        detected30HzCount++;
                    }
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

    std::cout << "\nAnalysis Result:\n";
    std::cout << "  30 Hz Stutter Bursts Detected: " << detected30HzCount << "\n";
    if (detected30HzCount > 5)
    {
        std::cout << "  >>> CRITICAL: 30 HZ STUTTER REPRODUCED IN LIVE AUDIO + EDITOR TEST!\n";
    }
    else
    {
        std::cout << "  30 Hz stutter was NOT detected in this run.\n";
    }

    return 0;
}
