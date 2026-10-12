/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

//==============================================================================
/**
*/
class PedalboardAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    PedalboardAudioProcessorEditor (PedalboardAudioProcessor&);
    ~PedalboardAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PedalboardAudioProcessor& audioProcessor;

    //Octaver
    juce::Slider dryWetOctaver;
    juce::Label dryWetOctaverLabel;
    juce::TextButton octaverActive{ "Active" };

    //Distortion
    juce::Slider GainDistortion,dryWetDist,distortionLevel;
    juce::ComboBox DistortionSelection;
    juce::Label GainDistortionLabel, DistortionSelectionLabel, dryWetDistLabel, distortionLevelLabel;
    juce::TextButton distortionActive{ "Active" };

    //Reverb
    juce::Slider roomSize,damping,dryWetRev,width;
    juce::Label roomSizeLabel, dampingLabel, dryWetRevLabel,widthLabel;
    juce::TextButton reverbActive{ "Active" };
    juce::TextButton freezeActive{ "Freeze" };
    juce::TextButton shimmerActive{ "Active" };


    juce::Image knobImage;
    std::unique_ptr<FilmstripLookAndFeel> customLookAndFeel;
    juce::Rectangle<int> octaverBox;
    juce::Rectangle<int> distortionBox;
    juce::Rectangle<int> reverbBox;

    //Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> DistortionSelectionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> 
        dryWetOctaverAttachment, dryWetDistAttachment, GainDistortionAttachment, distortionLevelAttachment,
        roomSizeAttachment,dampingAttachment,widthAttachment, dryWetRevAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        octaverActiveAttachment,
        distortionActiveAttachment,
        reverbActiveAttachment,
        freezeActiveAttachment,
        shimmerActiveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalboardAudioProcessorEditor)
};
