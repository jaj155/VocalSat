#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

#include <cmath>

// Forward declarations
static float compGain (float driveDb);

//==============================================================================
VocalSatAudioProcessor::VocalSatAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
VocalSatAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::DRIVE, 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.01f), 6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::TONE, 1 }, "Tone",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::MIX, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::BYPASS, 1 }, "Bypass", false,
        juce::AudioParameterBoolAttributes().withAutomatable (false)));

    return { params.begin(), params.end() };
}

//==============================================================================
void VocalSatAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Oversampler: 4x (factor 2), polyphase IIR, max quality.
    const int numCh = juce::jmax (1, getTotalNumInputChannels());
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, (size_t) oversamplingFactor,
        juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
        true, true);
    oversampler->initProcessing ((size_t) samplesPerBlock);
    oversamplingLatency = (int) oversampler->getLatencyInSamples();
    setLatencySamples (oversamplingLatency);

    // DC blocker state.
    dcStates.assign ((size_t) numCh, DcState {});

    // Dry-path delay buffer (latency compensation for parallel mix).
    delaySize = oversamplingLatency + samplesPerBlock + 8;
    if (delaySize < 1) delaySize = 1;
    dryDelay.setSize (numCh, delaySize);
    dryDelay.clear();
    delayWritePos = 0;

    // Smoothed parameters (~20 ms).
    const double smoothMs = 20.0;
    driveSmooth.reset (sampleRate, smoothMs / 1000.0);
    toneSmooth.reset  (sampleRate, smoothMs / 1000.0);
    mixSmooth.reset   (sampleRate, smoothMs / 1000.0);
    bypassSmooth.reset (sampleRate, 10.0 / 1000.0); // ~10 ms crossfade
    compSmooth.reset  (sampleRate, smoothMs / 1000.0);

    // Seed from current parameter values.
    const float driveDb = *apvts.getRawParameterValue (ParameterIDs::DRIVE);
    const float toneV  = *apvts.getRawParameterValue (ParameterIDs::TONE);
    const float mixV   = *apvts.getRawParameterValue (ParameterIDs::MIX);
    const bool  bp     = apvts.getRawParameterValue (ParameterIDs::BYPASS)->load() > 0.5f;

    driveSmooth.setCurrentAndTargetValue (driveDb);
    toneSmooth.setCurrentAndTargetValue  (toneV);
    mixSmooth.setCurrentAndTargetValue   (mixV);
    bypassSmooth.setCurrentAndTargetValue (bp ? 1.0f : 0.0f);
    compSmooth.setCurrentAndTargetValue  (compGain (driveDb));

    // Tone tilt EQ.
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) numCh };
    lowShelf.prepare (spec);
    highShelf.prepare (spec);
    updateToneCoefficients (toneV, sampleRate);

    // Scratch buffers.
    driveDbArr.assign ((size_t) samplesPerBlock, 0.0f);
    toneArr.assign    ((size_t) samplesPerBlock, 0.0f);
    mixArr.assign     ((size_t) samplesPerBlock, 0.0f);
    bypassArr.assign  ((size_t) samplesPerBlock, 0.0f);

    meterRmsDb.store  (-100.0f, std::memory_order_relaxed);
    meterPeakDb.store (-100.0f, std::memory_order_relaxed);
}

void VocalSatAudioProcessor::releaseResources()
{
    oversampler.reset();
    dryDelay.setSize (0, 0);
    dcStates.clear();
}

bool VocalSatAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::mono()
        || mainIn == juce::AudioChannelSet::stereo();
}

//==============================================================================
// Helper: auto-gain compensation curve.
static float compGain (float driveDb)
{
    const float preGain = std::pow (10.0f, driveDb / 20.0f);
    // Empirical: keep perceived loudness roughly constant across Drive.
    return 1.0f / std::pow (preGain, 0.55f);
}

void VocalSatAudioProcessor::updateToneCoefficients (float tonePercent, double sampleRate)
{
    const float t = tonePercent / 100.0f;
    const float lowGain  = -t * 4.5f;   // dB
    const float highGain =  t * 4.5f;   // dB

    lowShelf.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, 700.0, 0.7f, juce::Decibels::decibelsToGain (lowGain));
    highShelf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 700.0, 0.7f, juce::Decibels::decibelsToGain (highGain));
}

