#include "PluginEditor.h"

VSTSynthV1AudioProcessorEditor::VSTSynthV1AudioProcessorEditor (VSTSynthV1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);
    setLookAndFeel(&customLookAndFeel);
    
    auto setupLabeled = [this](LabeledSlider& ls, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach)
    {
        addAndMakeVisible(ls);
        if (paramID.isNotEmpty()) {
            attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramID, ls.slider);
            if (auto* param = audioProcessor.apvts.getParameter(paramID))
            {
                ls.slider.textFromValueFunction = [param](double v) {
                    return param->getText(param->convertTo0to1((float)v), 0);
                };
                ls.slider.valueFromTextFunction = [param](const juce::String& text) {
                    return (double)param->convertFrom0to1(param->getValueForText(text));
                };
            }
            else
            {
                ls.slider.textFromValueFunction = [](double v) { 
                    if (v >= 10.0 || std::abs(v - std::round(v)) < 0.001) return juce::String((int)std::round(v)); 
                    return juce::String(v, 2); 
                };
            }
        }
    };

    addAndMakeVisible(osc1Group); addAndMakeVisible(osc2Group); addAndMakeVisible(osc3Group); addAndMakeVisible(osc4Group);
    addAndMakeVisible(mixerGroup); addAndMakeVisible(filterGroup); addAndMakeVisible(envGroup); addAndMakeVisible(lfoGroup);

    setupLabeled(osc1WaveSlider, IDs::OSC1_WAVE, osc1WaveAttach);
    setupLabeled(osc1OctSlider, IDs::OSC1_OCTAVE, osc1OctAttach);
    setupLabeled(osc1DetuneSlider, IDs::OSC1_DETUNE, osc1DetuneAttach);
    setupLabeled(osc1PwmSlider, IDs::OSC1_PWM, osc1PwmAttach);
    setupLabeled(osc1VoicesSlider, IDs::OSC1_VOICES, osc1VoicesAttach);
    setupLabeled(osc1WidthSlider, IDs::OSC1_WIDTH, osc1WidthAttach);

    setupLabeled(osc2WaveSlider, IDs::OSC2_WAVE, osc2WaveAttach);
    setupLabeled(osc2OctSlider, IDs::OSC2_OCTAVE, osc2OctAttach);
    setupLabeled(osc2DetuneSlider, IDs::OSC2_DETUNE, osc2DetuneAttach);
    setupLabeled(osc2PwmSlider, IDs::OSC2_PWM, osc2PwmAttach);
    setupLabeled(osc2VoicesSlider, IDs::OSC2_VOICES, osc2VoicesAttach);
    setupLabeled(osc2WidthSlider, IDs::OSC2_WIDTH, osc2WidthAttach);

    setupLabeled(osc3WaveSlider, IDs::OSC3_WAVE, osc3WaveAttach);
    setupLabeled(osc3OctSlider, IDs::OSC3_OCTAVE, osc3OctAttach);
    setupLabeled(osc3DetuneSlider, IDs::OSC3_DETUNE, osc3DetuneAttach);
    setupLabeled(osc3PwmSlider, IDs::OSC3_PWM, osc3PwmAttach);
    setupLabeled(osc3VoicesSlider, IDs::OSC3_VOICES, osc3VoicesAttach);
    setupLabeled(osc3WidthSlider, IDs::OSC3_WIDTH, osc3WidthAttach);

    setupLabeled(osc4WaveSlider, IDs::OSC4_WAVE, osc4WaveAttach);
    setupLabeled(osc4OctSlider, IDs::OSC4_OCTAVE, osc4OctAttach);
    setupLabeled(osc4DetuneSlider, IDs::OSC4_DETUNE, osc4DetuneAttach);
    setupLabeled(osc4PwmSlider, IDs::OSC4_PWM, osc4PwmAttach);
    setupLabeled(osc4VoicesSlider, IDs::OSC4_VOICES, osc4VoicesAttach);
    setupLabeled(osc4WidthSlider, IDs::OSC4_WIDTH, osc4WidthAttach);

    setupLabeled(osc1LevelSlider, IDs::OSC1_LEVEL, osc1LevelAttach);
    setupLabeled(osc2LevelSlider, IDs::OSC2_LEVEL, osc2LevelAttach);
    setupLabeled(osc3LevelSlider, IDs::OSC3_LEVEL, osc3LevelAttach);
    setupLabeled(osc4LevelSlider, IDs::OSC4_LEVEL, osc4LevelAttach);
    setupLabeled(noiseLevelSlider, IDs::NOISE_LEVEL, noiseLevelAttach);
    setupLabeled(masterVolSlider, IDs::MASTER_VOL, masterVolAttach);

    setupLabeled(filterCutoffSlider, IDs::FILTER_CUTOFF, filterCutoffAttach);
    setupLabeled(filterResSlider, IDs::FILTER_RES, filterResAttach);
    setupLabeled(filterDriveSlider, IDs::FILTER_DRIVE, filterDriveAttach);
    setupLabeled(filterEnvAmtSlider, IDs::FILTER_ENV_AMT, filterEnvAmtAttach);

    setupLabeled(ampAttackSlider, IDs::AMP_ATTACK, ampAttackAttach);
    setupLabeled(ampDecaySlider, IDs::AMP_DECAY, ampDecayAttach);
    setupLabeled(ampSustainSlider, IDs::AMP_SUSTAIN, ampSustainAttach);
    setupLabeled(ampReleaseSlider, IDs::AMP_RELEASE, ampReleaseAttach);

    setupLabeled(filtAttackSlider, IDs::FILT_ATTACK, filtAttackAttach);
    setupLabeled(filtDecaySlider, IDs::FILT_DECAY, filtDecayAttach);
    setupLabeled(filtSustainSlider, IDs::FILT_SUSTAIN, filtSustainAttach);
    setupLabeled(filtReleaseSlider, IDs::FILT_RELEASE, filtReleaseAttach);

    setupLabeled(modAttackSlider, IDs::MOD_ATTACK, modAttackAttach);
    setupLabeled(modDecaySlider, IDs::MOD_DECAY, modDecayAttach);
    setupLabeled(modSustainSlider, IDs::MOD_SUSTAIN, modSustainAttach);
    setupLabeled(modReleaseSlider, IDs::MOD_RELEASE, modReleaseAttach);
    
    addAndMakeVisible(ampLabel); ampLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(filtLabel); filtLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modLabel); modLabel.setJustificationType(juce::Justification::centred);

    setupLabeled(lfo1WaveSlider, IDs::LFO1_WAVE, lfo1WaveAttach);
    setupLabeled(lfo1RateSlider, IDs::LFO1_RATE, lfo1RateAttach);
    setupLabeled(lfo1DepthSlider, IDs::LFO1_DEPTH, lfo1DepthAttach);

    setupLabeled(lfo2WaveSlider, IDs::LFO2_WAVE, lfo2WaveAttach);
    setupLabeled(lfo2RateSlider, IDs::LFO2_RATE, lfo2RateAttach);
    setupLabeled(lfo2DepthSlider, IDs::LFO2_DEPTH, lfo2DepthAttach);
    
    // Waveform labels are suppressed since we draw icons around them
    osc1WaveSlider.showTextBox = false; osc1WaveSlider.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    osc2WaveSlider.showTextBox = false; osc2WaveSlider.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    osc3WaveSlider.showTextBox = false; osc3WaveSlider.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    osc4WaveSlider.showTextBox = false; osc4WaveSlider.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfo1WaveSlider.showTextBox = false; lfo1WaveSlider.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfo2WaveSlider.showTextBox = false; lfo2WaveSlider.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    osc1WaveSlider.label.setText("", juce::dontSendNotification);
    osc2WaveSlider.label.setText("", juce::dontSendNotification);
    osc3WaveSlider.label.setText("", juce::dontSendNotification);
    osc4WaveSlider.label.setText("", juce::dontSendNotification);
    lfo1WaveSlider.label.setText("", juce::dontSendNotification);
    lfo2WaveSlider.label.setText("", juce::dontSendNotification);

    setupLabeled(playModeSlider, IDs::PLAY_MODE, playModeAttach);
    setupLabeled(glideSlider, IDs::GLIDE_TIME, glideAttach);
    setupLabeled(noiseColorSlider, IDs::NOISE_COLOR, noiseColorAttach);

    addAndMakeVisible(modGroup); addAndMakeVisible(playGroup); addAndMakeVisible(noiseGroup);
    playGroup.setLookAndFeel(&customLookAndFeel);
    noiseGroup.setLookAndFeel(&customLookAndFeel);
    modGroup.setLookAndFeel(&customLookAndFeel);
    
    vintageModMatrix = std::make_unique<VintageModMatrixComponent>(audioProcessor.apvts, customLookAndFeel);
    addAndMakeVisible(*vintageModMatrix);

    setSize (1200, 800);
    sendLookAndFeelChange();
}

