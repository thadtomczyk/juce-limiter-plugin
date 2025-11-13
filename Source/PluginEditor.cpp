#include "PluginProcessor.h"
#include "PluginEditor.h"

LimiterpluginAudioProcessorEditor::LimiterpluginAudioProcessorEditor (LimiterpluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Slider setup
    thresholdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    thresholdSlider.setRange(-24.0, 0.0, 0.1);
    thresholdSlider.setDoubleClickReturnValue(true, -1.0); // reset to -1 dB on double-click

    thresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::deepskyblue);
    thresholdSlider.setColour(juce::Slider::thumbColourId, juce::Colours::aqua);

    addAndMakeVisible(thresholdSlider);

    // Output Gain slider
    outputGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    outputGainSlider.setRange(-24.0, 24.0, 0.1);
    outputGainSlider.setDoubleClickReturnValue(true, 0.0); // reset to 0 dB
    outputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
    outputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::yellow);
    addAndMakeVisible(outputGainSlider);

    outputGainLabel.setText("Output Gain (dB)", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.attachToComponent(&outputGainSlider, false);
    addAndMakeVisible(outputGainLabel);

    // Attachment
    outputGainAttachment = std::make_unique<APVTS::SliderAttachment>(
        audioProcessor.getValueTreeState(), "OUTPUT_GAIN", outputGainSlider);

    // Label
    thresholdLabel.setText("Threshold (dB)", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centred);
    thresholdLabel.attachToComponent(&thresholdSlider, false); 
    addAndMakeVisible(thresholdLabel);

    // Attachment: binds slider to parameter
    thresholdAttachment = std::make_unique<APVTS::SliderAttachment>(
        audioProcessor.getValueTreeState(), "THRESHOLD", thresholdSlider);

    setSize(480, 300);

    startTimerHz(30);
}

LimiterpluginAudioProcessorEditor::~LimiterpluginAudioProcessorEditor()
{
}

void LimiterpluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colours::black);

    // Title text
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawFittedText("Super Professional Limiter",
        getLocalBounds().removeFromTop(40),
        juce::Justification::centred,
        1);

    auto r = grMeterBounds;

    // Background
    g.setColour(juce::Colours::darkgrey.withAlpha(0.7f));
    g.fillRoundedRectangle(r.toFloat(), 3.0f);

    // Fill amount
    const float frac = juce::jlimit(0.0f, 1.0f, displayedGRdB / maxGRRange);
    const int   fillH = int(r.getHeight() * frac);
    auto fill = r.removeFromBottom(fillH);

    g.setColour(juce::Colours::deepskyblue);
    g.fillRoundedRectangle(fill.toFloat(), 2.0f);

    // Peak-hold tick (white line)
    const float peakFrac = juce::jlimit(0.0f, 1.0f, peakHoldGRdB / maxGRRange);
    const int   y = grMeterBounds.getBottom() - int(grMeterBounds.getHeight() * peakFrac);
    g.setColour(juce::Colours::white);
    g.fillRect(grMeterBounds.getX(), y, grMeterBounds.getWidth(), 2);

    // Numeric readout
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(14.0f));
    auto textBox = grMeterBounds.expanded(10, 0).translated(0, -18);
    juce::String txt = juce::String(displayedGRdB, 1) + " dB";
    g.drawFittedText(txt, textBox, juce::Justification::centred, 1);

    // Label
    g.setFont(12.0f);
    g.drawFittedText("GR", r.removeFromTop(14), juce::Justification::centred, 1);
}

void LimiterpluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    
    // Reserve top 60px for the title and label area
    area.removeFromTop(60);

    // Center the knob in the remaining area
    auto leftMeter = area.removeFromLeft(20);
    auto rightMeter = area.removeFromRight(70);

    grMeterBounds = rightMeter.reduced(8);

    // Two knobs in the middle area
    auto knobs = area.reduced(10);
    auto left = knobs.removeFromLeft(knobs.getWidth() / 2).reduced(10);
    auto right = knobs.reduced(10);

    // Size/center the circles
    const int knobSize = 160;
    thresholdSlider.setBounds(left.withSizeKeepingCentre(knobSize, knobSize));
    outputGainSlider.setBounds(right.withSizeKeepingCentre(knobSize, knobSize));

}

void LimiterpluginAudioProcessorEditor::timerCallback()
{
    // Read current GR (dB) from processor
    const float target = audioProcessor.getCurrentGRdB();

    // Smooth UI movement
    const float alpha = 0.2f;
    displayedGRdB += alpha * (target - displayedGRdB);

    // Peak-hold: rise instantly, fall slowly
    if (target > peakHoldGRdB)
        peakHoldGRdB = target;
    else
        peakHoldGRdB = juce::jmax(0.0f, peakHoldGRdB - 0.2f); // ~0.2 dB per frame fall

    repaint(grMeterBounds);
}