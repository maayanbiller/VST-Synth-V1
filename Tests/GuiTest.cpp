#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/UI/PluginEditor.h"

juce::File logFile("C:\\Users\\maaya\\projects\\Synth\\VST-Synth-V1\\gui_test_log.txt");

void verifyComponent(juce::Component* comp, const juce::String& prefix)
{
    if (comp == nullptr) return;
    
    juce::String name = comp->getName();
    if (name.isEmpty()) name = "Unnamed";
    
    logFile.appendText(prefix + " -> " + name + " (" + juce::String(typeid(*comp).name()) + ") " + comp->getBounds().toString() + (comp->isVisible() ? " V" : " HIDDEN") + "\n");
    // Check bounds
    if (comp->getWidth() <= 0 || comp->getHeight() <= 0)
    {
        logFile.appendText("[ERROR] Component " + prefix + " -> " + name + " has invalid bounds: " + comp->getBounds().toString() + "\n");
    }
    
    // Set values if Slider or ComboBox to test virtual control
    if (auto* slider = dynamic_cast<juce::Slider*>(comp))
    {
        logFile.appendText("   SLIDER " + name + " min=" + juce::String(slider->getMinimum()) + " max=" + juce::String(slider->getMaximum()) + " int=" + juce::String(slider->getInterval()) + " val=" + juce::String(slider->getValue()) + " skew=" + juce::String(slider->getSkewFactor()) + " prop=" + juce::String(slider->getNormalisableRange().convertTo0to1(slider->getValue())) + "\n");
        slider->setValue(slider->getMinimum() + (slider->getMaximum() - slider->getMinimum()) * 0.5);
    }
    else if (auto* box = dynamic_cast<juce::ComboBox*>(comp))
    {
        if (box->getNumItems() > 0)
            box->setSelectedId(1, juce::dontSendNotification);
    }
    
    for (auto* child : comp->getChildren())
    {
        verifyComponent(child, prefix + "/" + name);
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    logFile.replaceWithText("Starting virtual test of GUI components...\n");
    int ks = 0; auto kd = BinaryData::getNamedResource("wood_knob_png", ks);
    juce::PNGImageFormat pngF; juce::Image img = pngF.decodeImage(juce::MemoryInputStream(kd, ks, false));
    logFile.appendText("Image valid: " + juce::String(img.isValid() ? "YES" : "NO") + "\n"); if (!img.isValid()) { logFile.appendText("Failed to load wood_knob.png from BinaryData!\n"); }
    
    VSTSynthV1AudioProcessor processor;
    logFile.appendText("Processor created.\n");
    if (processor.apvts.getParameter("MOD_SRC_0_DST_0") == nullptr) logFile.appendText("MOD_SRC_0_DST_0 is null!\n");
    if (processor.apvts.getParameter("OSC1_WAVE") == nullptr) logFile.appendText("OSC1_WAVE is null!\n");
    if (processor.apvts.getParameter("PLAY_MODE") == nullptr) logFile.appendText("PLAY_MODE is null!\n");
    
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditorIfNeeded());
    logFile.appendText("Editor created.\n");
    
    if (editor != nullptr)
    {
        editor->setSize(1200, 800);
        logFile.appendText("Editor size set.\n");
        editor->resized();
        logFile.appendText("Editor resized.\n");
        logFile.appendText("Verifying all child components...\n");
        verifyComponent(editor.get(), "Root");
        
        juce::Image image(juce::Image::ARGB, 1200, 800, true, juce::SoftwareImageType());
        if (argc > 2 && juce::String(argv[1]) == "snap")
        {
            for (int ai = 2; ai < argc; ++ai)
            {
                int idx = juce::String(argv[ai]).getIntValue();
                auto* ch = editor->getChildComponent(idx);
                juce::Image s = ch->createComponentSnapshot(ch->getLocalBounds(), true, 4.0f);
                juce::File sf("C:\\Users\\maaya\\projects\\Synth\\VST-Synth-V1\\snap_" + juce::String(idx) + ".png");
                sf.deleteFile();
                juce::FileOutputStream so(sf);
                juce::PNGImageFormat().writeImageToStream(s, so);
            }
            return 0;
        }
        juce::Graphics g(image);
        for (int ai = 1; ai < argc; ++ai)
        {
            int hideIdx = juce::String(argv[ai]).getIntValue();
            editor->getChildComponent(hideIdx)->setVisible(false);
            logFile.appendText("Hid child " + juce::String(hideIdx) + "\n");
        }
        editor->paintEntireComponent(g, true);
        
        juce::File outputFile("C:\\Users\\maaya\\projects\\Synth\\VST-Synth-V1\\headless_gui_render.png");
        outputFile.deleteFile(); juce::FileOutputStream stream(outputFile);
        juce::PNGImageFormat pngWriter;
        pngWriter.writeImageToStream(image, stream);
        logFile.appendText("Test passed. Render saved.\n");
        
        // --- ADDED AUDIO RENDER TEST ---
        processor.prepareToPlay(44100.0, 512);

        juce::AudioBuffer<float> buffer(2, 44100 * 5); // 5 seconds of audio
        buffer.clear();
        
        int numSamples = buffer.getNumSamples();
        int blockSize = 512;
        int sampleIdx = 0;
        
        while (sampleIdx < numSamples) {
            int processThisTime = juce::jmin(blockSize, numSamples - sampleIdx);
            juce::AudioBuffer<float> subBuffer(buffer.getArrayOfWritePointers(), 2, sampleIdx, processThisTime);
            juce::MidiBuffer subMidi;
            if (sampleIdx == 0) subMidi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
            if (sampleIdx <= 44100 * 2 && sampleIdx + processThisTime > 44100 * 2) subMidi.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 44100 * 2 - sampleIdx);
            
            processor.processBlock(subBuffer, subMidi);
            sampleIdx += processThisTime;
        }
        
        juce::File outWav("C:\\Users\\maaya\\projects\\Synth\\VST-Synth-V1\\test_out.wav");
        outWav.deleteFile();
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(new juce::FileOutputStream(outWav), 44100, 2, 16, {}, 0));
        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        logFile.appendText("Audio render saved.\n");
    }
    return 0;
}


