#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "SynthLookAndFeel.h"
#include "LabeledSlider.h"


#include "ModMatrixComponent.h"

class VSTSynthV1AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    VSTSynthV1AudioProcessorEditor (VSTSynthV1AudioProcessor&);
    ~VSTSynthV1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VSTSynthV1AudioProcessor& audioProcessor;
    SynthLookAndFeel customLookAndFeel;

    juce::GroupComponent osc1Group{"", "OSC 1"}, osc2Group{"", "OSC 2"}, osc3Group{"", "OSC 3"}, osc4Group{"", "OSC 4"};
    juce::GroupComponent filterGroup{"", "FILTER"}, mixerGroup{"", "MIXER"}, envGroup{"", "ENVELOPES"}, lfoGroup{"", "LFO"};

    LabeledSlider osc1WaveSlider{"Wave", juce::Slider::RotaryHorizontalVerticalDrag, false}, osc1OctSlider{"Octave"}, osc1DetuneSlider{"Detune"}, osc1PwmSlider{"PWM"}, osc1VoicesSlider{"Voices"}, osc1WidthSlider{"Width"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1WaveAttach, osc1OctAttach, osc1DetuneAttach, osc1PwmAttach, osc1VoicesAttach, osc1WidthAttach;

    LabeledSlider osc2WaveSlider{"Wave", juce::Slider::RotaryHorizontalVerticalDrag, false}, osc2OctSlider{"Octave"}, osc2DetuneSlider{"Detune"}, osc2PwmSlider{"PWM"}, osc2VoicesSlider{"Voices"}, osc2WidthSlider{"Width"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2WaveAttach, osc2OctAttach, osc2DetuneAttach, osc2PwmAttach, osc2VoicesAttach, osc2WidthAttach;

    LabeledSlider osc3WaveSlider{"Wave", juce::Slider::RotaryHorizontalVerticalDrag, false}, osc3OctSlider{"Octave"}, osc3DetuneSlider{"Detune"}, osc3PwmSlider{"PWM"}, osc3VoicesSlider{"Voices"}, osc3WidthSlider{"Width"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc3WaveAttach, osc3OctAttach, osc3DetuneAttach, osc3PwmAttach, osc3VoicesAttach, osc3WidthAttach;

    LabeledSlider osc4WaveSlider{"Wave", juce::Slider::RotaryHorizontalVerticalDrag, false}, osc4OctSlider{"Octave"}, osc4DetuneSlider{"Detune"}, osc4PwmSlider{"PWM"}, osc4VoicesSlider{"Voices"}, osc4WidthSlider{"Width"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc4WaveAttach, osc4OctAttach, osc4DetuneAttach, osc4PwmAttach, osc4VoicesAttach, osc4WidthAttach;

    LabeledSlider osc1LevelSlider{"OSC 1", juce::Slider::LinearVertical}, osc2LevelSlider{"OSC 2", juce::Slider::LinearVertical}, 
                  osc3LevelSlider{"OSC 3", juce::Slider::LinearVertical}, osc4LevelSlider{"OSC 4", juce::Slider::LinearVertical}, 
                  masterVolSlider{"Master", juce::Slider::LinearVertical};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1LevelAttach, osc2LevelAttach, osc3LevelAttach, osc4LevelAttach, masterVolAttach;

    LabeledSlider filterCutoffSlider{"Cutoff"}, filterResSlider{"Resonance"}, filterDriveSlider{"Drive"}, filterEnvAmtSlider{"Env Amt"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttach, filterResAttach, filterDriveAttach, filterEnvAmtAttach;

    LabeledSlider ampAttackSlider{"A", juce::Slider::LinearVertical}, ampDecaySlider{"D", juce::Slider::LinearVertical}, ampSustainSlider{"S", juce::Slider::LinearVertical}, ampReleaseSlider{"R", juce::Slider::LinearVertical};
    LabeledSlider filtAttackSlider{"A", juce::Slider::LinearVertical}, filtDecaySlider{"D", juce::Slider::LinearVertical}, filtSustainSlider{"S", juce::Slider::LinearVertical}, filtReleaseSlider{"R", juce::Slider::LinearVertical};
    LabeledSlider modAttackSlider{"A", juce::Slider::LinearVertical}, modDecaySlider{"D", juce::Slider::LinearVertical}, modSustainSlider{"S", juce::Slider::LinearVertical}, modReleaseSlider{"R", juce::Slider::LinearVertical};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAttackAttach, ampDecayAttach, ampSustainAttach, ampReleaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtAttackAttach, filtDecayAttach, filtSustainAttach, filtReleaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modAttackAttach, modDecayAttach, modSustainAttach, modReleaseAttach;
    
    juce::Label ampLabel{"", "AMP ENV"}, filtLabel{"", "FILTER ENV"}, modLabel{"", "MOD ENV"};

    LabeledSlider lfo1WaveSlider{"Wave 1", juce::Slider::RotaryHorizontalVerticalDrag, false}, lfo1RateSlider{"Rate 1"}, lfo1DepthSlider{"Depth 1"};
    LabeledSlider lfo2WaveSlider{"Wave 2", juce::Slider::RotaryHorizontalVerticalDrag, false}, lfo2RateSlider{"Rate 2"}, lfo2DepthSlider{"Depth 2"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1WaveAttach, lfo1RateAttach, lfo1DepthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2WaveAttach, lfo2RateAttach, lfo2DepthAttach;

    juce::GroupComponent modGroup{"", "MOD MATRIX"};
    std::unique_ptr<VintageModMatrixComponent> vintageModMatrix;

    juce::GroupComponent playGroup{"", "PLAY"}, noiseGroup{"", "NOISE"};
    LabeledSlider playModeSlider{"Mode"}, glideSlider{"Glide"};
    LabeledSlider noiseLevelSlider{"Level"}, noiseColorSlider{"Color"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> playModeAttach, glideAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseLevelAttach, noiseColorAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSynthV1AudioProcessorEditor)
};