VSTSynthV1AudioProcessorEditor::~VSTSynthV1AudioProcessorEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void VSTSynthV1AudioProcessorEditor::paint (juce::Graphics& g)
{
    customLookAndFeel.drawWoodBackground(g, getWidth(), getHeight());

    // Silkscreen UNISON headers for each oscillator section
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.setColour(juce::Colour(0xffc2b298).withAlpha(0.65f));

    for (auto* oscGrp : { &osc1Group, &osc2Group, &osc3Group, &osc4Group })
    {
        auto b = oscGrp->getBounds();
        int uniY = b.getY() + 22 + 110 + 66;
        g.drawText("- UNISON -", b.getX(), uniY, b.getWidth(), 14, juce::Justification::centred);
    }

    // Inset sub-panels for Envelopes
    auto envBounds = envGroup.getBounds().reduced(6);
    envBounds.removeFromTop(22);
    int gW = envBounds.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto subPlate = envBounds.removeFromLeft(gW).reduced(4, 2).toFloat();
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(subPlate, 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawRoundedRectangle(subPlate, 4.0f, 1.0f);
    }

    // Silkscreen markings for Noise Color
    auto ncBounds = noiseColorSlider.getBounds();
    int markY = ncBounds.getBottom() + 2;
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xffc2b298).withAlpha(0.70f));
    g.drawText("RED", ncBounds.getX() - 15, markY, 35, 12, juce::Justification::centred);
    g.drawText("VIOLET", ncBounds.getRight() - 20, markY, 40, 12, juce::Justification::centred);
}

