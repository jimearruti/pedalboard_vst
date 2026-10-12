#include "Reverb.h"

PedalReverb::PedalReverb() {}
PedalReverb::~PedalReverb() {}

void PedalReverb::prepare(double sampleRate, int samplesPerBlock)
{
    reverb.setSampleRate(sampleRate);
    reverb.reset();
    shimmerShifter.prepare(sampleRate, 2.0f);

    dryBuffer.setSize(1, samplesPerBlock);
    bypassSmoother.reset(sampleRate, 0.03);
}

void PedalReverb::process(juce::AudioBuffer<float>& buffer,
    bool isActive,
    float roomSize,
    float damping,
    float width,
    float dryWet,
    bool freezeActive,
    bool shimmerActive)
{
    const int numSamples = buffer.getNumSamples();

    // 1. Store a copy of the input in our pre-allocated dry buffer
    dryBuffer.copyFrom(0, 0, buffer.getReadPointer(0), numSamples);

    bypassSmoother.setTargetValue(isActive ? 1.0f : 0.0f);

    // update Reverb Parameters
    juce::Reverb::Parameters params;
    params.roomSize = roomSize;
    params.damping = damping;
    params.width = width;
    params.freezeMode = freezeActive;
    params.wetLevel = 1.0f;
    params.dryLevel = 0.0f;
    
    // send parameters to reverb
    reverb.setParameters(params);

    // process mono signal
    reverb.processMono(buffer.getWritePointer(0), numSamples);

    // octave up
    shimmerShifter.process(buffer, shimmerActive, 1.0f);
 
    auto* outData = buffer.getWritePointer(0);
    auto* dryData = dryBuffer.getReadPointer(0);

    for (int s = 0; s < numSamples; ++s)
    {
         // dry/wet mix
         float wetSample = (dryData[s] * (1.0f - dryWet)) + (outData[s] * dryWet);
         float bypassGain = bypassSmoother.getNextValue();
         // crossfade with dry if we turn it off/on
         outData[s] = (wetSample * bypassGain) + (dryData[s] * (1.0f - bypassGain));
    }
 
}