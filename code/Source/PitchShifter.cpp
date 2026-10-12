#include "PitchShifter.h"

PitchShifter::PitchShifter() {}
PitchShifter::~PitchShifter() {}

void PitchShifter::prepare(double sampleRate, float ratio)
{
    this->ratio = ratio;
    hopSize = fftSize / overlap;

    // initialize fft
    fft = std::make_unique<juce::dsp::FFT>((int)std::log2(fftSize));

    // Calculate the fixed resampling length
    rLen = (int)floorf((float)fftSize / ratio);

    // Setup audio buffers
    inputBufferLength = fftSize;
    inputBuffer.setSize(1, inputBufferLength);
    inputBuffer.clear();

    // Ensure output buffer is large enough for the pitch shift (0.5 is worst case)
    outputBufferLength = (int)floorf((float)fftSize / 0.5f);
    outputBuffer.setSize(1, outputBufferLength);
    outputBuffer.clear();

    // Pre-allocate fft window
    fftWindow.realloc(fftSize);
    updateWindow(fftWindow, fftSize);
    // Precalculate its square root
    for (int i = 0; i < fftSize; ++i)
    {
        fftWindow[i] = std::sqrt(fftWindow[i]);
    }

    // Pre-allocate synthesis window
    synthesisWindow.realloc(rLen);
    updateWindow(synthesisWindow, rLen);
    // Precalculate its square root
    for (int i = 0; i < rLen; ++i)
    {
        synthesisWindow[i] = std::sqrt(synthesisWindow[i]);
    }

    // pre-allocate other heap blocks
    resampledOutput.realloc(rLen);
    resampledOutput.clear(rLen);

    // fft returns buffer of 2*fftSize samples, per doc
    fftFrequencyDomain.realloc(2 * fftSize);
    fftFrequencyDomain.clear(2 * fftSize);
    fftTimeDomain.realloc(2 * fftSize);
    fftTimeDomain.clear(2 * fftSize);

    // pre-allocate phase info block
    omega.realloc(fftSize);
    // pre-calculate the relationship between w and f
    for (int i = 0; i < fftSize; ++i)
        omega[i] = juce::MathConstants<float>::twoPi * (float)i / (float)fftSize;

    // Phase buffers of size fftSize (since we only process half)
    inputPhase.setSize(1, fftSize);
    outputPhase.setSize(1, fftSize);
    inputPhase.clear();
    outputPhase.clear();

    // calculate window scale factor
    // using the squared window sum because we apply sqrt() twice
    float windowSum = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        windowSum += (fftWindow[i] * fftWindow[i]);
    }
    windowScaleFactor = 0.0f;
    if (overlap != 0 && windowSum != 0.0f)
    {
        windowScaleFactor = 1.0f / (float)overlap / windowSum * (float)fftSize;
    }

    // Reset positions
    inputBufferWritePosition = 0;
    outputBufferWritePosition = hopSize % outputBufferLength;
    outputBufferReadPosition = 0;
    samplesSinceLastFFT = 0;

    // reset ramp for on/off
    bypassSmoother.reset(sampleRate, 0.02);
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer, bool isActive, float dryWet)
{
    const int numSamples = buffer.getNumSamples();
    const int outBufLen = outputBuffer.getNumSamples();

    // set target value for ramp on/off
    bypassSmoother.setTargetValue(isActive ? 1.0f : 0.0f);

    float* channelData = buffer.getWritePointer(0);

    int currentInputBufferWritePosition = inputBufferWritePosition;
    int currentOutputBufferWritePosition = outputBufferWritePosition;
    int currentOutputBufferReadPosition = outputBufferReadPosition;
    int currentSamplesSinceLastFFT = samplesSinceLastFFT;

    for (int sample = 0; sample < numSamples; ++sample)
    {
#
        const float in = channelData[sample];
        const float wet = outputBuffer.getSample(0, currentOutputBufferReadPosition);
        // mixing
        float effectMix = (in * (1.0f - dryWet)) + (wet * dryWet);
        // get gain according to on/off ramp (in case we're toggling)
        float currentBypassGain = bypassSmoother.getNextValue();
        // crossfade
        channelData[sample] = (effectMix * currentBypassGain) + (in * (1.0f - currentBypassGain));

        // clear the samples we've used
        outputBuffer.setSample(0, currentOutputBufferReadPosition, 0.0f);
        if (++currentOutputBufferReadPosition >= outBufLen) currentOutputBufferReadPosition = 0;
        inputBuffer.setSample(0, currentInputBufferWritePosition, in);
        if (++currentInputBufferWritePosition >= inputBufferLength) currentInputBufferWritePosition = 0;

        // If we have a full buffer, perform fft
        if (++currentSamplesSinceLastFFT >= hopSize)
        {
            currentSamplesSinceLastFFT = 0;

            // Analysis
            int fftReadPtr = currentInputBufferWritePosition;
            for (int i = 0; i < fftSize; ++i)
            {
                // fftWindow[i] has sqrt incorporated
                fftTimeDomain[i].real(fftWindow[i] * inputBuffer.getSample(0, fftReadPtr));
                fftTimeDomain[i].imag(0.0f);
                if (++fftReadPtr >= inputBufferLength) fftReadPtr = 0;
            }

            fft->perform(fftTimeDomain, fftFrequencyDomain, false);

            // Phase vocoder
            for (int i = 0; i <= fftSize / 2; ++i)
            {
                // get magnitude and phase of fft
                float mag = std::abs(fftFrequencyDomain[i]);
                float phase = std::arg(fftFrequencyDomain[i]);

                // calculate the phase deviation
                float phaseDev = phase - inputPhase.getSample(0, i) - omega[i] * (float)hopSize;
                // find exact frequency
                float deltaPhi = omega[i] * (float)hopSize + princArg(phaseDev);
                // stretch the phase by ratio
                float newPhase = princArg(outputPhase.getSample(0, i) + deltaPhi * ratio);

                inputPhase.setSample(0, i, phase);
                outputPhase.setSample(0, i, newPhase);

                fftFrequencyDomain[i] = std::polar(mag, newPhase);
            }

            // fill the rest of the buffer with the conjugates
            for (int i = fftSize / 2 + 1; i < fftSize; ++i)
                fftFrequencyDomain[i] = std::conj(fftFrequencyDomain[fftSize - i]);

            // ifft
            fft->perform(fftFrequencyDomain, fftTimeDomain, true);

            // resample
            float resampleStep = (float)fftSize / (float)rLen;
            for (int i = 0; i < rLen; ++i)
            {
                float x = (float)i * resampleStep;
                int ix = (int)x;
                float dx = x - (float)ix;

                float s1 = fftTimeDomain[ix].real();
                float s2 = fftTimeDomain[(ix + 1) % fftSize].real();

                // synthesisWindow[i] has the sqrt() from prepare
                resampledOutput[i] = (s1 + dx * (s2 - s1)) * synthesisWindow[i];
            }

            // Overlap add
            int synthesisPtr = currentOutputBufferWritePosition;
            for (int i = 0; i < rLen; ++i)
            {
                float currentVal = outputBuffer.getSample(0, synthesisPtr);
                outputBuffer.setSample(0, synthesisPtr,
                    currentVal + (resampledOutput[i] * windowScaleFactor));

                if (++synthesisPtr >= outBufLen) synthesisPtr = 0;
            }
            // move pointer over hop size
            currentOutputBufferWritePosition = (currentOutputBufferWritePosition + hopSize) % outBufLen;
        }

    }

    // update pointers
    inputBufferWritePosition = currentInputBufferWritePosition;
    outputBufferWritePosition = currentOutputBufferWritePosition;
    outputBufferReadPosition = currentOutputBufferReadPosition;
    samplesSinceLastFFT = currentSamplesSinceLastFFT;

    // reset when it's off and silent
    if (!isActive && !bypassSmoother.isSmoothing() && needsReset) {
        reset();
        needsReset = false;
    }
    else if (isActive) {
        needsReset = true;
    }
}

float PitchShifter::princArg(float phase) {
    // wrap over 2 pi
    if (phase >= 0.0f) return fmodf(phase + juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi) - juce::MathConstants<float>::pi;
    else return fmodf(phase + juce::MathConstants<float>::pi, -juce::MathConstants<float>::twoPi) + juce::MathConstants<float>::pi;
}

void PitchShifter::updateWindow(juce::HeapBlock<float>& window, int len) {
    // Hann window
    for (int i = 0; i < len; ++i)
        window[i] = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(len - 1));
}

void PitchShifter::reset() {
    inputBuffer.clear();
    outputBuffer.clear();
    inputPhase.clear();
    outputPhase.clear();
    inputBufferWritePosition = 0;
    outputBufferReadPosition = 0;
    outputBufferWritePosition = hopSize % outputBufferLength;
    samplesSinceLastFFT = 0;
}