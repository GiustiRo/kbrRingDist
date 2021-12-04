/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CtagdrcAudioProcessor::CtagdrcAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
          .withInput("Input", AudioChannelSet::stereo(), true)
#endif
          .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
      ),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    //Add parameter listener
    parameters.addParameterListener("lookahead", this);
    parameters.addParameterListener("inputgain", this);
    parameters.addParameterListener("mix", this);
    parameters.addParameterListener("air", this);
    parameters.addParameterListener("drive", this);
    //parameters.addParameterListener("trim", this);
    parameters.addParameterListener("model", this);


    gainReduction.set(0.0f);
    currentInput.set(-std::numeric_limits<float>::infinity());
    currentOutput.set(-std::numeric_limits<float>::infinity());

}

CtagdrcAudioProcessor::~CtagdrcAudioProcessor()
{
}

//==============================================================================
const String CtagdrcAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CtagdrcAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool CtagdrcAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool CtagdrcAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double CtagdrcAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CtagdrcAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int CtagdrcAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CtagdrcAudioProcessor::setCurrentProgram(int index)
{
}

const String CtagdrcAudioProcessor::getProgramName(int index)
{
    return {};
}

void CtagdrcAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================
void CtagdrcAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    //Prepare dsp classes
    compressor.prepare({sampleRate, static_cast<uint32>(samplesPerBlock), 2});
    inLevelFollower.prepare(sampleRate);
    outLevelFollower.prepare(sampleRate);
    inLevelFollower.setPeakDecay(0.3f);
    outLevelFollower.setPeakDecay(0.3f);

    //Notify host about latency
    if (*parameters.getRawParameterValue("lookahead") > 0.5f)
        setLatencySamples(static_cast<int>(0.005 * sampleRate));
    else
        setLatencySamples(0);
}

void CtagdrcAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CtagdrcAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void CtagdrcAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    // Clear buffer
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //Update input peak metering
    inLevelFollower.updatePeak(buffer.getArrayOfReadPointers(), totalNumInputChannels, numSamples);
    currentInput.set(Decibels::gainToDecibels(inLevelFollower.getPeak()));

    // Do compressor processing
    compressor.process(buffer);

    // Update gain reduction metering
    gainReduction.set(compressor.getMaxGainReduction());

    // Update output peak metering
    outLevelFollower.updatePeak(buffer.getArrayOfReadPointers(), totalNumInputChannels, numSamples);
    currentOutput = Decibels::gainToDecibels(outLevelFollower.getPeak());

}

//==============================================================================
bool CtagdrcAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* CtagdrcAudioProcessor::createEditor()
{
    return new CtagdrcAudioProcessorEditor(*this, parameters);
}

//==============================================================================
void CtagdrcAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    // create xml with state information
    std::unique_ptr <juce::XmlElement> outputXml(parameters.state.createXml());
    // save xml to binary
    copyXmlToBinary(*outputXml, destData);
}

void CtagdrcAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // create xml from binary
    std::unique_ptr<juce::XmlElement> inputXml(getXmlFromBinary(data, sizeInBytes));
    // check that theParams returned correctly
    if (inputXml != nullptr)
    {
        // if theParams tag name matches tree state tag name
        if (inputXml->hasTagName(parameters.state.getType()))
        {
            // copy xml into tree state
            parameters.state = juce::ValueTree::fromXml(*inputXml);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CtagdrcAudioProcessor();
}

void CtagdrcAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == "mix") {
        compressor.setMix(newValue);
        compressor.setDrive(newValue * 10);
    }
    // Apply modifications...
    // 
    /*else if (parameterID == "mix") {
        compressor.setMix(newValue);
    }*/
    //else if (parameterID == "air") {
    //    compressor.setAir(newValue);
    //}
}

//float CtagdrcAudioProcessor::softClip(const float& input, const float& drive) {
//
//    //1.5f to account for drop in gain from the saturation initial state
//    //pow(10, (-1 * drive) * 0.04f) to account for the increase in gain when the drive goes up
//
//    return piDivisor * atan(pow(10, (drive * 4) * 0.05f) * input) * 1.5f * pow(10, (-1 * drive) * 0.04f);
//}

AudioProcessorValueTreeState::ParameterLayout CtagdrcAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterBool>("lookahead", "Lookahead", false));

    params.push_back(std::make_unique<AudioParameterFloat>("inputgain", "Input",
                                                           NormalisableRange<float>(
                                                               Constants::Parameter::inputStart,
                                                               Constants::Parameter::inputEnd,
                                                               Constants::Parameter::inputInterval), 10.0f,
                                                           String(),
                                                           AudioProcessorParameter::genericParameter,
                                                           [](float value, float)
                                                           {
                                                               return String(value, 1) + " dB";
                                                           }));


    params.push_back(std::make_unique<AudioParameterFloat>("mix", "Mix",
                                                           NormalisableRange<float>(
                                                               Constants::Parameter::mixStart,
                                                               Constants::Parameter::mixEnd,
                                                               Constants::Parameter::mixInterval),
                                                           0.0f, "%", AudioProcessorParameter::genericParameter,
                                                           [](float value, float)
                                                           {
                                                               return String(round(value * 100), 0) + " %";
                                                           })); // Param A.

    params.push_back(std::make_unique<AudioParameterFloat>("air", "Air",
                                                            NormalisableRange<float>(
                                                                Constants::Parameter::airStart,
                                                                Constants::Parameter::airEnd,
                                                                Constants::Parameter::airInterval),
                                                            0.0f, "%", AudioProcessorParameter::genericParameter,
                                                            [](float value, float)
                                                            {
                                                                return String(value, 1) + " dB";
                                                            })); // Param B.

    params.push_back(std::make_unique<AudioParameterFloat>("drive", "Drive",
                                                            NormalisableRange<float>(
                                                                Constants::Parameter::driveStart,
                                                                Constants::Parameter::driveEnd,
                                                                Constants::Parameter::driveInterval),
                                                            0.0f, "%", AudioProcessorParameter::genericParameter,
                                                            [](float value, float)
                                                            {
                                                                return String(value, 1) + " dB";
                                                            }));

    return {params.begin(), params.end()};
}
