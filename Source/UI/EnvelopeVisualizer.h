#pragma once
#include <JuceHeader.h>

class EnvelopeVisualizer : public juce::Component, 
                           public juce::AudioProcessorValueTreeState::Listener,
                           public juce::AsyncUpdater
{
public:
    EnvelopeVisualizer(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
    {
        // Cache atomic pointers for lock-free reads in the paint method
        attackParam = apvts.getRawParameterValue("AMP_ATTACK");
        decayParam = apvts.getRawParameterValue("AMP_DECAY");
        sustainParam = apvts.getRawParameterValue("AMP_SUSTAIN");
        releaseParam = apvts.getRawParameterValue("AMP_RELEASE");

        apvts.addParameterListener("AMP_ATTACK", this);
        apvts.addParameterListener("AMP_DECAY", this);
        apvts.addParameterListener("AMP_SUSTAIN", this);
        apvts.addParameterListener("AMP_RELEASE", this);
    }

    ~EnvelopeVisualizer() override
    {
        apvts.removeParameterListener("AMP_ATTACK", this);
        apvts.removeParameterListener("AMP_DECAY", this);
        apvts.removeParameterListener("AMP_SUSTAIN", this);
        apvts.removeParameterListener("AMP_RELEASE", this);
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        juce::ignoreUnused(parameterID, newValue);
        // Allocation-free, lock-free UI trigger (no callAsync risk)
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRoundedRectangle(bounds, 10.0f);

        // Fetch values from cached pointers
        float attack = attackParam != nullptr ? attackParam->load() : 0.1f;
        float decay = decayParam != nullptr ? decayParam->load() : 0.1f;
        float sustain = sustainParam != nullptr ? sustainParam->load() : 1.0f;
        float release = releaseParam != nullptr ? releaseParam->load() : 0.1f;

        // Piecewise mapping: makes short times (e.g. 5ms) visibly wide enough
        auto mapTime = [](float t) {
            if (t <= 0.1f) return juce::jmap(t, 0.0f, 0.1f, 0.0f, 0.15f);
            if (t <= 1.0f) return juce::jmap(t, 0.1f, 1.0f, 0.15f, 0.45f);
            return juce::jmap(t, 1.0f, 5.0f, 0.45f, 1.0f);
        };

        float aScaled = mapTime(attack);
        float dScaled = mapTime(decay);
        float rScaled = mapTime(release);
        float holdPhase = 0.3f;
        float totalTime = aScaled + dScaled + holdPhase + rScaled; 

        // Fix drawing bounds clipping (padding of 10 instead of 5)
        float widthMap = bounds.getWidth() - 10.0f;
        float attackX = (aScaled / totalTime) * widthMap;
        float decayX = attackX + ((dScaled / totalTime) * widthMap);
        float holdX = decayX + ((holdPhase / totalTime) * widthMap);
        float releaseX = holdX + ((rScaled / totalTime) * widthMap);

        float bottomY = bounds.getHeight() - 5.0f;
        float topY = 5.0f;
        float sustainY = bottomY - (sustain * (bottomY - topY));

        // Draw Envelope Path
        juce::Path p;
        p.startNewSubPath(5.0f, bottomY); // Start
        p.lineTo(5.0f + attackX, topY); // Attack
        p.lineTo(5.0f + decayX, sustainY); // Decay
        p.lineTo(5.0f + holdX, sustainY); // Sustain hold
        p.lineTo(5.0f + releaseX, bottomY); // Release

        g.setColour(juce::Colours::cyan);
        g.strokePath(p, juce::PathStrokeType(2.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

        // Draw fill under path
        juce::Path fillPath = p;
        fillPath.lineTo(5.0f + releaseX, bottomY);
        fillPath.lineTo(5.0f, bottomY);
        fillPath.closeSubPath();
        
        g.setGradientFill(juce::ColourGradient(juce::Colours::cyan.withAlpha(0.4f), 0, topY, juce::Colours::cyan.withAlpha(0.0f), 0, bottomY, false));
        g.fillPath(fillPath);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
};
