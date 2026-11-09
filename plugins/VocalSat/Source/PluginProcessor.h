#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/** VocalSat - vocal-first analog saturation. */
class VocalSatAudioProcessor : public juce::AudioProcessor
{
public:
    VocalSatAudioProcessor();
    ~VocalSatAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "VocalSat"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    //==============================================================================
    float getMeterRmsDb()  const noexcept { return meterRmsDb.load (std::memory_order_relaxed); }
    float getMeterPeakDb() const noexcept { return meterPeakDb.load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ---- DSP state (allocated in prepareToPlay) -----------------------------
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int oversamplingFactor = 2; // 2^2 = 4x
    int oversamplingLatency = 0;

    // DC blocker (20 Hz HPF) per channel
    struct DcState { float x1 = 0.0f; float y1 = 0.0f; };
    std::vector<DcState> dcStates;

    // Dry-path delay (latency compensation for parallel mix)
    juce::AudioBuffer<float> dryDelay;
    int delayWritePos = 0;
    int delaySize = 0;

    // Smoothed parameters
    juce::SmoothedValue<float> driveSmooth;   // dB
    juce::SmoothedValue<float> toneSmooth;    // -100..+100 %
    juce::SmoothedValue<float> mixSmooth;     // 0..100 %
    juce::SmoothedValue<float> bypassSmooth;  // 0..1
    juce::SmoothedValue<float> compSmooth;    // auto-gain (linear)

    // Tone tilt EQ
    juce::dsp::IIR::Filter<float> lowShelf;
    juce::dsp::IIR::Filter<float> highShelf;

    void updateToneCoefficients (float tonePercent, double sampleRate);

    // Metering
    std::atomic<float> meterRmsDb  { -100.0f };
    std::atomic<float> meterPeakDb { -100.0f };

    // Per-block scratch (sized in prepareToPlay)
    std::vector<float> driveDbArr;
    std::vector<float> toneArr;
    std::vector<float> mixArr;
    std::vector<float> bypassArr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSatAudioProcessor)
};