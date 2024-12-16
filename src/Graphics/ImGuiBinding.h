#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Window.h"
#include "VulkanDevice.h"

using namespace std;

class ImGui_Binding {
public:
    ImGui_Binding(shared_ptr<GLFW_Window> window, shared_ptr<VulkanDevice> device, vector<string> ttf_font_file_paths = vector<string>(), float font_size = 16.0f);
    ~ImGui_Binding();

    virtual void RenderFrame(ImDrawData* draw_data) const;
    virtual void PresentFrame() const;
    
protected:
    shared_ptr<VulkanDevice>  vulkan_device;
    shared_ptr<GLFW_Window>   glfw_window;

    /*
    * Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    * Read imgui 'docs/FONTS.md' for more instructions and details.
    */
    virtual void setContext(vector<string> ttf_font_file_paths, float font_size) const;
};