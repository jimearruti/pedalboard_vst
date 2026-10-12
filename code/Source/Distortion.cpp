#pragma once
#include "Distortion.h"

PedalDistortion::PedalDistortion() {}
PedalDistortion::~PedalDistortion() {}

void PedalDistortion::prepare(double sampleRate)
{
    // prepare ramp for on/off
    bypassSmoother.reset(sampleRate, 0.02);
}

void PedalDistortion::process(juce::AudioBuffer<float>& buffer,
    bool isActive,
    float dryWet,
    float gain,
    float level,
    int distortionType)
{
    const int numSamples = buffer.getNumSamples();
    float* channelData = buffer.getWritePointer(0);

    // Set the target for bypass ramp
    bypassSmoother.setTargetValue(isActive ? 1.0f : 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        // Get dry signal
        const float drySample = channelData[i];

        // apply gain to the wet signal
        float wetSample = gain * drySample;

        // Apply selected distortion algorithm
        switch (distortionType)
        {
            case 0: // Hard Clipping
            {
                const float threshold = 0.5f;
                if (wetSample > threshold) wetSample = threshold;
                else if (wetSample < -threshold) wetSample = -threshold;
                break;
            }
            case 1: // Soft Clipping (Cubic)
            {
                const float threshold1 = 1.0f / 3.0f;
                const float threshold2 = 2.0f / 3.0f;

                if (wetSample > threshold1)
                {
                    if (wetSample > threshold2) wetSample = 0.5f;
                    else wetSample = (3.0f - std::pow(2.0f - 3.0f * wetSample, 2.0f)) / 6.0f;
                }
                else if (wetSample < -threshold1)
                {
                    if (wetSample < -threshold2) wetSample = -0.5f;
                    else wetSample = -(3.0f - std::pow(2.0f + 3.0f * wetSample, 2.0f)) / 6.0f;
                }
                break;
            }
            case 2: // Hyperbolic Tangent
            {
                wetSample = wetSample / (1.0f + std::abs(wetSample));
                break;
            }
            default:
                break;
        }

        // Dry/Wet combination
        float mixSample = (drySample * (1.0f - dryWet)) + (wetSample * dryWet);
        
        // Apply the level knob
        float mixSampleLeveled = mixSample * level;

        // Smooth it according to the ramp (to prevent clicks when turning on/off)
        // get gain for that moment of the ramp
        float currentBypassGain = bypassSmoother.getNextValue();
        // crossfade
        channelData[i] = (mixSampleLeveled * currentBypassGain) + (drySample * (1.0f - currentBypassGain));

        // Clip for safety
        channelData[i] = juce::jlimit(-1.0f, 1.0f, channelData[i]);
    }
}