#include "PluginEditor.h"

VSTSynthV1AudioProcessorEditor::VSTSynthV1AudioProcessorEditor (VSTSynthV1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1000, 600);

    addAndMakeVisible(osc1Group);
    addAndMakeVisible(osc2Group);
    addAndMakeVisible(filterGroup);
    addAndMakeVisible(envGroup);
    addAndMakeVisible(lfoGroup);

    addAndMakeVisible(osc1WaveCombo);
    osc1WaveComboAtt = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, IDs::OSC1_WAVE, osc1WaveCombo);

    osc1SemiSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc1SemiSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc1SemiSlider);
    osc1SemiSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC1_SEMI, osc1SemiSlider);

    osc1DetuneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc1DetuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc1DetuneSlider);
    osc1DetuneSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC1_DETUNE, osc1DetuneSlider);

    osc1LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc1LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc1LevelSlider);
    osc1LevelSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC1_LEVEL, osc1LevelSlider);

    osc1PwmSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc1PwmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc1PwmSlider);
    osc1PwmSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC1_PWM, osc1PwmSlider);

    addAndMakeVisible(osc2WaveCombo);
    osc2WaveComboAtt = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, IDs::OSC2_WAVE, osc2WaveCombo);

    osc2SemiSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc2SemiSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc2SemiSlider);
    osc2SemiSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC2_SEMI, osc2SemiSlider);

    osc2DetuneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc2DetuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc2DetuneSlider);
    osc2DetuneSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC2_DETUNE, osc2DetuneSlider);

    osc2LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc2LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc2LevelSlider);
    osc2LevelSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC2_LEVEL, osc2LevelSlider);

    osc2PwmSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    osc2PwmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(osc2PwmSlider);
    osc2PwmSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::OSC2_PWM, osc2PwmSlider);

    addAndMakeVisible(subWaveCombo);
    subWaveComboAtt = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, IDs::SUB_WAVE, subWaveCombo);

    subOctSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    subOctSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(subOctSlider);
    subOctSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::SUB_OCT, subOctSlider);

    subLevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    subLevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(subLevelSlider);
    subLevelSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::SUB_LEVEL, subLevelSlider);

    noiseLevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    noiseLevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(noiseLevelSlider);
    noiseLevelSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::NOISE_LEVEL, noiseLevelSlider);

    addAndMakeVisible(filterTypeCombo);
    filterTypeComboAtt = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, IDs::FILTER_TYPE, filterTypeCombo);

    filterCutoffSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterCutoffSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filterCutoffSlider);
    filterCutoffSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILTER_CUTOFF, filterCutoffSlider);

    filterResSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterResSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filterResSlider);
    filterResSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILTER_RES, filterResSlider);

    filterDriveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterDriveSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filterDriveSlider);
    filterDriveSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILTER_DRIVE, filterDriveSlider);

    filterEnvAmtSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterEnvAmtSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filterEnvAmtSlider);
    filterEnvAmtSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILTER_ENV_AMT, filterEnvAmtSlider);

    ampAttackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ampAttackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(ampAttackSlider);
    ampAttackSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::AMP_ATTACK, ampAttackSlider);

    ampDecaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ampDecaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(ampDecaySlider);
    ampDecaySliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::AMP_DECAY, ampDecaySlider);

    ampSustainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ampSustainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(ampSustainSlider);
    ampSustainSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::AMP_SUSTAIN, ampSustainSlider);

    ampReleaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ampReleaseSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(ampReleaseSlider);
    ampReleaseSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::AMP_RELEASE, ampReleaseSlider);

    filtAttackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filtAttackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filtAttackSlider);
    filtAttackSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILT_ATTACK, filtAttackSlider);

    filtDecaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filtDecaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filtDecaySlider);
    filtDecaySliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILT_DECAY, filtDecaySlider);

    filtSustainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filtSustainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filtSustainSlider);
    filtSustainSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILT_SUSTAIN, filtSustainSlider);

    filtReleaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filtReleaseSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(filtReleaseSlider);
    filtReleaseSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::FILT_RELEASE, filtReleaseSlider);

    addAndMakeVisible(lfo1WaveCombo);
    lfo1WaveComboAtt = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, IDs::LFO1_WAVE, lfo1WaveCombo);

    lfo1RateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    lfo1RateSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(lfo1RateSlider);
    lfo1RateSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::LFO1_RATE, lfo1RateSlider);

    lfo1DepthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    lfo1DepthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(lfo1DepthSlider);
    lfo1DepthSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::LFO1_DEPTH, lfo1DepthSlider);

    masterVolSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterVolSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(masterVolSlider);
    masterVolSliderAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, IDs::MASTER_VOL, masterVolSlider);

}

