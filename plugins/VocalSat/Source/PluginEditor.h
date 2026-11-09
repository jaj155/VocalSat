#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
/**
    WebView2 editor for VocalSat.

    The layout contract is Design/v2-ui-map.json (plate 45 x 30 cells =
    720 x 480 px, scalable, aspect locked, in-page corner gripper).
    Source/ui/public/index.html is composed from that contract; do not add
    controls that are not in the map.
*/
class VocalSatAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalSatAudioProcessorEditor (VocalSatAudioProcessor&);
    ~VocalSatAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalSatAudioProcessor& audioProcessor;

    // UI layout contract (Design/v2-ui-map.json)
    static constexpr int    kPlateW     = 720;   // 45 cells * 16 px
    static constexpr int    kPlateH     = 480;   // 30 cells * 16 px
    static constexpr double kAspect     = (double) kPlateW / (double) kPlateH;
    static constexpr double kMinUiScale = 0.5;
    static constexpr double kMaxUiScale = 3.0;

    void setUiScaleAndSize (double s);
    void snapWindowToAspect();

    int gripStartW = 0;
    int gripStartH = 0;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member Declaration Order (Prevents DAW Crashes)
    //   Relays -> WebView -> Attachments
    // C++ destroys members in reverse order; the WebView references the
    // relays via .withOptionsFrom(), so it must die before them.
    // ═══════════════════════════════════════════════════════════════════

    // 1. PARAMETER RELAYS FIRST (destroyed last)
    juce::WebSliderRelay driveRelay  { ParameterIDs::DRIVE };
    juce::WebSliderRelay toneRelay   { ParameterIDs::TONE };
    juce::WebSliderRelay mixRelay    { ParameterIDs::MIX };
    juce::WebToggleButtonRelay bypassRelay { ParameterIDs::BYPASS };

    // 2. WEBVIEW SECOND (destroyed middle)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. PARAMETER ATTACHMENTS LAST (destroyed first)
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;

    // Resource provider (embedded index.html via juce_add_binary_data)
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSatAudioProcessorEditor)
};
