/*
  ==============================================================================

    CustomLookAndFeel.h
    Created: 12 Apr 2026 12:33:02pm
    Author:  jimea

  ==============================================================================
*/

#pragma once

class FilmstripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripLookAndFeel(juce::Image strip, int numFrames)
        : knobStrip(strip), frameCount(numFrames)
    {
        // We calculate this once to save processing power later
        frameHeight = knobStrip.getHeight() / frameCount;
        frameWidth = knobStrip.getWidth();
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override
    {
        if (knobStrip.isValid())
        {
            const int diameter = std::min(width, height);
            const int centerX = x + (width - diameter) / 2;
            const int centerY = y + (height - diameter) / 2;

            // Using floor is usually smoother for filmstrips
            const int frameIndex = (int)std::floor(sliderPos * (frameCount - 1));

            // Using the pre-calculated frameHeight from the constructor
            const int sourceY = frameIndex * frameHeight;

            // 3. DRAW
            g.drawImage(knobStrip,
                centerX, centerY, diameter, diameter,   // Destination (Centered Square)
                0, sourceY,                             // Source Top-Left
                frameWidth, frameHeight);               // Source Size (One Frame)
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) override
{
        if (button.getName() == "Active" || button.getName() == "Freeze" || button.getName() == "Shimmer")
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto diameter = std::min(bounds.getWidth(), bounds.getHeight());
        auto circleBounds = bounds.withSizeKeepingCentre(diameter, diameter).reduced(2.0f);

        auto center = circleBounds.getCentre();
        auto radius = diameter / 2.0f;

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillEllipse(circleBounds.translated(0, 2));

        // Metal base
        juce::ColourGradient metalGrad(juce::Colours::grey, center.x, circleBounds.getY(),
            juce::Colours::darkgrey, center.x, circleBounds.getBottom(), false);
        g.setGradientFill(metalGrad);
        g.fillEllipse(circleBounds);

        // Inner Plunger
        auto innerRect = circleBounds.reduced(radius * 0.35f);
        auto plungerBaseCol = button.getToggleState() ? juce::Colours::lightgrey : juce::Colours::grey;
        if (button.isDown()) plungerBaseCol = plungerBaseCol.darker(0.2f);

        g.setColour(plungerBaseCol);
        g.fillEllipse(innerRect);

        // Glossy highlight
        auto highlight = innerRect.reduced(2.0f).removeFromTop(innerRect.getHeight() * 0.4f);
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::white.withAlpha(0.4f), highlight.getY(),
            juce::Colours::white.withAlpha(0.0f), highlight.getBottom()));
        g.fillEllipse(highlight);

        // Dark rim
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawEllipse(circleBounds, 1.0f);
    }
    else
    {
        juce::LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour,
            shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
}

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
        bool isMouseOverButton, bool isButtonDown) override
    {
        // If it's one of the stompboxes, we don't want to draw any text 
        // because we draw the labels manually in the Editor's paint() function.
        if (button.getName() == "Active" ||
            button.getName() == "Freeze" ||
            button.getName() == "Shimmer")
        {
            return;
        }

        // For any other normal buttons, use the default JUCE text drawing
        juce::LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);
    }

private:
    juce::Image knobStrip;
    int frameCount;
    int frameWidth, frameHeight;
};