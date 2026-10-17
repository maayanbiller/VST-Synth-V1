#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/DAHDSREnvelope.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    std::cout << "=== Recording Raw Amplitude Data Over Time ===" << std::endl;

    const double sampleRate = 44100.0;
    const int blockSize = 128;

    // Output files in Tests/output/
    juce::File outDir("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output");
    outDir.createDirectory();

    // Decode test_recording.mp3 to WAV
    {
        juce::AudioFormatManager formatMgr;
        formatMgr.registerBasicFormats();
        juce::File mp3File("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\test_recording.mp3");
        std::unique_ptr<juce::AudioFormatReader> reader(formatMgr.createReaderFor(mp3File));
        if (reader != nullptr)
        {
            std::cout << "MP3 Reader created! Samples: " << reader->lengthInSamples << ", SR: " << reader->sampleRate << ", Ch: " << reader->numChannels << std::endl;
            juce::AudioBuffer<float> decodedBuf((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&decodedBuf, 0, (int)reader->lengthInSamples, 0, true, true);
            
            juce::File wavOut("C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\test_recording.wav");
            wavOut.deleteFile();
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(wavOut), reader->sampleRate, reader->numChannels, 16, {}, 0));
            if (writer != nullptr)
            {
                writer->writeFromAudioSampleBuffer(decodedBuf, 0, decodedBuf.getNumSamples());
                std::cout << "Decoded WAV saved: " << wavOut.getFullPathName() << std::endl;
            }
        }
        else
        {
            std::cout << "Notice: AudioFormatManager could not directly read MP3 (may need MP3 format explicitly registered)." << std::endl;
        }
    }

    std::string txtPath = "C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\raw_amplitude_readable_document.txt";
    std::string csvAudioPath = "C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\raw_amplitude_audio_stream.csv";
    std::string csvEnvPath = "C:\\Users\\maaya\\Projects\\Synth\\VST-Synth-V1\\Tests\\output\\raw_amplitude_envelope_curve.csv";

    // -------------------------------------------------------------
    // Part 1: Record Pure DAHDSREnvelope Curve (Sample by Sample)
    // -------------------------------------------------------------
    DAHDSREnvelope env;
    env.setSampleRate(sampleRate);
    float attackSec = 0.100f;  // 100 ms
    float decaySec  = 0.150f;  // 150 ms
    float sustainLvl= 0.700f;  // 0.70
    float releaseSec= 0.200f;  // 200 ms
    DAHDSREnvelope::Parameters envP { 0.0f, attackSec, 0.0f, decaySec, sustainLvl, releaseSec };
    env.setParameters(envP);

    std::ofstream envCsv(csvEnvPath);
    envCsv << "sample_idx,time_ms,envelope_value,stage\n";

    std::vector<float> envSamples;
    std::vector<std::string> envStages;

    // Trigger noteOn
    env.noteOn();
    int attackSamples = static_cast<int>(attackSec * sampleRate);
    int decaySamples  = static_cast<int>(decaySec * sampleRate);
    int sustainSamples= static_cast<int>(0.100f * sampleRate); // hold sustain for 100ms
    int releaseSamples= static_cast<int>(releaseSec * sampleRate);

    // Record Attack
    for (int i = 0; i < attackSamples; ++i)
    {
        float val = env.getNextSample();
        envSamples.push_back(val);
        envStages.push_back("Attack");
    }

    // Record Decay
    for (int i = 0; i < decaySamples; ++i)
    {
        float val = env.getNextSample();
        envSamples.push_back(val);
        envStages.push_back("Decay");
    }

    // Record Sustain
    for (int i = 0; i < sustainSamples; ++i)
    {
        float val = env.getNextSample();
        envSamples.push_back(val);
        envStages.push_back("Sustain");
    }

    // Trigger noteOff
    env.noteOff();
    int postReleaseLimit = releaseSamples + 500;
    for (int i = 0; i < postReleaseLimit; ++i)
    {
        float val = env.getNextSample();
        envSamples.push_back(val);
        envStages.push_back(env.isActive() ? "Release" : "Idle/Silent");
    }

    for (size_t i = 0; i < envSamples.size(); ++i)
    {
        double tMs = (double)i * 1000.0 / sampleRate;
        envCsv << i << "," << std::fixed << std::setprecision(4) << tMs << "," 
               << std::setprecision(8) << envSamples[i] << "," << envStages[i] << "\n";
    }
    envCsv.close();

    // -------------------------------------------------------------
    // Part 2: Record Full Audio Output from VSTSynthV1AudioProcessor
    // -------------------------------------------------------------
    VSTSynthV1AudioProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    *processor.apvts.getRawParameterValue(IDs::OSC1_LEVEL) = 0.8f;
    *processor.apvts.getRawParameterValue(IDs::OSC1_WAVE) = 2.0f; // Saw
    *processor.apvts.getRawParameterValue(IDs::FILTER_CUTOFF) = 8000.0f;
    *processor.apvts.getRawParameterValue(IDs::AMP_ATTACK) = attackSec;
    *processor.apvts.getRawParameterValue(IDs::AMP_DECAY) = decaySec;
    *processor.apvts.getRawParameterValue(IDs::AMP_SUSTAIN) = sustainLvl;
    *processor.apvts.getRawParameterValue(IDs::AMP_RELEASE) = releaseSec;

    float preNoteSec = 0.020f;   // 20ms silence before note
    float noteHoldSec = 0.350f;  // 350ms note duration (covers attack, decay, sustain)
    float releaseWaitSec = 0.300f; // 300ms to allow 200ms release to fully finish into silence
    float totalSec = preNoteSec + noteHoldSec + releaseWaitSec;
    int totalSamples = static_cast<int>(totalSec * sampleRate);

    int noteOnSample = static_cast<int>(preNoteSec * sampleRate);
    int noteOffSample = static_cast<int>((preNoteSec + noteHoldSec) * sampleRate);

    juce::AudioBuffer<float> audioBuffer(2, totalSamples);
    audioBuffer.clear();

    int sampleIdx = 0;
    while (sampleIdx < totalSamples)
    {
        int toProcess = std::min(blockSize, totalSamples - sampleIdx);
        juce::AudioBuffer<float> chunk(audioBuffer.getArrayOfWritePointers(), 2, sampleIdx, toProcess);
        juce::MidiBuffer midi;

        if (sampleIdx <= noteOnSample && sampleIdx + toProcess > noteOnSample)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), noteOnSample - sampleIdx);
        }
        if (sampleIdx <= noteOffSample && sampleIdx + toProcess > noteOffSample)
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), noteOffSample - sampleIdx);
        }

        processor.processBlock(chunk, midi);
        sampleIdx += toProcess;
    }

    std::ofstream audioCsv(csvAudioPath);
    audioCsv << "sample_idx,time_sec,time_ms,audio_left,audio_right,state_label\n";

    for (int i = 0; i < totalSamples; ++i)
    {
        double tSec = (double)i / sampleRate;
        double tMs = tSec * 1000.0;
        float l = audioBuffer.getSample(0, i);
        float r = audioBuffer.getSample(1, i);

        std::string label = "Pre-Note";
        if (i >= noteOnSample && i < noteOffSample)
        {
            double noteMs = (i - noteOnSample) * 1000.0 / sampleRate;
            if (noteMs < attackSec * 1000.0) label = "Attack";
            else if (noteMs < (attackSec + decaySec) * 1000.0) label = "Decay";
            else label = "Sustain";
        }
        else if (i >= noteOffSample)
        {
            label = "Release_or_Tail";
        }

        audioCsv << i << "," << std::fixed << std::setprecision(6) << tSec << "," 
                 << std::setprecision(3) << tMs << "," 
                 << std::setprecision(8) << l << "," << r << "," << label << "\n";
    }
    audioCsv.close();

    // -------------------------------------------------------------
    // Part 3: Write Readable Text Document (raw_amplitude_readable_document.txt)
    // -------------------------------------------------------------
    std::ofstream txtOut(txtPath);
    txtOut << "====================================================================================\n";
    txtOut << "  VST SYNTH V1 - RAW AMPLITUDE RECORDING OVER TIME\n";
    txtOut << "  Target: Debug Standalone Engine (Sample Rate: 44,100 Hz)\n";
    txtOut << "  Settings: Attack = 100 ms | Decay = 150 ms | Sustain = 0.70 | Release = 200 ms\n";
    txtOut << "====================================================================================\n\n";

    txtOut << "------------------------------------------------------------------------------------\n";
    txtOut << "SECTION 1: ATTACK ONSET (Sample-by-sample for the first 35 samples from NoteOn)\n";
    txtOut << "------------------------------------------------------------------------------------\n";
    txtOut << std::setw(10) << "Sample" 
           << std::setw(14) << "Time (ms)" 
           << std::setw(20) << "Envelope Val" 
           << std::setw(22) << "Audio Amp Left" 
           << std::setw(22) << "Audio Amp Right" << "\n";
    txtOut << "------------------------------------------------------------------------------------\n";

    for (int i = 0; i < 35; ++i)
    {
        int audioIdx = noteOnSample + i;
        double tMs = (double)i * 1000.0 / sampleRate;
        float envVal = (i < (int)envSamples.size()) ? envSamples[i] : 0.0f;
        float aL = audioBuffer.getSample(0, audioIdx);
        float aR = audioBuffer.getSample(1, audioIdx);

        txtOut << std::setw(10) << i 
               << std::setw(14) << std::fixed << std::setprecision(4) << tMs 
               << std::setw(20) << std::setprecision(8) << envVal 
               << std::setw(22) << std::setprecision(8) << aL 
               << std::setw(22) << std::setprecision(8) << aR << "\n";
    }

    txtOut << "\n------------------------------------------------------------------------------------\n";
    txtOut << "SECTION 2: ATTACK PROGRESSION (Sampled every 5 ms during 100 ms Attack)\n";
    txtOut << "------------------------------------------------------------------------------------\n";
    txtOut << std::setw(10) << "Sample" 
           << std::setw(14) << "Time (ms)" 
           << std::setw(20) << "Envelope Val" 
           << std::setw(22) << "Audio Amp Left" 
           << std::setw(22) << "Audio Amp Right" << "\n";
    txtOut << "------------------------------------------------------------------------------------\n";

    for (int ms = 5; ms <= 100; ms += 5)
    {
        int step = static_cast<int>(ms * sampleRate / 1000.0);
        int audioIdx = noteOnSample + step;
        double tMs = (double)step * 1000.0 / sampleRate;
        float envVal = (step < (int)envSamples.size()) ? envSamples[step] : 0.0f;
        float aL = audioBuffer.getSample(0, audioIdx);
        float aR = audioBuffer.getSample(1, audioIdx);

        txtOut << std::setw(10) << step 
               << std::setw(14) << std::fixed << std::setprecision(2) << tMs 
               << std::setw(20) << std::setprecision(6) << envVal 
               << std::setw(22) << std::setprecision(6) << aL 
               << std::setw(22) << std::setprecision(6) << aR << "\n";
    }

    txtOut << "\n------------------------------------------------------------------------------------\n";
    txtOut << "SECTION 3: DECAY PROGRESSION (Sampled every 15 ms from Peak 1.0 to Sustain 0.70)\n";
    txtOut << "------------------------------------------------------------------------------------\n";
    txtOut << std::setw(10) << "Sample" 
           << std::setw(14) << "Time (ms)" 
           << std::setw(20) << "Envelope Val" 
           << std::setw(22) << "Audio Amp Left" 
           << std::setw(22) << "Audio Amp Right" << "\n";
    txtOut << "------------------------------------------------------------------------------------\n";

    for (int ms = 100; ms <= 250; ms += 15)
    {
        int step = static_cast<int>(ms * sampleRate / 1000.0);
        int audioIdx = noteOnSample + step;
        double tMs = (double)step * 1000.0 / sampleRate;
        float envVal = (step < (int)envSamples.size()) ? envSamples[step] : 0.0f;
        float aL = audioBuffer.getSample(0, audioIdx);
        float aR = audioBuffer.getSample(1, audioIdx);

        txtOut << std::setw(10) << step 
               << std::setw(14) << std::fixed << std::setprecision(2) << tMs 
               << std::setw(20) << std::setprecision(6) << envVal 
               << std::setw(22) << std::setprecision(6) << aL 
               << std::setw(22) << std::setprecision(6) << aR << "\n";
    }

    txtOut << "\n------------------------------------------------------------------------------------\n";
    txtOut << "SECTION 4: RELEASE PROGRESSION (Sampled every 10 ms from NoteOff at t=0ms to 200ms)\n";
    txtOut << "------------------------------------------------------------------------------------\n";
    txtOut << std::setw(10) << "RelSample" 
           << std::setw(14) << "RelTime (ms)" 
           << std::setw(20) << "Envelope Val" 
           << std::setw(22) << "Audio Amp Left" 
           << std::setw(22) << "Audio Amp Right" << "\n";
    txtOut << "------------------------------------------------------------------------------------\n";

    int relStartInEnv = attackSamples + decaySamples + sustainSamples;

    for (int ms = 0; ms <= 200; ms += 10)
    {
        int step = static_cast<int>(ms * sampleRate / 1000.0);
        int audioIdx = noteOffSample + step;
        int envIdx = relStartInEnv + step;

        double tMs = (double)step * 1000.0 / sampleRate;
        float envVal = (envIdx < (int)envSamples.size()) ? envSamples[envIdx] : 0.0f;
        float aL = (audioIdx < totalSamples) ? audioBuffer.getSample(0, audioIdx) : 0.0f;
        float aR = (audioIdx < totalSamples) ? audioBuffer.getSample(1, audioIdx) : 0.0f;

        txtOut << std::setw(10) << step 
               << std::setw(14) << std::fixed << std::setprecision(2) << tMs 
               << std::setw(20) << std::setprecision(6) << envVal 
               << std::setw(22) << std::setprecision(6) << aL 
               << std::setw(22) << std::setprecision(6) << aR << "\n";
    }

    txtOut << "\n------------------------------------------------------------------------------------\n";
    txtOut << "SECTION 5: RELEASE TAIL-END LANDING INTO ZERO (Final 25 active samples + 25 silent samples)\n";
    txtOut << "------------------------------------------------------------------------------------\n";
    txtOut << std::setw(10) << "RelSample" 
           << std::setw(14) << "RelTime (ms)" 
           << std::setw(20) << "Envelope Val" 
           << std::setw(22) << "Audio Amp Left" 
           << std::setw(22) << "Audio Amp Right" << "\n";
    txtOut << "------------------------------------------------------------------------------------\n";

    // Find sample where envelope lands on 0.0
    int landingSample = -1;
    for (size_t i = relStartInEnv; i < envSamples.size(); ++i)
    {
        if (envSamples[i] == 0.0f)
        {
            landingSample = static_cast<int>(i - relStartInEnv);
            break;
        }
    }

    if (landingSample > 0)
    {
        int startInspect = std::max(0, landingSample - 25);
        int endInspect = std::min(landingSample + 25, (int)(envSamples.size() - relStartInEnv));

        for (int s = startInspect; s < endInspect; ++s)
        {
            int audioIdx = noteOffSample + s;
            int envIdx = relStartInEnv + s;

            double tMs = (double)s * 1000.0 / sampleRate;
            float envVal = (envIdx < (int)envSamples.size()) ? envSamples[envIdx] : 0.0f;
            float aL = (audioIdx < totalSamples) ? audioBuffer.getSample(0, audioIdx) : 0.0f;
            float aR = (audioIdx < totalSamples) ? audioBuffer.getSample(1, audioIdx) : 0.0f;

            txtOut << std::setw(10) << s 
                   << std::setw(14) << std::fixed << std::setprecision(4) << tMs 
                   << std::setw(20) << std::setprecision(8) << envVal 
                   << std::setw(22) << std::setprecision(8) << aL 
                   << std::setw(22) << std::setprecision(8) << aR 
                   << (s == landingSample ? "  <-- EXACT ZERO LANDING POINT" : "") << "\n";
        }
    }

    txtOut.close();

    std::cout << "[DONE] Written to:" << std::endl;
    std::cout << "  - " << txtPath << std::endl;
    std::cout << "  - " << csvAudioPath << std::endl;
    std::cout << "  - " << csvEnvPath << std::endl;

    return 0;
}
