/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "gui/include/SCLookAndFeel.h"
#include "gui/include/LabeledSlider.h"
#include "gui/include/Meter.h"

//==============================================================================
/**
*/
class CtagdrcAudioProcessorEditor : public AudioProcessorEditor, public Button::Listener, Timer
{
public:
    CtagdrcAudioProcessorEditor(CtagdrcAudioProcessor&, AudioProcessorValueTreeState&);
    ~CtagdrcAudioProcessorEditor();

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button*) override;
    void timerCallback() override;
    Image bg_image;

private:
    void initWidgets();
    void setGUIState(bool);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CtagdrcAudioProcessor& processor;
    AudioProcessorValueTreeState& valueTreeState;

    //LookAndFeel
    SCLookAndFeel scLaF;
    Colour backgroundApp;

    //Widgets
    MeterBackground meterbg;
    Meter meter;

    LabeledSlider mixLSlider; // Param A.
    LabeledSlider airLSlider; // Param B.
    Label appTitle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CtagdrcAudioProcessorEditor)
};