void VSTSynthV1AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(15);
    
    auto topArea = area.removeFromTop(300);
    int colWidth = topArea.getWidth() / 5;

    auto osc1Area = topArea.removeFromLeft(colWidth).reduced(5);
    auto osc2Area = topArea.removeFromLeft(colWidth).reduced(5);
    auto osc3Area = topArea.removeFromLeft(colWidth).reduced(5);
    auto osc4Area = topArea.removeFromLeft(colWidth).reduced(5);
    auto mixArea = topArea.reduced(5);

    osc1Group.setBounds(osc1Area);
    osc2Group.setBounds(osc2Area);
    osc3Group.setBounds(osc3Area);
    osc4Group.setBounds(osc4Area);
    mixerGroup.setBounds(mixArea);

    auto layoutOsc = [](juce::Rectangle<int> b, LabeledSlider& w, LabeledSlider& o, LabeledSlider& d, LabeledSlider& p, LabeledSlider& v, LabeledSlider& wd) {
        b.removeFromTop(22); // title
        auto topRow = b.removeFromTop(110);
        w.setBounds(topRow.reduced(4));
        auto midRow = b.removeFromTop(66);
        int mw = midRow.getWidth() / 3;
        o.setBounds(midRow.removeFromLeft(mw).reduced(2));
        d.setBounds(midRow.removeFromLeft(mw).reduced(2));
        p.setBounds(midRow.reduced(2));
        b.removeFromTop(14); // Spacing for - UNISON - header
        auto botRow = b.removeFromTop(62);
        int bw = botRow.getWidth() / 2;
        v.setBounds(botRow.removeFromLeft(bw).reduced(2));
        wd.setBounds(botRow.reduced(2));
    };

    layoutOsc(osc1Area, osc1WaveSlider, osc1OctSlider, osc1DetuneSlider, osc1PwmSlider, osc1VoicesSlider, osc1WidthSlider);
    layoutOsc(osc2Area, osc2WaveSlider, osc2OctSlider, osc2DetuneSlider, osc2PwmSlider, osc2VoicesSlider, osc2WidthSlider);
    layoutOsc(osc3Area, osc3WaveSlider, osc3OctSlider, osc3DetuneSlider, osc3PwmSlider, osc3VoicesSlider, osc3WidthSlider);
    layoutOsc(osc4Area, osc4WaveSlider, osc4OctSlider, osc4DetuneSlider, osc4PwmSlider, osc4VoicesSlider, osc4WidthSlider);

    mixArea.removeFromTop(20);
    int mixW = mixArea.getWidth() / 6;
    osc1LevelSlider.setBounds(mixArea.removeFromLeft(mixW).reduced(2));
    osc2LevelSlider.setBounds(mixArea.removeFromLeft(mixW).reduced(2));
    osc3LevelSlider.setBounds(mixArea.removeFromLeft(mixW).reduced(2));
    osc4LevelSlider.setBounds(mixArea.removeFromLeft(mixW).reduced(2));
    noiseLevelSlider.setBounds(mixArea.removeFromLeft(mixW).reduced(2));
    masterVolSlider.setBounds(mixArea.reduced(2));

    area.removeFromTop(10); // Spacing between top and bottom half

    auto middleRow = area.removeFromTop(230);
    auto bottomRow = area.removeFromTop(230);

    filterGroup.setBounds(middleRow.removeFromLeft(250).reduced(5));
    lfoGroup.setBounds(middleRow.removeFromRight(250).reduced(5));
    envGroup.setBounds(middleRow.reduced(5));

    playGroup.setBounds(bottomRow.removeFromLeft(220).reduced(5));
    noiseGroup.setBounds(bottomRow.removeFromLeft(180).reduced(5));
    modGroup.setBounds(bottomRow.reduced(5));

    auto filtArea = filterGroup.getBounds().reduced(5);
    filtArea.removeFromTop(22);
    auto topFiltRow = filtArea.removeFromTop(filtArea.getHeight() * 0.58f);
    filterCutoffSlider.setBounds(topFiltRow.withSizeKeepingCentre(120, 105));
    auto botFiltRow = filtArea;
    int bw = botFiltRow.getWidth() / 3;
    filterResSlider.setBounds(botFiltRow.removeFromLeft(bw).reduced(2));
    filterDriveSlider.setBounds(botFiltRow.removeFromLeft(bw).reduced(2));
    filterEnvAmtSlider.setBounds(botFiltRow.reduced(2));

    auto lfoA = lfoGroup.getBounds().reduced(5);
    lfoA.removeFromTop(20);
    auto lfo1A = lfoA.removeFromTop(lfoA.getHeight()/2);
    int lW = lfo1A.getWidth() / 3;
    lfo1WaveSlider.setBounds(lfo1A.removeFromLeft(lW).reduced(2));
    lfo1RateSlider.setBounds(lfo1A.removeFromLeft(lW).reduced(2));
    lfo1DepthSlider.setBounds(lfo1A.reduced(2));

    auto lfo2A = lfoA;
    lfo2WaveSlider.setBounds(lfo2A.removeFromLeft(lW).reduced(2));
    lfo2RateSlider.setBounds(lfo2A.removeFromLeft(lW).reduced(2));
    lfo2DepthSlider.setBounds(lfo2A.reduced(2));
    
    // Layout Play - well proportioned secondary controls
    auto pArea = playGroup.getBounds().reduced(5);
    pArea.removeFromTop(22);
    auto pKnobs = pArea.withSizeKeepingCentre(180, 85);
    int pw = pKnobs.getWidth() / 2;
    playModeSlider.setBounds(pKnobs.removeFromLeft(pw).reduced(2));
    glideSlider.setBounds(pKnobs.reduced(2));

    // Layout Noise - centered color control
    auto nArea = noiseGroup.getBounds().reduced(5);
    nArea.removeFromTop(22);
    noiseColorSlider.setBounds(nArea.withSizeKeepingCentre(90, 90));

    // Layout Mod Matrix
    auto mArea = modGroup.getBounds().reduced(5);
    mArea.removeFromTop(20);
    if (vintageModMatrix)
        vintageModMatrix->setBounds(mArea);

    auto envA = envGroup.getBounds().reduced(5);
    envA.removeFromTop(15);
    
    // Add spaces between envelope groups! (Requirement 4)
    int groupWidth = envA.getWidth() / 3;
    
    auto modA = envA.removeFromLeft(groupWidth).reduced(8, 0);
    modLabel.setBounds(modA.removeFromTop(20));
    int mw = modA.getWidth() / 4;
    modAttackSlider.setBounds(modA.removeFromLeft(mw));
    modDecaySlider.setBounds(modA.removeFromLeft(mw));
    modSustainSlider.setBounds(modA.removeFromLeft(mw));
    modReleaseSlider.setBounds(modA);

    auto filtEA = envA.removeFromLeft(groupWidth).reduced(8, 0);
    filtLabel.setBounds(filtEA.removeFromTop(20));
    int fw = filtEA.getWidth() / 4;
    filtAttackSlider.setBounds(filtEA.removeFromLeft(fw));
    filtDecaySlider.setBounds(filtEA.removeFromLeft(fw));
    filtSustainSlider.setBounds(filtEA.removeFromLeft(fw));
    filtReleaseSlider.setBounds(filtEA);

    auto ampA = envA.reduced(8, 0);
    ampLabel.setBounds(ampA.removeFromTop(20));
    int aw = ampA.getWidth() / 4;
    ampAttackSlider.setBounds(ampA.removeFromLeft(aw));
    ampDecaySlider.setBounds(ampA.removeFromLeft(aw));
    ampSustainSlider.setBounds(ampA.removeFromLeft(aw));
    ampReleaseSlider.setBounds(ampA);
}


