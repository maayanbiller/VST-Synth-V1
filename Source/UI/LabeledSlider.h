#pragma once
#include <JuceHeader.h>

class LabeledSlider : public juce::Component
{
public:
    LabeledSlider(const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromTop(20));
        slider.setBounds(area);
    }

    juce::Slider slider;
    juce::Label label;
};
