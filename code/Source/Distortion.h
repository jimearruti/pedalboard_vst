#pragma once
#include <JuceHeader.h>

class PedalDistortion
{
public:
    PedalDistortion();
    ~PedalDistortion();
    void prepare(double sampleRate);
    void process(
        juce::AudioBuffer<float>& buffer,
        bool isActive,
        float dryWet,
        float gain,
        float level,
        int distortionType);
private:
    juce::SmoothedValue<float> bypassSmoother;
};