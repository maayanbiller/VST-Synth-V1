#pragma once
#include <JuceHeader.h>

class SynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SynthLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8dcca));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        setColour(juce::GroupComponent::outlineColourId, juce::Colour(0x88221100));
        setColour(juce::GroupComponent::textColourId, juce::Colour(0xffe8dcca));
        setColour(juce::Label::textColourId, juce::Colour(0xffe8dcca));

        
        juce::JPEGImageFormat jpegFormat;
        juce::PNGImageFormat pngFormat;

        int bgSize = 0;
        const char* bgData = BinaryData::getNamedResource("wood_board_bg_jpg", bgSize);
        if (bgData != nullptr) {
            juce::MemoryInputStream stream(bgData, (size_t)bgSize, false);
            cachedBgImage = jpegFormat.decodeImage(stream);
        }

        int knobSize = 0;
        const char* knobData = BinaryData::getNamedResource("wood_knob_png", knobSize);
        if (knobData != nullptr) {
            juce::MemoryInputStream stream(knobData, (size_t)knobSize, false);
            cachedKnobImage = pngFormat.decodeImage(stream);
        }

        int faderSize = 0;
        const char* faderData = BinaryData::getNamedResource("wood_fader_png", faderSize);
        if (faderData != nullptr) {
            juce::MemoryInputStream stream(faderData, (size_t)faderSize, false);
            cachedFaderImage = pngFormat.decodeImage(stream);
        }

        const char* waveResNames[4] = { "wave_sine_png", "wave_triangle_png", "wave_saw_png", "wave_square_png" };
        for (int i = 0; i < 4; ++i) {
            int waveSize = 0;
            const char* waveData = BinaryData::getNamedResource(waveResNames[i], waveSize);
            if (waveData != nullptr) {
                juce::MemoryInputStream stream(waveData, (size_t)waveSize, false);
                cachedWaveImages[i] = pngFormat.decodeImage(stream);
            }
        }

    }

    juce::Image cachedKnobImage;
    juce::Image cachedBgImage;
    juce::Image cachedFaderImage;
    juce::Image cachedWaveImages[4];

    void drawWoodBackground(juce::Graphics& g, int width, int height)
    {
        if (cachedBgImage.isValid())
            g.drawImage(cachedBgImage, 0, 0, width, height, 0, 0, cachedBgImage.getWidth(), cachedBgImage.getHeight());
        else
            g.fillAll(juce::Colour(0xff2b1d14)); 
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        bool isWave = slider.getName().containsIgnoreCase("Wave");
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        float minDim = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto radius = isWave ? (minDim * 0.28f)
                             : (minDim / 2.0f);
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;

        // Draw shadow
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse(rx + 2.0f, ry + 4.0f, rw, rw);

        if (cachedKnobImage.isValid())
        {
            g.addTransform(juce::AffineTransform::rotation(rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle), centreX, centreY));
            g.drawImageWithin(cachedKnobImage, rx, ry, rw, rw, juce::RectanglePlacement::centred);
            g.addTransform(juce::AffineTransform::rotation(-(rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle)), centreX, centreY));
        }
        else
        {
            // Sleek metallic knob
            juce::ColourGradient grad(juce::Colour(0xff444444), centreX, ry, juce::Colour(0xff111111), centreX, ry + rw, false);
            g.setGradientFill(grad);
            g.fillEllipse(rx, ry, rw, rw);
            
            // Inner rim
            g.setColour(juce::Colour(0xff666666));
            g.drawEllipse(rx+1, ry+1, rw-2, rw-2, 1.5f);
            
            // Indicator
            g.setColour(juce::Colour(0xffe8dcca));
            juce::Path p;
            auto pointerLength = radius * 0.7f;
            p.addRectangle(-1.5f, -radius + 4.0f, 3.0f, pointerLength);
            p.applyTransform(juce::AffineTransform::rotation(rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle)).translated(centreX, centreY));
            g.fillPath(p);
        }

        // Draw clean silkscreen waveform icons around knob without circular bezels
        if (isWave)
        {
            float r2 = radius + minDim * 0.15f;
            float iconSize = std::clamp(minDim * 0.18f, 15.0f, 20.0f);
            for (int i = 0; i < 4; ++i) {
                float a = rotaryStartAngle + (i / 3.0f) * (rotaryEndAngle - rotaryStartAngle);
                float px = centreX + r2 * std::sin(a);
                float py = centreY - r2 * std::cos(a);
                bool isSelected = (std::round(slider.getValue()) == i);

                auto iconRect = juce::Rectangle<float>(px - iconSize * 0.5f, py - iconSize * 0.5f, iconSize, iconSize);

                if (isSelected)
                {
                    // Warm golden halo glow around active waveform symbol
                    g.setColour(juce::Colour(0xffffaa22).withAlpha(0.35f));
                    g.fillEllipse(iconRect.expanded(4.0f));

                    // Subtle illuminated radial pointer tick
                    g.setColour(juce::Colour(0xffffcc44));
                    float t1 = radius + 2.0f;
                    float t2 = radius + 6.0f;
                    g.drawLine(centreX + t1 * std::sin(a), centreY - t1 * std::cos(a),
                               centreX + t2 * std::sin(a), centreY - t2 * std::cos(a), 2.0f);
                }

                if (cachedWaveImages[i].isValid())
                {
                    g.setOpacity(isSelected ? 1.0f : 0.40f);
                    g.drawImageWithin(cachedWaveImages[i], iconRect.getX(), iconRect.getY(), iconRect.getWidth(), iconRect.getHeight(), juce::RectanglePlacement::centred);
                    g.setOpacity(1.0f);
                }
                else
                {
                    // Fallback vector drawing
                    juce::Path icon;
                    if (i == 0) { // Sine
                        icon.startNewSubPath(px - 8, py);
                        icon.quadraticTo(px - 4.0f, py - 6, px, py);
                        icon.quadraticTo(px + 4.0f, py + 6, px + 8, py);
                    } else if (i == 1) { // Triangle
                        icon.startNewSubPath(px - 8, py + 5);
                        icon.lineTo(px, py - 6);
                        icon.lineTo(px + 8, py + 5);
                    } else if (i == 2) { // Saw
                        icon.startNewSubPath(px - 7, py + 5);
                        icon.lineTo(px + 7, py - 6);
                        icon.lineTo(px + 7, py + 5);
                    } else if (i == 3) { // Square
                        icon.startNewSubPath(px - 7, py + 5);
                        icon.lineTo(px - 7, py - 5);
                        icon.lineTo(px + 7, py - 5);
                        icon.lineTo(px + 7, py + 5);
                    }
                    g.setColour(isSelected ? juce::Colour(0xffffcc44) : juce::Colour(0xffe8dcca).withAlpha(0.40f));
                    g.strokePath(icon, juce::PathStrokeType(isSelected ? 2.2f : 1.4f, juce::PathStrokeType::curved));
                }
            }
        }
    }
    
    
    
    void drawGroupComponentOutline (juce::Graphics& g, int width, int height,
                                    const juce::String& text, const juce::Justification& position,
                                    juce::GroupComponent& group) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
        
        // Recessed faceplate: smoked anodized metal / dark plate
        juce::ColourGradient grad(juce::Colours::black.withAlpha(0.25f), bounds.getX(), bounds.getY(),
                                  juce::Colours::black.withAlpha(0.42f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Inset bevel and border
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 5.0f, 1.0f);

        // Header plate for title
        if (text.isNotEmpty())
        {
            auto titleArea = bounds.removeFromTop(20.0f);
            g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            g.setColour(juce::Colour(0xffd8cbba));
            g.drawText(text, titleArea, juce::Justification::centred, false);
            
            // Subtle engraved hairline under title
            g.setColour(juce::Colours::white.withAlpha(0.06f));
            g.drawLine(titleArea.getX() + 20.0f, titleArea.getBottom(), titleArea.getRight() - 20.0f, titleArea.getBottom(), 1.0f);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        bool isToggle = button.getToggleState();
        
        juce::ColourGradient grad((isToggle ? juce::Colour(0xff40e0d0).withAlpha(0.85f) : juce::Colours::white.withAlpha(0.28f)), 
                                  bounds.getX(), bounds.getY(), 
                                  (isToggle ? juce::Colour(0xff20a090).withAlpha(0.7f) : juce::Colours::white.withAlpha(0.10f)), 
                                  bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, 4.0f);

        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.fillRoundedRectangle(bounds, 4.0f);
        }

        if (isToggle) {
            g.setColour(juce::Colour(0xffe8dcca).withAlpha(0.9f));
            g.drawRoundedRectangle(bounds, 4.0f, 1.5f);
            g.setColour(juce::Colour(0xffe8dcca).withAlpha(0.4f));
            g.drawRoundedRectangle(bounds.expanded(1.0f), 5.0f, 2.0f);
        } else {
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::Font font(juce::jmin(14.0f, button.getHeight() * 0.6f));
        g.setFont (font);
        
        bool isToggle = button.getToggleState();
        g.setColour(isToggle ? juce::Colour(0xffffffff) : juce::Colour(0xffeeeeee));

        g.drawFittedText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        
        juce::Rectangle<float> track(bounds.getCentreX() - 3.0f, bounds.getY(), 6.0f, bounds.getHeight());
        g.setColour(juce::Colour(0xff111111));
        g.fillRoundedRectangle(track, 3.0f);
        
        
        juce::Rectangle<float> thumb(bounds.getCentreX() - 14.0f, sliderPos - 10.0f, 28.0f, 20.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(thumb.translated(0.0f, 4.0f), 3.0f); // shadow
        
        if (cachedFaderImage.isValid())
        {
            g.drawImageWithin(cachedFaderImage, thumb.getX(), thumb.getY(), thumb.getWidth(), thumb.getHeight(), juce::RectanglePlacement::centred);
        }
        else
        {
            juce::ColourGradient grad2(juce::Colour(0xff333333), thumb.getX(), thumb.getY(), juce::Colour(0xff1a1a1a), thumb.getX(), thumb.getBottom(), false);
            g.setGradientFill(grad2);
            g.fillRoundedRectangle(thumb, 3.0f);
            g.setColour(juce::Colour(0xff444444));
            g.drawRoundedRectangle(thumb, 3.0f, 1.0f);
            
            g.setColour(juce::Colours::white);
            g.fillRect(thumb.getCentreX() - 8.0f, thumb.getCentreY() - 1.0f, 16.0f, 2.0f);
        }
    }
};


