#include "PluginProcessor.h"
#include "PluginEditor.h"

LimiterpluginAudioProcessor::APVTS::ParameterLayout
LimiterpluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "THRESHOLD",                // Parameter ID
        "Threshold",                // Display name
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f),
        -1.0f                       // Default dB
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT_GAIN",                // Parameter ID
        "Output Gain",                // Display Name
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f                          // default dB
    ));

    return { params.begin(), params.end() };
}


LimiterpluginAudioProcessor::LimiterpluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Convert the threshold from decibels to linear amplitude
    thresholdLin = std::pow(10.0f, thresholdDb / 20.0f);

}

LimiterpluginAudioProcessor::~LimiterpluginAudioProcessor()
{
}

const juce::String LimiterpluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LimiterpluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LimiterpluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LimiterpluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LimiterpluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LimiterpluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LimiterpluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LimiterpluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LimiterpluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void LimiterpluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void LimiterpluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    // Store the sample rate
    currentSampleRate = static_cast<float> (sampleRate);

    // Convert the threshold from dB to linear again, in case the user changes it later
    thresholdLin = std::pow(10.0f, thresholdDb / 20.0f);

    // Compute the release coefficient
    const float releaseTimeSeconds = releaseTimeMs / 1000.0f;

    if (releaseTimeSeconds > 0.0f)
        releaseCoeff = std::exp(-1.0f / (releaseTimeSeconds * currentSampleRate));
    else
        releaseCoeff = 0.0f;

    // Make one gain envelope per output channel, starting at 1.0f (no gain change)
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    gainEnvelope.assign((size_t)totalNumOutputChannels, 1.0f);
}

void LimiterpluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LimiterpluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void LimiterpluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // Read parameter from APVTS
    const float paramThresholdDb = parameters.getRawParameterValue("THRESHOLD")->load();

    // Recompute linear threshold if value changed
    if (paramThresholdDb != lastThresholdDb)
    {
        thresholdDb = paramThresholdDb;
        thresholdLin = std::pow(10.0f, thresholdDb / 20.0f);
        lastThresholdDb = paramThresholdDb;
    }

    // Read output gain and cache linear
    const float paramOutGainDb = parameters.getRawParameterValue("OUTPUT_GAIN")->load();
    if (paramOutGainDb != lastOutputGainDb)
    {
        outputGainLin = std::pow(10.0f, paramOutGainDb / 20.0f);
        lastOutputGainDb = paramOutGainDb;
    }


    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();


    // i -> ch for more applicable documentation
    for (auto ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());


    // Safety check: make sure we have one gain envelope per channel
    if ((int)gainEnvelope.size() < totalNumOutputChannels)
        gainEnvelope.assign((size_t)totalNumOutputChannels, 1.0f);

    float blockMaxGRdB = 0.0f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* samples = buffer.getWritePointer(channel);
        float env = gainEnvelope[(size_t)channel];

        for (int n = 0; n < numSamples; ++n)
        {
            float x = samples[n];
            float mag = std::abs(x);

            // Calculate how much we need to turn down
            float desiredGain = 1.0f;
            if (mag > thresholdLin && mag > 0.0f)
                desiredGain = thresholdLin / mag;

            // Smooth the gain
            if (desiredGain < env)
            {
                // Attack
                env = desiredGain;
            }
            else
            {
                // Release
                env = env * releaseCoeff + (1.0f - releaseCoeff) * desiredGain;
            }

            samples[n] = x * env;   // Limiter gain
            samples[n] *= outputGainLin;   // Makeup/output gain applied

            //  Compute GR dB for this sample and keep the max
            if (env < 1.0f)
            {
                const float grDb = -20.0f * std::log10(juce::jlimit(1.0e-6f, 1.0f, env));
                if (grDb > blockMaxGRdB) blockMaxGRdB = grDb;
            }
        }

        // Save this channel's envelope for next time
        gainEnvelope[(size_t)channel] = env;

    }

    // Publish the block's GR value
    currentGRdB.store(blockMaxGRdB, std::memory_order_relaxed);

}

//==============================================================================
bool LimiterpluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LimiterpluginAudioProcessor::createEditor()
{
    return new LimiterpluginAudioProcessorEditor (*this);
}

//==============================================================================
void LimiterpluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LimiterpluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LimiterpluginAudioProcessor();
}
