#include "PluginEditor.h"

#include <cstring>
#include <vector>

#include <BinaryData.h>

//==============================================================================
VocalSatAudioProcessorEditor::VocalSatAudioProcessorEditor (VocalSatAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setOpaque (true);

    // CRITICAL: relays are members and already constructed; the WebView is
    // created with .withOptionsFrom() for each of them, attachments after.
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options()
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory))
                    .withBackgroundColour (juce::Colour (0xff2a2621))) // plate colour, kills drag flash
            .withNativeIntegrationEnabled()
            .withOptionsFrom (driveRelay)
            .withOptionsFrom (toneRelay)
            .withOptionsFrom (mixRelay)
            .withOptionsFrom (bypassRelay)
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
            // Preset UI scales (0.5x .. 3x), aspect locked.
            .withNativeFunction ("setUiScale",
                [this] (const juce::Array<juce::var>& args,
                        juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.size() > 0)
                        setUiScaleAndSize ((double) args[0]);
                    complete ({});
                })
            // In-page corner grip (apc-fit.js forwards accumulated deltas).
            .withNativeFunction ("resizeGripStart",
                [this] (const juce::Array<juce::var>&,
                        juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    gripStartW = getWidth();
                    gripStartH = getHeight();
                    complete ({});
                })
            .withNativeFunction ("resizeGrip",
                [this] (const juce::Array<juce::var>& args,
                        juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.size() >= 1 && gripStartW > 0)
                    {
                        const int lo = juce::roundToInt (kPlateW * kMinUiScale);
                        const int hi = juce::roundToInt (kPlateW * kMaxUiScale);
                        const int newW = juce::jlimit (lo, hi, gripStartW + (int) args[0]);
                        setSize (newW, juce::roundToInt (newW / kAspect));
                    }
                    complete ({});
                })
            .withNativeFunction ("resizeGripEnd",
                [this] (const juce::Array<juce::var>&,
                        juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    gripStartW = gripStartH = 0;
                    complete ({});
                })
            .withNativeFunction ("getUiPrefs",
                [this] (const juce::Array<juce::var>&,
                        juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    auto* prefs = new juce::DynamicObject();
                    prefs->setProperty ("uiScale",
                        (double) audioProcessor.apvts.state.getProperty ("uiScale", 1.0));
                    complete (juce::var (prefs));
                })
            // Meter feed for the VU canvas (30 Hz poll from the page).
            .withNativeFunction ("getMeter",
                [this] (const juce::Array<juce::var>&,
                        juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    auto* meter = new juce::DynamicObject();
                    meter->setProperty ("rmsDb",  (double) audioProcessor.getMeterRmsDb());
                    meter->setProperty ("peakDb", (double) audioProcessor.getMeterPeakDb());
                    complete (juce::var (meter));
                }));

    addAndMakeVisible (*webView);

    // CRITICAL: attachments after the WebView (they reference relays + params).
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter (ParameterIDs::DRIVE), driveRelay, nullptr);
    toneAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter (ParameterIDs::TONE), toneRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter (ParameterIDs::MIX), mixRelay, nullptr);
    bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter (ParameterIDs::BYPASS), bypassRelay, nullptr);

    // Scalable window: in-page corner grip + preset scales, aspect locked.
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (kPlateW * kMinUiScale),
                     juce::roundToInt (kPlateH * kMinUiScale),
                     juce::roundToInt (kPlateW * kMaxUiScale),
                     juce::roundToInt (kPlateH * kMaxUiScale));
    if (auto* bc = getConstrainer())
        bc->setFixedAspectRatio (kAspect);

    const double uiScale = juce::jlimit (kMinUiScale, kMaxUiScale,
        (double) audioProcessor.apvts.state.getProperty ("uiScale", 1.0));
    setSize (juce::roundToInt (kPlateW * uiScale),
             juce::roundToInt (kPlateH * uiScale));

    // Embedded content via the resource provider (never a data URI).
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
}

VocalSatAudioProcessorEditor::~VocalSatAudioProcessorEditor() = default;

//==============================================================================
void VocalSatAudioProcessorEditor::setUiScaleAndSize (double s)
{
    s = juce::jlimit (kMinUiScale, kMaxUiScale, s);
    audioProcessor.apvts.state.setProperty ("uiScale", s, nullptr);
    setSize (juce::roundToInt (kPlateW * s), juce::roundToInt (kPlateH * s));
}

void VocalSatAudioProcessorEditor::snapWindowToAspect()
{
    // Host-initiated resizes bypass the constrainer; snap the window back so
    // the webview viewport aspect always matches the authored plate.
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const double current = (double) getWidth() / (double) getHeight();
    if (std::abs (current - kAspect) < 0.001)
        return;

    setSize (getWidth(), juce::roundToInt (getWidth() / kAspect));
}

//==============================================================================
void VocalSatAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2a2621));
}

void VocalSatAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    snapWindowToAspect();
}

//==============================================================================
// Resource provider
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
VocalSatAudioProcessorEditor::getResource (const juce::String& url)
{
    auto resourcePath = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);

    if (resourcePath.isEmpty() || resourcePath == "/")
        resourcePath = "/index.html";

    const auto path = resourcePath.substring (1);

    if (path == "index.html" || path.isEmpty())
    {
        int size = 0;
        if (const char* data = BinaryData::getNamedResource ("index_html", size))
        {
            if (size > 0)
            {
                std::vector<std::byte> bytes ((size_t) size);
                std::memcpy (bytes.data(), data, (size_t) size);
                return juce::WebBrowserComponent::Resource {
                    std::move (bytes), juce::String ("text/html")
                };
            }
        }
    }

    return std::nullopt;
}
