#pragma once

/*
    Stable parameter identifiers for VocalSat.

    These strings are part of the plugin's public contract: they are used by
    APVTS, the WebView relays (Juce.getSliderState("drive") ...) and the
    ui-map contract (Design/v2-ui-map.json -> controls[].param).
    Never rename them without a /apc-evolve round + design revision.
*/
namespace ParameterIDs
{
    inline constexpr const char* DRIVE  = "drive";
    inline constexpr const char* TONE   = "tone";
    inline constexpr const char* MIX    = "mix";
    inline constexpr const char* BYPASS = "bypass";
}
