/*
Copyright (C) 2020 Phillip Lamp
This file is part of the CTAGDRC project <https://github.com/p-hlp/CTAGDRC>.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "include/Compressor.h"

#include "../util/include/SIMDMath.h"


Compressor::~Compressor()
{
    rawSidechainSignal = nullptr;
}

void Compressor::prepare(const dsp::ProcessSpec& ps)
{
    procSpec = ps;
    ballistics.prepare(ps.sampleRate);
    delay.setDelay(0.005f);
    delay.prepare(ps);
    originalSignal.setSize(2, ps.maximumBlockSize);
    sidechainSignal.resize(ps.maximumBlockSize, 0.0f);
    rawSidechainSignal = sidechainSignal.data();
    originalSignal.clear();
    lookahead.prepare(procSpec.sampleRate, lookaheadDelay, ps.maximumBlockSize);
    smoothedAutoMakeup.prepare(ps.sampleRate);
    smoothedAutoMakeup.setAlpha(0.03);
    processChain.prepare(procSpec);

    lfoPhase = 0.0f;
    inverseSampleRate = 1.0f / (float)ps.sampleRate;
    



}



void Compressor::setMix(float newMix) // Param A.
{
    mix = newMix;
}

void Compressor::setAir(float band4G) // Param B.
{
    band4Gain = band4G;
}

void Compressor::setDrive(float newDrive) // Param A.
{
    drive = newDrive;
}

void Compressor::setDepth(float newDepth) 
{
    depth = newDepth;
}

void Compressor::setCarrFreq(float newFreq)
{
    carrierfreq = newFreq;
}


double Compressor::getSampleRate()
{
    return procSpec.sampleRate;
}

float Compressor::getMaxGainReduction()
{
    return maxGainReduction;
}

void Compressor::process(AudioBuffer<float>& buffer)
{
    if (!false)
    {
        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = buffer.getNumChannels();

        jassert(numSamples == static_cast<int>(sidechainSignal.size()));

        // Clear any old samples
        originalSignal.clear();
        FloatVectorOperations::fill(rawSidechainSignal, 0.0f, numSamples);
        //maxGainReduction = 0.0f;

        // Apply input gain
        applyInputGain(buffer, numSamples);

        // Get max l/r amplitude values and fill sidechain signal
        FloatVectorOperations::abs(rawSidechainSignal, buffer.getReadPointer(0), numSamples);
        FloatVectorOperations::max(rawSidechainSignal, rawSidechainSignal, buffer.getReadPointer(1), numSamples);

        // Calculate crest factor on max. amplitude values of input buffer
        ballistics.processCrestFactor(rawSidechainSignal, numSamples);

        // Compute attenuation - converts side-chain signal from linear to logarithmic domain
        gainComputer.applyCompressionToBuffer(rawSidechainSignal, numSamples);

        // Smooth attenuation - still logarithmic
        ballistics.applyBallistics(rawSidechainSignal, numSamples);

        // Get minimum = max. gain reduction from side chain buffer
        maxGainReduction = FloatVectorOperations::findMinimum(rawSidechainSignal, numSamples);

        // Calculate auto makeup
        //autoMakeup = calculateAutoMakeup(rawSidechainSignal, numSamples);

        // Do lookahead if enabled
        if (lookaheadEnabled)
        {
            // Delay input buffer
            delay.process(buffer);

            // Process side-chain (delay + gain reduction fade in)
            lookahead.process(rawSidechainSignal, numSamples);
        }

        // Add makeup gain and convert side-chain to linear domain
        for (int i = 0; i < numSamples; ++i)
            sidechainSignal[i] = Decibels::decibelsToGain(sidechainSignal[i] /*+ makeup + autoMakeup*/);

        // Copy buffer to original signal
        for (int i = 0; i < numChannels; ++i)
            originalSignal.copyFrom(i, 0, buffer, i, 0, numSamples);

        // Multiply attenuation with buffer - apply compression
        for (int i = 0; i < numChannels; ++i)
            FloatVectorOperations::multiply(buffer.getWritePointer(i), rawSidechainSignal, buffer.getNumSamples());

        // Mix dry & wet signal
        for (int i = 0; i < numChannels; ++i)
        {
            float* channelData = buffer.getWritePointer(i); //wet signal
            FloatVectorOperations::multiply(channelData, mix, numSamples);
            FloatVectorOperations::addWithMultiply(channelData, originalSignal.getReadPointer(i), 1 - mix, numSamples);


        }

        // Saturation
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* outputData = buffer.getWritePointer(channel);
            auto* inputData = buffer.getReadPointer(channel);
            auto* rawDrive = &drive;

            for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
                outputData[sample] = softClip(inputData[sample], *rawDrive) * pow(10, /**rawTrim * */0.05f);
            }
        }

        // Eq
        setupBands();
        dsp::AudioBlock<float> block(buffer);
        dsp::ProcessContextReplacing<float> context(block);
        processChain.process(context);

        // RingMod
        float* currentDepth = &depth;
        float* currentFrequency = &carrierfreq;
        float phase;
        //float* inverseSR = &inverseSampleRate;

        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            phase = lfoPhase;


            for (int sample = 0; sample < numSamples; ++sample) {
                const float in = channelData[sample];
                float carrier = 2.0f * lfo(phase) - 1.0f;
                //float out = in * (1 - (int)currentDepth + (int)currentDepth * carrier);
                float out = in * (1 - depth + depth * carrier);

                channelData[sample] = out;

                phase += carrierfreq * inverseSampleRate;
                if (phase >= 1.0f)
                    phase -= 1.0f;
            }
        }
        lfoPhase = phase;

        /*for (int channel = 0; channel < numChannels; ++channel)
            buffer.clear(channel, 0, numSamples);*/

    }
}

