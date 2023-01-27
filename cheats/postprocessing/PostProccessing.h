#pragma once

struct ImDrawList;

#ifdef _WIN32
struct IDirect3DDevice9;
#endif

namespace PostProcessing
{
#ifdef _WIN32
    void setDevice(IDirect3DDevice9* device) noexcept;
    void clearBlurTextures() noexcept;
    void onDeviceReset() noexcept;
#endif
    void performFullscreenBlur(ImDrawList* drawList, float alpha) noexcept;
    void performBlur(ImDrawList* drawList, const ImVec2& p_min, const ImVec2& p_max, float alpha) noexcept;
    void performFullscreenChromaticAberration(ImDrawList* drawList, float amount) noexcept;
    void performFullscreenMonochrome(ImDrawList* drawList, float amount) noexcept;
}
