#pragma once
#include <JuceHeader.h>

class ModMatrixView : public juce::Component
{
public:
    ModMatrixView()
    {
        // Simple UI for the Mod Matrix Grid
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("Modulation Matrix (Coming Soon)", bounds, juce::Justification::centred, false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModMatrixView)
};
