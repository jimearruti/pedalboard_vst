/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Distortion.h"

//==============================================================================
PedalboardAudioProcessor::PedalboardAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::mono(), true)
                     #endif
                       ),parameters(*this, nullptr, "parameters", createParameterLayout())
#endif
{
}

PedalboardAudioProcessor::~PedalboardAudioProcessor()
{
}

//==============================================================================
const juce::String PedalboardAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PedalboardAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PedalboardAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PedalboardAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PedalboardAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PedalboardAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PedalboardAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PedalboardAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PedalboardAudioProcessor::getProgramName (int index)
{
    return {};
}

void PedalboardAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PedalboardAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    pitchShifterDown.prepare(sampleRate, 0.5);
    myReverb.prepare(sampleRate, samplesPerBlock);
    myDistortion.prepare(sampleRate);
}

void PedalboardAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PedalboardAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PedalboardAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;


    auto octaverIsActive = parameters.getRawParameterValue("octaverActive")->load();
    auto dryWetOctaver = parameters.getRawParameterValue("dryWetOctaver")->load();

    pitchShifterDown.process(buffer, octaverIsActive, dryWetOctaver);

    auto distIsActive = parameters.getRawParameterValue("distortionActive")->load();
    auto dryWetDist = parameters.getRawParameterValue("dryWetDist")->load();
    auto distGain = parameters.getRawParameterValue("GainDistortion")->load();
    auto distLevel = parameters.getRawParameterValue("distortionLevel")->load();
    auto distortionType = parameters.getRawParameterValue("DistortionSelection")->load();

    myDistortion.process(buffer, distIsActive, dryWetDist, distGain, distLevel, distortionType);

    auto reverbIsActive = parameters.getRawParameterValue("reverbActive")->load();
    auto roomSize = parameters.getRawParameterValue("roomSize")->load();
    auto damping = parameters.getRawParameterValue("damping")->load();
    auto width = parameters.getRawParameterValue("width")->load();
    auto dryWetRev = parameters.getRawParameterValue("dryWetRev")->load();
    auto freezeActive = parameters.getRawParameterValue("freezeActive")->load();
    auto shimmerActive = parameters.getRawParameterValue("shimmerActive")->load();

    myReverb.process(buffer, reverbIsActive, roomSize, damping, width, dryWetRev, freezeActive, shimmerActive);


}

//==============================================================================
bool PedalboardAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PedalboardAudioProcessor::createEditor()
{
    return new PedalboardAudioProcessorEditor (*this);
}

//==============================================================================
void PedalboardAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PedalboardAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PedalboardAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout PedalboardAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    //Active
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "octaverActive", 1 }, "Octaver Active", false));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "distortionActive", 1 }, "Distortion Active", false));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "reverbActive", 1 }, "Reverb Active", false));
    //Octaver
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "dryWetOctaver", 1 }, "Dry/Wet", 0.0f, 1.0f, 0.5f));
    //Distortion
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID 
        {"DistortionSelection", 1},"Distortion Selection",
        juce::StringArray {"Hard Clipping", "Soft Clipping", "Hiperbolic Tangent"}, 0));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"GainDistortion", 1},"Gain Distortion", 1.0f, 10.0, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "distortionLevel", 1 }, "Distortion Level", 0.0f, 1.0, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "dryWetDist", 1 }, "Dry/Wet", 0.0f, 1.0f, 0.5f));

    //Reverb
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"roomSize", 1},"Room Size", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"damping", 1},"Damping",0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"dryWetRev", 1},"Dry/Wet", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"width", 1},"Width",  0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "freezeActive", 1 }, "Freeze Active", false));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "shimmerActive", 1 }, "Shimmer Active", false));

    return {parameters.begin(),parameters.end()};
    
}
