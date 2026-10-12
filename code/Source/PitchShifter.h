#pragma once

#include <JuceHeader.h>
#include <complex>


class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter();

    void prepare(double sampleRate, float ratio);
    void process(juce::AudioBuffer<float>& buffer, bool isActive, float dryWet);
    void reset();

private:

    float princArg(float phase);
    void updateWindow(juce::HeapBlock<float>& window, int len);

    juce::SmoothedValue<float> bypassSmoother;
    bool needsReset = false;

    // Buffer lengths
    int inputBufferLength = 0;
    int outputBufferLength = 0;

    // Positions
    int inputBufferWritePosition = 0;
    int outputBufferWritePosition = 0;
    int outputBufferReadPosition = 0;
    int samplesSinceLastFFT = 0;
    int rLen = 0;

    // Settings
    int fftSize = 2048;
    int overlap = 8;
    int hopSize;
    float ratio;
    float windowScaleFactor = 0.0f;

    // Core buffers
    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;
    juce::AudioBuffer<float> inputPhase;
    juce::AudioBuffer<float> outputPhase;

    // fft analysis/synthesis buffers
    juce::HeapBlock<float> fftWindow, omega;
    juce::HeapBlock<std::complex<float>> fftTimeDomain, fftFrequencyDomain;
    std::unique_ptr<juce::dsp::FFT> fft;
    juce::HeapBlock<float> resampledOutput;
    juce::HeapBlock<float> synthesisWindow;

};