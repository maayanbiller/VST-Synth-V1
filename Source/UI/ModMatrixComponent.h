#pragma once
#include <JuceHeader.h>
#include "../DSP/SynthParameters.h"
#include "LabeledSlider.h"

class MatrixCellButton : public juce::Component
{
public:
    MatrixCellButton(juce::AudioProcessorValueTreeState& vts, const juce::String& paramId)
        : apvts(vts), param(paramId)
    {
        setRepaintsOnMouseActivity(true);
    }

    void setState(int newState)
    {
        if (state != newState)
        {
            state = std::clamp(newState, 0, 5);
            repaint();
        }
    }

    int getState() const { return state; }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            state = 0; // Clear connection
        }
        else
        {
            state = (state + 1) % 6; // Cycle Off -> Blue -> Amber -> Green -> Red -> Purple -> Off
        }

        if (auto* p = apvts.getParameter(param))
        {
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(state)));
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float radius = 4.0f;

        // Color definitions:
        // 0: Frosted semi-transparent white (Off)
        // 1: Direct Blue (fixed connection)
        // 2: Bus 1 Amber/Gold
        // 3: Bus 2 Emerald Green
        // 4: Bus 3 Crimson Red
        // 5: Bus 4 Electric Purple
        static const juce::Colour stateColours[6] = {
            juce::Colours::white.withAlpha(0.22f),       // 0: Frosted Off
            juce::Colour(0xff29b6f6),                   // 1: Electric Blue
            juce::Colour(0xffffaa22),                   // 2: Amber
            juce::Colour(0xff22dd66),                   // 3: Green
            juce::Colour(0xffff3344),                   // 4: Red
            juce::Colour(0xffbb44ff)                    // 5: Purple
        };

        juce::Colour baseCol = stateColours[state];

        if (state == 0)
        {
            // Clean frosted semi-transparent button
            juce::ColourGradient grad(juce::Colours::white.withAlpha(isMouseOver() ? 0.35f : 0.22f),
                                      bounds.getX(), bounds.getY(),
                                      juce::Colours::white.withAlpha(isMouseOver() ? 0.18f : 0.08f),
                                      bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, radius);

            // Subtle glass border
            g.setColour(juce::Colours::white.withAlpha(isMouseOver() ? 0.6f : 0.35f));
            g.drawRoundedRectangle(bounds, radius, 1.0f);
        }
        else
        {
            // Illuminated vintage button with outer glow halo
            g.setColour(baseCol.withAlpha(0.40f));
            g.drawRoundedRectangle(bounds.expanded(1.5f), radius + 1.0f, 2.0f);

            // Button body
            juce::ColourGradient grad(baseCol.withAlpha(0.95f),
                                      bounds.getX(), bounds.getY(),
                                      baseCol.darker(0.3f).withAlpha(0.85f),
                                      bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, radius);

            // Top highlight reflection
            auto topHighlight = bounds.removeFromTop(bounds.getHeight() * 0.45f).reduced(1.0f);
            g.setColour(juce::Colours::white.withAlpha(0.45f));
            g.fillRoundedRectangle(topHighlight, radius * 0.7f);

            // Crisp illuminated border
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.drawRoundedRectangle(bounds, radius, 1.2f);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String param;
    int state = 0;
};