VSTSynthV1AudioProcessorEditor::~VSTSynthV1AudioProcessorEditor()
{
}

void VSTSynthV1AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(30, 30, 30));
    g.setColour (juce::Colours::white);
    
    // Draw some basic labels manually
    g.drawText("Master Vol", masterVolSlider.getBounds().translated(0, -20), juce::Justification::centred);
}

void VSTSynthV1AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    auto topRow = bounds.removeFromTop(200);
    osc1Group.setBounds(topRow.removeFromLeft(200));
    osc2Group.setBounds(topRow.removeFromLeft(200));
    
    // Quick and dirty layout just to expose all params
    int x = 20;
    int y = 30;
    
    osc1WaveCombo.setBounds(x, y, 80, 20); y+=30;
    osc1SemiSlider.setBounds(x, y, 40, 40); x+=50;
    osc1DetuneSlider.setBounds(x, y, 40, 40); x+=50;
    osc1LevelSlider.setBounds(x, y, 40, 40); x+=50;
    
    x = 220; y = 30;
    osc2WaveCombo.setBounds(x, y, 80, 20); y+=30;
    osc2SemiSlider.setBounds(x, y, 40, 40); x+=50;
    osc2DetuneSlider.setBounds(x, y, 40, 40); x+=50;
    osc2LevelSlider.setBounds(x, y, 40, 40); x+=50;
    
    auto midRow = bounds.removeFromTop(150);
    filterGroup.setBounds(midRow.removeFromLeft(300));
    lfoGroup.setBounds(midRow.removeFromLeft(300));
    
    x = 20; y = 220;
    filterTypeCombo.setBounds(x, y, 80, 20); y+=30;
    filterCutoffSlider.setBounds(x, y, 50, 50); x+=60;
    filterResSlider.setBounds(x, y, 50, 50); x+=60;
    filterDriveSlider.setBounds(x, y, 50, 50); x+=60;
    filterEnvAmtSlider.setBounds(x, y, 50, 50); x+=60;

    x = 320; y = 220;
    lfo1WaveCombo.setBounds(x, y, 80, 20); y+=30;
    lfo1RateSlider.setBounds(x, y, 40, 40); x+=50;
    lfo1DepthSlider.setBounds(x, y, 40, 40); x+=50;

    auto botRow = bounds.removeFromTop(150);
    envGroup.setBounds(botRow.removeFromLeft(400));
    
    x = 20; y = 380;
    ampAttackSlider.setBounds(x, y, 40, 40); x+=50;
    ampDecaySlider.setBounds(x, y, 40, 40); x+=50;
    ampSustainSlider.setBounds(x, y, 40, 40); x+=50;
    ampReleaseSlider.setBounds(x, y, 40, 40); x+=50;

    filtAttackSlider.setBounds(x, y, 40, 40); x+=50;
    filtDecaySlider.setBounds(x, y, 40, 40); x+=50;
    filtSustainSlider.setBounds(x, y, 40, 40); x+=50;
    filtReleaseSlider.setBounds(x, y, 40, 40); x+=50;

    masterVolSlider.setBounds(850, 500, 80, 80);
}
