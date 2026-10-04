#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class VSTSynthV1AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    VSTSynthV1AudioProcessorEditor (VSTSynthV1AudioProcessor&);
    ~VSTSynthV1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VSTSynthV1AudioProcessor& audioProcessor;

    // Component Groups
    juce::GroupComponent osc1Group{"Osc1", "Oscillator 1"};
    juce::GroupComponent osc2Group{"Osc2", "Oscillator 2"};
    juce::GroupComponent filterGroup{"Filter", "Filter"};
    juce::GroupComponent envGroup{"Env", "Envelopes"};
    juce::GroupComponent lfoGroup{"LFO", "LFOs"};

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::ComboBox osc1WaveCombo;
    std::unique_ptr<ComboBoxAttachment> osc1WaveComboAtt;
    juce::Slider osc1SemiSlider;
    std::unique_ptr<SliderAttachment> osc1SemiSliderAtt;
    juce::Slider osc1DetuneSlider;
    std::unique_ptr<SliderAttachment> osc1DetuneSliderAtt;
    juce::Slider osc1LevelSlider;
    std::unique_ptr<SliderAttachment> osc1LevelSliderAtt;
    juce::Slider osc1PwmSlider;
    std::unique_ptr<SliderAttachment> osc1PwmSliderAtt;
    juce::ComboBox osc2WaveCombo;
    std::unique_ptr<ComboBoxAttachment> osc2WaveComboAtt;
    juce::Slider osc2SemiSlider;
    std::unique_ptr<SliderAttachment> osc2SemiSliderAtt;
    juce::Slider osc2DetuneSlider;
    std::unique_ptr<SliderAttachment> osc2DetuneSliderAtt;
    juce::Slider osc2LevelSlider;
    std::unique_ptr<SliderAttachment> osc2LevelSliderAtt;
    juce::Slider osc2PwmSlider;
    std::unique_ptr<SliderAttachment> osc2PwmSliderAtt;
    juce::ComboBox subWaveCombo;
    std::unique_ptr<ComboBoxAttachment> subWaveComboAtt;
    juce::Slider subOctSlider;
    std::unique_ptr<SliderAttachment> subOctSliderAtt;
    juce::Slider subLevelSlider;
    std::unique_ptr<SliderAttachment> subLevelSliderAtt;
    juce::Slider noiseLevelSlider;
    std::unique_ptr<SliderAttachment> noiseLevelSliderAtt;
    juce::ComboBox filterTypeCombo;
    std::unique_ptr<ComboBoxAttachment> filterTypeComboAtt;
    juce::Slider filterCutoffSlider;
    std::unique_ptr<SliderAttachment> filterCutoffSliderAtt;
    juce::Slider filterResSlider;
    std::unique_ptr<SliderAttachment> filterResSliderAtt;
    juce::Slider filterDriveSlider;
    std::unique_ptr<SliderAttachment> filterDriveSliderAtt;
    juce::Slider filterEnvAmtSlider;
    std::unique_ptr<SliderAttachment> filterEnvAmtSliderAtt;
    juce::Slider ampAttackSlider;
    std::unique_ptr<SliderAttachment> ampAttackSliderAtt;
    juce::Slider ampDecaySlider;
    std::unique_ptr<SliderAttachment> ampDecaySliderAtt;
    juce::Slider ampSustainSlider;
    std::unique_ptr<SliderAttachment> ampSustainSliderAtt;
    juce::Slider ampReleaseSlider;
    std::unique_ptr<SliderAttachment> ampReleaseSliderAtt;
    juce::Slider filtAttackSlider;
    std::unique_ptr<SliderAttachment> filtAttackSliderAtt;
    juce::Slider filtDecaySlider;
    std::unique_ptr<SliderAttachment> filtDecaySliderAtt;
    juce::Slider filtSustainSlider;
    std::unique_ptr<SliderAttachment> filtSustainSliderAtt;
    juce::Slider filtReleaseSlider;
    std::unique_ptr<SliderAttachment> filtReleaseSliderAtt;
    juce::ComboBox lfo1WaveCombo;
    std::unique_ptr<ComboBoxAttachment> lfo1WaveComboAtt;
    juce::Slider lfo1RateSlider;
    std::unique_ptr<SliderAttachment> lfo1RateSliderAtt;
    juce::Slider lfo1DepthSlider;
    std::unique_ptr<SliderAttachment> lfo1DepthSliderAtt;
    juce::Slider masterVolSlider;
    std::unique_ptr<SliderAttachment> masterVolSliderAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSynthV1AudioProcessorEditor)
};
