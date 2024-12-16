#include <iostream>

#include "Window.h"

GLFW_Window::GLFW_Window(string name, uint16_t width, uint16_t height, vector<pair<uint32_t, unsigned int>> window_hints)
{
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    
    for(pair<uint32_t, unsigned int> hint : window_hints) 
    {
        glfwWindowHint(hint.first, hint.second);
    }
    
    // https://www.glfw.org/docs/latest/window_guide.html#window_hints
    
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);                // resizable
    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);             // "do not create openGL context"
    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);  // transparent framebuffer
    // glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);                // Fullscreen hint
    // glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);                // Minimizable hint

    // Commonly Used:
    // GLFW_RESIZABLE:     Controls whether the window can be resized (GLFW_TRUE or GLFW_FALSE).
    // GLFW_VISIBLE:       Controls the initial visibility of the window (GLFW_TRUE or GLFW_FALSE).
    // GLFW_DECORATED:     Controls whether the window has a border and title bar (GLFW_TRUE or GLFW_FALSE).
    // GLFW_FOCUSED:       Controls whether the window is initially focused (GLFW_TRUE or GLFW_FALSE).
    // GLFW_AUTO_ICONIFY:  Controls whether the window will automatically iconify (minimize) when it loses focus (GLFW_TRUE or GLFW_FALSE).
    // GLFW_FLOATING:      Controls whether the window should be always on top of other windows (GLFW_TRUE or GLFW_FALSE).
    // GLFW_MAXIMIZED:     Controls whether the window is maximized when created (GLFW_TRUE or GLFW_FALSE).
    // GLFW_CENTER_CURSOR: Controls whether the cursor is initially positioned at the center of the window (GLFW_TRUE or GLFW_FALSE).

    // Less Commonly Used:
    // GLFW_SAMPLES: Number of samples for multisampling (anti-aliasing) (e.g., glfwWindowHint(GLFW_SAMPLES, 4)).
    // GLFW_REFRESH_RATE: The desired refresh rate for fullscreen windows (e.g., glfwWindowHint(GLFW_REFRESH_RATE, 60)).
    // GLFW_CONTEXT_VERSION_MAJOR and GLFW_CONTEXT_VERSION_MINOR: Specify the desired OpenGL context version (e.g., glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4)).
    // GLFW_OPENGL_PROFILE: Specify the desired OpenGL profile (e.g., glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE)).
    // GLFW_STEREO: Controls whether the window should be stereoscopic (3D) (GLFW_TRUE or GLFW_FALSE).

    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), name.c_str(), nullptr, nullptr);

    if (!window) {
        cerr << "GLFW_Window::GLFW_Window() - Error: window is a nullptr!" << endl;
        exit(EXIT_FAILURE);
    }
};

GLFW_Window::~GLFW_Window () 
{
    glfwDestroyWindow(window);
    glfwTerminate();

    window = nullptr;
}

void GLFW_Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const
{
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, surface);

    if (result != VK_SUCCESS) {
        cerr << "GLFW_Window::createWindowSurface() - Error: failed to create window surface!" << endl;
        exit(EXIT_FAILURE);
    }
}

void GLFW_Window::glfw_error_callback(int error, const char* description) 
{
    fprintf(stderr, "GLFW_Window::glfw_error_callback() - Error code %d: %s\n", error, description);
}

ImVector<const char*> GLFW_Window::requiresExtensions_Vulkan() const
{
    ImVector<const char*> required_extensions;

    uint32_t extensions_ct = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_ct);

    for (uint32_t i = 0; i < extensions_ct; i++) 
    {
        required_extensions.push_back(glfw_extensions[i]);
    }

    return required_extensions;
}