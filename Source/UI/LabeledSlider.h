#pragma once
#include <JuceHeader.h>


class LabeledSlider : public juce::Component
{
public:
    class CustomSlider : public juce::Slider {};

    LabeledSlider(const juce::String& name, juce::Slider::SliderStyle style = juce::Slider::RotaryHorizontalVerticalDrag, bool useTextBox = true)

    {
        slider.setSliderStyle(style);
        showTextBox = useTextBox;
        
        slider.setName(name);
        
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        
        if (label.getText().isNotEmpty())
            label.setBounds(area.removeFromTop(15));
        
        if (area.getHeight() < 40)
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        else
            
            if (showTextBox) slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 85, 20);
            else slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);


        slider.setBounds(area);
    }

    CustomSlider slider;
    juce::Label label;
    bool showTextBox = true;
    
};

