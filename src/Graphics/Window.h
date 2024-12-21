#pragma once

#include <memory>
#include <string>
#include <vector>

using namespace std;

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "../../extern/imgui/imgui.h"

class GLFW_Window {
public:
    GLFW_Window(string name, uint16_t width, uint16_t height, vector<pair<uint32_t, unsigned int>> window_hints);
    virtual ~GLFW_Window ();

    GLFWwindow *window;

    virtual void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const;

    ImVector<const char*> requiresExtensions_Vulkan() const;
    
protected:
    static void glfw_error_callback(int error, const char* description);
};

#endif
#endif