float Compressor::softClip(const float& input, const float& drive) {

    //1.5f to account for drop in gain from the saturation initial state
    //pow(10, (-1 * drive) * 0.04f) to account for the increase in gain when the drive goes up
    
    //Original signal:
    //return piDivisor * atan(pow(10, (drive * 4) * 0.05f) * input) * 1.5f * pow(10, (-1 * drive) * 0.04f);

    return piDivisor * atan(pow(10, (drive * 3) * 0.04f) * input) * 2.3f * pow(10, (-1 * drive) * 0.03f);
}

inline void Compressor::applyInputGain(AudioBuffer<float>& buffer, int numSamples)
{
    if (prevInput == input)
        buffer.applyGain(0, numSamples, Decibels::decibelsToGain(prevInput));
    else
    {
        buffer.applyGainRamp(0, numSamples, Decibels::decibelsToGain(prevInput), Decibels::decibelsToGain(input));
        prevInput = input;
    }
}


float Compressor::lfo(float phase)
{
    float out = 0.0f;

    /*if (phase < 0.25f)
        out = 0.5f + 2.0f * phase;
    else if (phase < 0.75f)
        out = 1.0f - 2.0f * (phase - 0.25f);
    else
        out = 2.0f * (phase - 0.75f);*/

    if (phase < 0.48f)
        out = 1.0f;
    else if (phase < 0.5f)
        out = 1.0f - 50.0f * (phase - 0.48f);
    else if (phase < 0.98f)
        out = 0.0f;
    else
        out = 50.0f * (phase - 0.98f);

    /*switch (waveform) {
    case waveformSine: {
        out = 0.5f + 0.5f * sinf(twoPi * phase);
        break;
    }
    case waveformTriangle: {
        if (phase < 0.25f)
            out = 0.5f + 2.0f * phase;
        else if (phase < 0.75f)
            out = 1.0f - 2.0f * (phase - 0.25f);
        else
            out = 2.0f * (phase - 0.75f);
        break;
    }
    case waveformSawtooth: {
        if (phase < 0.5f)
            out = 0.5f + phase;
        else
            out = phase - 0.5f;
        break;
    }
    case waveformInverseSawtooth: {
        if (phase < 0.5f)
            out = 0.5f - phase;
        else
            out = 1.5f - phase;
        break;
    }*/
    //case waveformSquare: {
        /*if (phase < 0.5f)
            out = 0.0f;
        else
            out = 1.0f;*/
        //break;
    //}
    /*case waveformSquareSlopedEdges: {
        if (phase < 0.48f)
            out = 1.0f;
        else if (phase < 0.5f)
            out = 1.0f - 50.0f * (phase - 0.48f);
        else if (phase < 0.98f)
            out = 0.0f;
        else
            out = 50.0f * (phase - 0.98f);
        break;
    }
    }*/

    return out;
}

