/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PedalboardAudioProcessorEditor::PedalboardAudioProcessorEditor (PedalboardAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{

    // Custom Knobs
    auto knobImage = juce::ImageCache::getFromMemory(BinaryData::hise_knob_small_png,
        BinaryData::hise_knob_small_pngSize);
    customLookAndFeel = std::make_unique<FilmstripLookAndFeel>(knobImage, 128);
    juce::LookAndFeel::setDefaultLookAndFeel(customLookAndFeel.get());

    // define sliders
    auto sliders = { &dryWetOctaver, &dryWetDist, &GainDistortion, &distortionLevel, &roomSize,&damping,&width,&dryWetRev };
    auto labels  = { &dryWetOctaverLabel, &dryWetDistLabel, &GainDistortionLabel, &distortionLevelLabel,
        &roomSizeLabel, &dampingLabel, &widthLabel,&dryWetRevLabel};
    juce::StringArray names = { "Dry/Wet", "Dry/Wet", "Gain", "Level", "Room Size", "Damping", "Width", "Dry/Wet"};
    
    for (int i = 0; i < sliders.size(); ++i)
    {
        auto* slider = *(sliders.begin() + i);
        auto* label  = *(labels.begin() + i);
        
        addAndMakeVisible(slider);
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);

        addAndMakeVisible(label);
        label->setText(names[i], juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->attachToComponent(slider, false);
    }
    
    octaverActive.setClickingTogglesState(true); // Makes it stay "on" or "off"
    octaverActive.setButtonText("Active");
    octaverActive.setName("Active");
    addAndMakeVisible(octaverActive);
    octaverActive.onClick = [this] { repaint(); };

    reverbActive.setClickingTogglesState(true); // Makes it stay "on" or "off"
    reverbActive.setButtonText("Active");
    reverbActive.setName("Active");
    addAndMakeVisible(reverbActive);
    reverbActive.onClick = [this] { repaint(); };

    distortionActive.setClickingTogglesState(true); // Makes it stay "on" or "off"
    distortionActive.setButtonText("Active");
    distortionActive.setName("Active");
    addAndMakeVisible(distortionActive);
    distortionActive.onClick = [this] { repaint(); };

    DistortionSelection.addItemList(juce::StringArray {"Hard Clipping", "Soft Clipping", "Hiperbolic Tangent"}, 1);
    DistortionSelection.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(DistortionSelection);

    freezeActive.setClickingTogglesState(true);
    freezeActive.setButtonText("Freeze");
    freezeActive.setName("Freeze");
    addAndMakeVisible(freezeActive);
    freezeActive.onClick = [this] { repaint(); };

    shimmerActive.setClickingTogglesState(true);
    shimmerActive.setButtonText("Shimmer");
    shimmerActive.setName("Shimmer");
    addAndMakeVisible(shimmerActive);
    shimmerActive.onClick = [this] { repaint(); };

    //Attachments
    dryWetOctaverAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "dryWetOctaver", dryWetOctaver);
    octaverActiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "octaverActive", octaverActive);
    dryWetDistAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "dryWetDist", dryWetDist);
    GainDistortionAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "GainDistortion", GainDistortion);
    distortionLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "distortionLevel", distortionLevel);
    DistortionSelectionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "DistortionSelection", DistortionSelection);
    distortionActiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "distortionActive", distortionActive);
    reverbActiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "reverbActive", reverbActive);
    roomSizeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "roomSize", roomSize);
    dampingAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "damping",damping);
    dryWetRevAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "dryWetRev", dryWetRev);
    widthAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "width", width);
    freezeActiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "freezeActive", freezeActive);
    shimmerActiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "shimmerActive", shimmerActive);

    setSize (750, 500);
}

void PedalboardAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds();

    // Main Title
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("PEDALBOARD", bounds.removeFromTop(40), juce::Justification::centred);

    // --- DRAW OCTAVER PEDAL ---
    auto octColor = juce::Colour(0xff4b0082); // Indigo
    g.setColour(octColor);
    g.fillRoundedRectangle(octaverBox.toFloat(), 10.0f);
    g.setColour(juce::Colours::violet.withAlpha(0.7f));
    g.drawRoundedRectangle(octaverBox.toFloat(), 10.0f, 2.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("OCTAVE DOWN", octaverBox.withHeight(30), juce::Justification::centred);

    // LED for Octaver
    auto octLedArea = octaverActive.getBounds().translated(0, -22).withSizeKeepingCentre(12, 12);
    if (octaverActive.getToggleState()) {
        g.setColour(juce::Colours::violet.withAlpha(0.4f));
        g.fillEllipse(octLedArea.toFloat().expanded(5.0f));
        g.setColour(juce::Colours::violet);
    }
    else {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
    }
    g.fillEllipse(octLedArea.toFloat());

    // --- DRAW DISTORTION PEDAL ---
    auto distColor = juce::Colour(0xff5c1d1d); // Deep Red
    g.setColour(distColor);
    g.fillRoundedRectangle(distortionBox.toFloat(), 10.0f);
    g.setColour(juce::Colours::orange.withAlpha(0.7f));
    g.drawRoundedRectangle(distortionBox.toFloat(), 10.0f, 2.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("OVERDRIVE", distortionBox.withHeight(30), juce::Justification::centred);

    // LED for Distortion
    auto distLedArea = distortionActive.getBounds().translated(0, -22).withSizeKeepingCentre(12, 12);
    if (distortionActive.getToggleState()) {
        g.setColour(juce::Colours::red.withAlpha(0.4f));
        g.fillEllipse(distLedArea.toFloat().expanded(5.0f));
        g.setColour(juce::Colours::red);
    }
    else {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
    }
    g.fillEllipse(distLedArea.toFloat());

    // --- DRAW REVERB PEDAL ---
    auto reverbColor = juce::Colour(0xff1d3a5c); // Deep Blue
    g.setColour(reverbColor);
    g.fillRoundedRectangle(reverbBox.toFloat(), 10.0f);
    g.setColour(juce::Colours::cyan.withAlpha(0.7f));
    g.drawRoundedRectangle(reverbBox.toFloat(), 10.0f, 2.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("REVERB", reverbBox.withHeight(30), juce::Justification::centred);

    // LED for Reverb Active
    auto revLedArea = reverbActive.getBounds().translated(0, -22).withSizeKeepingCentre(12, 12);
    if (reverbActive.getToggleState()) {
        g.setColour(juce::Colours::cyan.withAlpha(0.4f));
        g.fillEllipse(revLedArea.toFloat().expanded(5.0f));
        g.setColour(juce::Colours::cyan);
    }
    else {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
    }
    g.fillEllipse(revLedArea.toFloat());

    // LED for Freeze
    auto freezeLedArea = freezeActive.getBounds().translated(0, -22).withSizeKeepingCentre(12, 12);
    if (freezeActive.getToggleState()) {
        auto skyBlueNeon = juce::Colour::fromFloatRGBA(0.4f, 0.8f, 1.0f, 1.0f);
        g.setColour(skyBlueNeon.withAlpha(0.4f));
        g.fillEllipse(freezeLedArea.toFloat().expanded(5.0f));
        g.setColour(skyBlueNeon);
    }
    else {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
    }
    g.fillEllipse(freezeLedArea.toFloat());

    // LED for Shimmer
    auto shimLedArea = shimmerActive.getBounds().translated(0, -22).withSizeKeepingCentre(12, 12);
    if (shimmerActive.getToggleState()) {
        g.setColour(juce::Colours::gold.withAlpha(0.4f));
        g.fillEllipse(shimLedArea.toFloat().expanded(5.0f));
        g.setColour(juce::Colours::gold);
    }
    else {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
    }
    g.fillEllipse(shimLedArea.toFloat());

    // Labels for Reverb Foot-switches
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("FREEZE", freezeActive.getBounds().translated(0, -25), juce::Justification::centredTop);
    g.drawText("SHIMMER", shimmerActive.getBounds().translated(0, -25), juce::Justification::centredTop);
}


void PedalboardAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(50);
    area.reduce(15, 15);

    int spacing = 15;
    int pedalWidth = (area.getWidth() - (spacing * 2)) / 3;

    // Allocate pedal areas to member Variables (octaverBox, distortionBox, reverbBox)
    octaverBox = area.removeFromLeft(pedalWidth);
    area.removeFromLeft(spacing);
    distortionBox = area.removeFromLeft(pedalWidth);
    area.removeFromLeft(spacing);
    reverbBox = area;

    // --- OCTAVER LAYOUT ---
    auto octArea = octaverBox;
    octArea.removeFromTop(45);
    octArea.reduce(15, 15);
    dryWetOctaver.setBounds(octArea.removeFromTop(140).withSizeKeepingCentre(100, 100));
    octaverActive.setBounds(octArea.removeFromBottom(60).withSizeKeepingCentre(80, 50));

    // --- DISTORTION LAYOUT ---
    auto distArea = distortionBox;
    distArea.removeFromTop(45);
    distArea.reduce(10, 10);

    // Row 1: Gain and Mix (Side-by-side)
    auto distRow1 = distArea.removeFromTop(85);
    GainDistortion.setBounds(distRow1.removeFromLeft(distRow1.getWidth() / 2).withSizeKeepingCentre(70, 70));
    dryWetDist.setBounds(distRow1.withSizeKeepingCentre(70, 70));

    // Row 2: Level (Centered)
    distortionLevel.setBounds(distArea.removeFromTop(85).withSizeKeepingCentre(70, 70));

    // Row 3: ComboBox
    DistortionSelection.setBounds(distArea.removeFromTop(35).reduced(10, 0));

    // Bottom: Stomp
    distortionActive.setBounds(distArea.removeFromBottom(60).withSizeKeepingCentre(80, 50));

    // --- REVERB LAYOUT ---
    auto revArea = reverbBox;
    revArea.removeFromTop(45);
    revArea.reduce(15, 15);

    // Bottom Row: 3 switches
    auto stompArea = revArea.removeFromBottom(60);
    int bWidth = stompArea.getWidth() / 3;
    reverbActive.setBounds(stompArea.removeFromLeft(bWidth).withSizeKeepingCentre(65, 45));
    freezeActive.setBounds(stompArea.removeFromLeft(bWidth).withSizeKeepingCentre(65, 45));
    shimmerActive.setBounds(stompArea.withSizeKeepingCentre(65, 45));

    // Top: 2x2 Knob Grid
    int rowHeight = revArea.getHeight() / 2;
    auto row1 = revArea.removeFromTop(rowHeight);
    auto row2 = revArea;

    roomSize.setBounds(row1.removeFromLeft(row1.getWidth() / 2).withSizeKeepingCentre(75, 75));
    damping.setBounds(row1.withSizeKeepingCentre(75, 75));
    width.setBounds(row2.removeFromLeft(row2.getWidth() / 2).withSizeKeepingCentre(75, 75));
    dryWetRev.setBounds(row2.withSizeKeepingCentre(75, 75));
}
PedalboardAudioProcessorEditor::~PedalboardAudioProcessorEditor()
{
    // This cleans up the LookAndFeel when the editor is closed
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}