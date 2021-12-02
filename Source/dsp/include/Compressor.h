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
#pragma once
#include "LevelDetector.h"
#include "GainComputer.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "DelayLine.h"
#include "LookAhead.h"

/* Compressor-Class:
 * The circruit is modeled after the "ideal" VCA-Compressor
 * based on the paper "Digital Dynamic Range Compressor Design �  Tutorial and Analysis"
 * by Giannoulis, Massberg & Reiss
 */

class Compressor
{
public:

    Compressor() = default;
    ~Compressor();

    // Prepares compressor with a ProcessSpec-Object containing samplerate, blocksize and number of channels
    void prepare(const dsp::ProcessSpec& ps);
    
    // Sets mix 0.0f - 1.0f
    void setMix(float); // Param A.

    void setAir(float); // Param B.

    // Return current sampleRate
    double getSampleRate();

    float getMaxGainReduction();

    // Processes input buffer
    void process(AudioBuffer<float>& buffer);

private:
    inline void applyInputGain(AudioBuffer<float>&, int);

    void setupBands()
    {
        *processChain.get<ChainIndex::Band4>().state =
            *dsp::IIR::Coefficients<float>::makeHighShelf(procSpec.sampleRate, band4Freq,
                band4Q, Decibels::decibelsToGain(band4Gain));
    }

    //Directly initialize process spec to avoid debugging problems
    dsp::ProcessSpec procSpec{-1, 0, 0};

    AudioBuffer<float> originalSignal;
    std::vector<float> sidechainSignal;
    float* rawSidechainSignal{nullptr};



    LevelDetector ballistics;
    GainComputer gainComputer;
    DelayLine delay;
    LookAhead lookahead;
    SmoothingFilter smoothedAutoMakeup;

    double lookaheadDelay{0.005};
    float input{0.0f};
    float prevInput{0.0f};

    bool lookaheadEnabled{false};
    float mix{1.0f};
    float maxGainReduction{0.0f};
    float band4Freq{ 9500.0f }, band4Gain{ 0.0f }, band4Q{0.4f};

    using StereoFilter = dsp::ProcessorDuplicator<dsp::IIR::Filter<float>,
        dsp::IIR::Coefficients<float>>;
    dsp::ProcessorChain<StereoFilter> processChain;
    enum ChainIndex { Band4 };
};