class VintageModMatrixComponent : public juce::Component, public juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int numRows = 5;
    static constexpr int numCols = 8;

    VintageModMatrixComponent(juce::AudioProcessorValueTreeState& vts, SynthLookAndFeel& laf)
        : apvts(vts), customLF(laf),
          bus1Slider("", juce::Slider::RotaryHorizontalVerticalDrag, true),
          bus2Slider("", juce::Slider::RotaryHorizontalVerticalDrag, true),
          bus3Slider("", juce::Slider::RotaryHorizontalVerticalDrag, true),
          bus4Slider("", juce::Slider::RotaryHorizontalVerticalDrag, true)
    {
        // Setup 5x8 matrix grid
        for (int r = 0; r < numRows; ++r)
        {
            for (int c = 0; c < numCols; ++c)
            {
                juce::String paramId = IDs::getModCellId(r, c);
                auto* btn = new MatrixCellButton(apvts, paramId);
                cells.add(btn);
                addAndMakeVisible(btn);
                apvts.addParameterListener(paramId, this);

                if (auto* p = apvts.getRawParameterValue(paramId))
                    btn->setState(static_cast<int>(*p));
            }
        }

        // Setup 4 Side Bus Knobs
        auto setupBusSlider = [this](LabeledSlider& s, const juce::String& paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach) {
            s.slider.setLookAndFeel(&customLF);
            attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, s.slider);
            if (auto* param = apvts.getParameter(paramId))
            {
                s.slider.textFromValueFunction = [param](double v) {
                    return param->getText(param->convertTo0to1((float)v), 0);
                };
                s.slider.valueFromTextFunction = [param](const juce::String& text) {
                    return (double)param->convertFrom0to1(param->getValueForText(text));
                };
            }
            addAndMakeVisible(s);
        };

        setupBusSlider(bus1Slider, IDs::MOD_BUS_1_AMT, bus1Attach);
        setupBusSlider(bus2Slider, IDs::MOD_BUS_2_AMT, bus2Attach);
        setupBusSlider(bus3Slider, IDs::MOD_BUS_3_AMT, bus3Attach);
        setupBusSlider(bus4Slider, IDs::MOD_BUS_4_AMT, bus4Attach);
    }

    ~VintageModMatrixComponent() override
    {
        for (int r = 0; r < numRows; ++r)
        {
            for (int c = 0; c < numCols; ++c)
            {
                apvts.removeParameterListener(IDs::getModCellId(r, c), this);
            }
        }
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        for (int r = 0; r < numRows; ++r)
        {
            for (int c = 0; c < numCols; ++c)
            {
                if (parameterID == IDs::getModCellId(r, c))
                {
                    int idx = r * numCols + c;
                    int intVal = static_cast<int>(std::round(newValue));
                    juce::MessageManager::callAsync([this, idx, intVal]() {
                        if (cells[idx] != nullptr)
                            cells[idx]->setState(intVal);
                    });
                    return;
                }
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        // Vintage plate background
        auto area = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(juce::Colour(0x28000000));
        g.fillRoundedRectangle(area, 6.0f);
        g.setColour(juce::Colour(0x44ffffff));
        g.drawRoundedRectangle(area, 6.0f, 1.0f);

        // Header titles
        g.setFont(juce::Font(10.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xffe8dcca));

        static const juce::String colTitles[8] = {
            "PITCH", "CUTOFF", "RES", "DRIVE", "PWM", "AMP", "OSC 1", "OSC 2"
        };
        static const juce::String rowTitles[5] = {
            "LFO 1", "LFO 2", "MOD ENV", "VEL", "WHEEL"
        };

        // Draw Column headers above the grid
        for (int c = 0; c < numCols; ++c)
        {
            if (auto* btn = cells[c])
            {
                auto b = btn->getBounds();
                juce::Rectangle<int> headerRect(b.getX() - 4, b.getY() - 18, b.getWidth() + 8, 16);
                g.drawFittedText(colTitles[c], headerRect, juce::Justification::centred, 1);
            }
        }

        // Draw Row headers to the left of the grid
        for (int r = 0; r < numRows; ++r)
        {
            if (auto* btn = cells[r * numCols])
            {
                auto b = btn->getBounds();
                juce::Rectangle<int> labelRect(4, b.getY() + (b.getHeight() - 16) / 2, b.getX() - 8, 16);
                g.drawFittedText(rowTitles[r], labelRect, juce::Justification::centredRight, 1);
            }
        }

        // Draw Bus section title and LED indicators
        auto busHeaderArea = juce::Rectangle<int>(busArea.getX(), 8, busArea.getWidth(), 16);
        g.drawFittedText("BUS AMOUNTS", busHeaderArea, juce::Justification::centred, 1);

        // Draw colored LEDs next to the 4 bus knobs
        static const juce::Colour busColors[4] = {
            juce::Colour(0xffffaa22), // Amber
            juce::Colour(0xff22dd66), // Green
            juce::Colour(0xffff3344), // Red
            juce::Colour(0xffbb44ff)  // Purple
        };
        static const juce::String busNames[4] = { "BUS 1", "BUS 2", "BUS 3", "BUS 4" };

        LabeledSlider* busSliders[4] = { &bus1Slider, &bus2Slider, &bus3Slider, &bus4Slider };
        for (int i = 0; i < 4; ++i)
        {
            auto sb = busSliders[i]->getBounds();
            // Position of LED above the knob
            float ledX = sb.getCentreX() - 20.0f;
            float ledY = sb.getY() - 14.0f;
            float ledD = 8.0f;

            // Halo glow
            g.setColour(busColors[i].withAlpha(0.45f));
            g.fillEllipse(ledX - 3.0f, ledY - 3.0f, ledD + 6.0f, ledD + 6.0f);

            // LED lens
            g.setColour(busColors[i]);
            g.fillEllipse(ledX, ledY, ledD, ledD);

            // Reflection dot
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.fillEllipse(ledX + 2.0f, ledY + 1.5f, 2.5f, 2.5f);

            // Label text next to LED
            g.setFont(juce::Font(9.5f, juce::Font::bold));
            g.setColour(busColors[i]);
            juce::Rectangle<int> nameRect(ledX + 12.0f, ledY - 2.0f, 40, 12);
            g.drawFittedText(busNames[i], nameRect, juce::Justification::centredLeft, 1);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(8);

        // Right side is Bus Knobs
        busArea = bounds.removeFromRight(230);

        // Left side is Matrix Grid
        int leftHeaderWidth = 62;
        bounds.removeFromLeft(leftHeaderWidth);
        bounds.removeFromTop(20); // space for column headers

        int colW = bounds.getWidth() / numCols;
        int rowH = bounds.getHeight() / numRows;

        int btnSize = std::min(colW, rowH) - 4;
        btnSize = std::clamp(btnSize, 24, 34);

        for (int r = 0; r < numRows; ++r)
        {
            for (int c = 0; c < numCols; ++c)
            {
                int x = bounds.getX() + c * colW + (colW - btnSize) / 2;
                int y = bounds.getY() + r * rowH + (rowH - btnSize) / 2;
                cells[r * numCols + c]->setBounds(x, y, btnSize, btnSize);
            }
        }

        // Layout 4 bus knobs in busArea (horizontal bank)
        auto bArea = busArea;
        bArea.removeFromTop(22); // space for "BUS AMOUNTS" title
        bArea.removeFromTop(18); // space for LED indicators and "BUS 1..4"
        int bColW = bArea.getWidth() / 4;
        bus1Slider.setBounds(bArea.removeFromLeft(bColW).reduced(2));
        bus2Slider.setBounds(bArea.removeFromLeft(bColW).reduced(2));
        bus3Slider.setBounds(bArea.removeFromLeft(bColW).reduced(2));
        bus4Slider.setBounds(bArea.reduced(2));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    SynthLookAndFeel& customLF;
    juce::OwnedArray<MatrixCellButton> cells;

    juce::Rectangle<int> busArea;
    LabeledSlider bus1Slider, bus2Slider, bus3Slider, bus4Slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bus1Attach, bus2Attach, bus3Attach, bus4Attach;
};