//==============================================================================
void VocalSatAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const int numCh = juce::jmax (1, buffer.getNumChannels());

    // ---- read parameter targets -------------------------------------------
    const float driveTarget = *apvts.getRawParameterValue (ParameterIDs::DRIVE);
    const float toneTarget  = *apvts.getRawParameterValue (ParameterIDs::TONE);
    const float mixTarget   = *apvts.getRawParameterValue (ParameterIDs::MIX);
    const bool  bypassOn    = apvts.getRawParameterValue (ParameterIDs::BYPASS)->load() > 0.5f;

    driveSmooth.setTargetValue (driveTarget);
    toneSmooth.setTargetValue  (toneTarget);
    mixSmooth.setTargetValue   (mixTarget);
    bypassSmooth.setTargetValue (bypassOn ? 1.0f : 0.0f);
    compSmooth.setTargetValue  (compGain (driveTarget));

    // ---- per-sample smoothing + DC blocker + dry capture -------------------
    if ((int) driveDbArr.size() < numSamples)
    {
        driveDbArr.assign ((size_t) numSamples, 0.0f);
        toneArr.assign    ((size_t) numSamples, 0.0f);
        mixArr.assign     ((size_t) numSamples, 0.0f);
        bypassArr.assign  ((size_t) numSamples, 0.0f);
    }

    for (int n = 0; n < numSamples; ++n)
    {
        driveDbArr[(size_t) n] = driveSmooth.getNextValue();
        toneArr[(size_t) n]    = toneSmooth.getNextValue();
        mixArr[(size_t) n]     = mixSmooth.getNextValue();
        bypassArr[(size_t) n]  = bypassSmooth.getNextValue();
    }

    // DC blocker on input (host rate), per channel.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& st = dcStates[(size_t) ch];
        const float R = std::exp (-2.0f * juce::MathConstants<float>::pi * 20.0f
                                  / (float) getSampleRate());
        for (int n = 0; n < numSamples; ++n)
        {
            const float x = data[n];
            const float y = x - st.x1 + R * st.y1;
            st.x1 = x;
            st.y1 = y;
            data[n] = y;
        }
    }

    // Capture dry (delay-compensated path) into delay buffer.
    if (delaySize > 0)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            for (int ch = 0; ch < numCh; ++ch)
                dryDelay.setSample (ch, (delayWritePos + n) % delaySize,
                                    buffer.getSample (ch, n));
        }
        delayWritePos = (delayWritePos + numSamples) % delaySize;
    }

    // ---- oversample + saturate --------------------------------------------
    if (oversampler != nullptr)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto osBlock = oversampler->processSamplesUp (block);

        const int osSamples = (int) osBlock.getNumSamples();
        const int factor = (int) oversampler->getOversamplingFactor();

        for (int ch = 0; ch < numCh; ++ch)
        {
            for (int m = 0; m < osSamples; ++m)
            {
                const int hostIdx = juce::jmin (numSamples - 1, m / factor);
                const float driveDb = driveDbArr[(size_t) hostIdx];
                const float preGain = std::pow (10.0f, driveDb / 20.0f);
                const float k = 1.0f + 0.5f * (driveDb / 24.0f);
                const float bias = 0.08f;

                float x = osBlock.getSample (ch, m);
                float xd = x * preGain;
                float y = (std::tanh (k * (xd + bias)) - std::tanh (k * bias)) / k;
                osBlock.setSample (ch, m, y);
            }
        }

        oversampler->processSamplesDown (block);
    }

    // ---- tone tilt EQ (host rate) -----------------------------------------
    updateToneCoefficients (toneArr.empty() ? 0.0f : toneArr.back(), getSampleRate());

    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        lowShelf.process (ctx);
        highShelf.process (ctx);
    }

    // ---- dry/wet mix + auto gain + bypass ---------------------------------
    const int latency = oversamplingLatency;

    for (int n = 0; n < numSamples; ++n)
    {
        const float mixV    = mixArr[(size_t) n] / 100.0f;
        const float bypassV = bypassArr[(size_t) n];

        // Auto-gain compensation smoothed toward the per-sample target.
        compSmooth.setTargetValue (compGain (driveDbArr[(size_t) n]));
        const float compNow = compSmooth.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float wet = buffer.getSample (ch, n);

            int readIdx = ((delayWritePos - numSamples + n - latency) % delaySize + delaySize) % delaySize;
            const float dry = (delaySize > 0) ? dryDelay.getSample (ch, readIdx) : wet;

            const float processed = (dry * (1.0f - mixV) + wet * mixV) * compNow;
            const float finalOut = processed * (1.0f - bypassV) + dry * bypassV;
            buffer.setSample (ch, n, finalOut);
        }
    }

    // ---- metering ---------------------------------------------------------
    float sumSq = 0.0f;
    float peak  = 0.0f;
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            const auto s = data[n];
            sumSq += s * s;
            peak = juce::jmax (peak, std::abs (s));
        }
    }
    const auto rms = std::sqrt (sumSq / (float) (numSamples * numCh));
    meterRmsDb.store  (juce::Decibels::gainToDecibels (rms,  -100.0f), std::memory_order_relaxed);
    meterPeakDb.store (juce::Decibels::gainToDecibels (peak, -100.0f), std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor* VocalSatAudioProcessor::createEditor()
{
    return new VocalSatAudioProcessorEditor (*this);
}

//==============================================================================
void VocalSatAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalSatAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorParameter* VocalSatAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter (ParameterIDs::BYPASS);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalSatAudioProcessor();
}