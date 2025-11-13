#pragma once

#include <vector>
#include <cmath>
#include <memory>
#include <limits>
#include <atomic>


#include <JuceHeader.h>


class LimiterpluginAudioProcessor  : public juce::AudioProcessor
{
public:

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS& getValueTreeState() { return parameters; }

    LimiterpluginAudioProcessor();
    ~LimiterpluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getCurrentGRdB() const { return currentGRdB.load(std::memory_order_relaxed); }

private:

    float lastThresholdDb = std::numeric_limits<float>::quiet_NaN();


    APVTS parameters{ *this, nullptr, "Parameters", createParameterLayout() };
    static APVTS::ParameterLayout createParameterLayout();

  
    
    // Limiter State
    float currentSampleRate = 44100.0f;

    // Settings
    float thresholdDb = -1.0f;  // Ceiling in dBFS
    float thresholdLin = 0.0f;  // Same threshold in linear amplitude
    float releaseTimeMs = 50.0f;   // Release time in ms
    float releaseCoeff = 0.0f;  //Coefficient for exponential release

    // Output gain
    float lastOutputGainDb = std::numeric_limits<float>::quiet_NaN();
    float outputGainLin = 1.0f; // Cached linear makeup gain
    std::atomic<float> currentGRdB{ 0.0f }; // Current gain reduction in dB

    // One gain envelope per channel
    std::vector<float> gainEnvelope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LimiterpluginAudioProcessor)
};
