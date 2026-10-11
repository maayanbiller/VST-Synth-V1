#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceGui;

    std::cout << "--- MEASURING SYNTH LATENCY ---" << std::endl;
    VSTSynthV1AudioProcessor processor;

    double sampleRate = 44100.0;
    int blockSize = 1024;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    juce::MidiBuffer midiMessages;
    // Inject a Note On at exactly sample 0
    midiMessages.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);

    processor.processBlock(buffer, midiMessages);

    int firstSampleWithAudio = -1;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (std::abs(buffer.getSample(0, i)) > 0.00001f)
        {
            firstSampleWithAudio = i;
            break;
        }
    }

    if (firstSampleWithAudio != -1)
    {
        double latencyMs = (firstSampleWithAudio / sampleRate) * 1000.0;
        std::cout << "Audio output detected at sample: " << firstSampleWithAudio << std::endl;
        std::cout << "Internal DSP Latency: " << latencyMs << " ms" << std::endl;
    }
    else
    {
        std::cout << "No audio output detected in the first block!" << std::endl;
    }

    return 0;
}