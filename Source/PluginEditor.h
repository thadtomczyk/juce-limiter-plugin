#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LimiterpluginAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    LimiterpluginAudioProcessorEditor (LimiterpluginAudioProcessor&);
    ~LimiterpluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;


private:
    
    LimiterpluginAudioProcessor& audioProcessor;

    using APVTS = LimiterpluginAudioProcessor::APVTS;

    // UI controls
    juce::Slider thresholdSlider;
    juce::Label  thresholdLabel;
    
    juce::Slider outputGainSlider;
    juce::Label  outputGainLabel;
    std::unique_ptr<APVTS::SliderAttachment> outputGainAttachment;


    // Binds the slider to the "THRESHOLD" parameter
    std::unique_ptr<APVTS::SliderAttachment> thresholdAttachment;


    void timerCallback() override;

    float displayedGRdB = 0.0f;   // Smoothed current value
    float peakHoldGRdB = 0.0f;    // Held maximum that slowly decays
    juce::Rectangle<int> grMeterBounds; // Area for drawing

    const float maxGRRange = 24.0f; // Show up to 24 dB of reduction

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LimiterpluginAudioProcessorEditor)
};
