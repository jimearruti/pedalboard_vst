#pragma once
#include <JuceHeader.h>
#include "PitchShifter.h"

class PedalReverb
{
public:
    PedalReverb();
    ~PedalReverb();

    // Prepare internal buffers and the shimmer shifter
    void prepare(double sampleRate, int samplesPerBlock);
    // The main processing chain
    void process(juce::AudioBuffer<float>& buffer,
        bool isActive,
        float roomSize,
        float damping,
        float width,
        float dryWet,
        bool freezeActive,
        bool shimmerActive);

private:
    PitchShifter shimmerShifter;
    juce::Reverb reverb;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> bypassSmoother;

};