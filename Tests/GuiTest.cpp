#include <JuceHeader.h>
#include <iostream>
#include "../Source/PluginProcessor.h"
#include "../Source/UI/PluginEditor.h"

class GuiTest : public juce::UnitTest
{
public:
    GuiTest() : juce::UnitTest ("GUI Component Test") {}

    void runTest() override
    {
        beginTest ("Instantiating Processor");
        
        // Initialize GUI manager for headless UI creation
        juce::ScopedJuceInitialiser_GUI guiInitialiser;
        
        auto processor = std::make_unique<VSTSynthV1AudioProcessor>();
        expect (processor != nullptr, "Processor failed to instantiate");
        
        beginTest ("Instantiating Editor");
        auto* editor = processor->createEditorIfNeeded();
        expect (editor != nullptr, "Editor failed to instantiate");
        
        if (editor != nullptr)
        {
            expect (editor->getWidth() == 800, "Editor width should be 800");
            expect (editor->getHeight() == 600, "Editor height should be 600");
            
            // Wait, we don't have components yet! We will add them in a moment.
            // But this validates the Editor can be created headlessly.
        }
        
        beginTest ("APVTS Parameters");
        // Verify APVTS has expected parameters
        auto* masterVol = processor->apvts.getParameter("MASTER_VOL");
        expect (masterVol != nullptr, "MASTER_VOL parameter is missing");
        
        if (masterVol)
        {
            masterVol->setValueNotifyingHost(0.75f);
            expectEquals (masterVol->getValue(), 0.75f, "Parameter value didn't update");
        }
        
        processor->editorBeingDeleted(editor);
        delete editor;
    }
};

static GuiTest guiTest;

int main (int argc, char* argv[])
{
    juce::ignoreUnused (argc, argv);
    
    // Create a logger that writes to stdout
    class ConsoleLogger : public juce::Logger {
        void logMessage (const juce::String& message) override { std::cout << message << std::endl; }
    };
    ConsoleLogger logger;
    juce::Logger::setCurrentLogger (&logger);
    
    std::cout << "Starting Test Runner..." << std::endl;
    juce::UnitTestRunner runner;
    runner.runAllTests();
    std::cout << "Tests Completed." << std::endl;
    
    juce::Logger::setCurrentLogger (nullptr);
    return 0;
}
