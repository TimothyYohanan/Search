#include <iostream>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "extern/imgui/imgui.h"
#include "extern/imgui/backends/imgui_impl_glfw.h"
#include "extern/imgui/backends/imgui_impl_vulkan.h"

#include "src/database.h"
#include "src/search.h"

#include "src/Graphics/Window.h"
#include "src/Graphics/VulkanDevice.h"
#include "src/Graphics/ImGuiBinding.h"


using namespace std;

static shared_ptr<GLFW_Window>    glfw_window;
static shared_ptr<VulkanDevice>   vulkan_device;
static unique_ptr<ImGui_Binding>  imgui_binding;

static const vector<pair<uint32_t, unsigned int>> glfw_window_hints = {
    pair(GLFW_CLIENT_API, GLFW_NO_API), // we aren't using opengl
    pair(GLFW_RESIZABLE, GLFW_FALSE)    // we don't want a resizable window
};  

static const ImVec2               window_size    = ImVec2(600, 400);
static const ImVec4               clear_color    = ImVec4(0.121f, 0.121f, 0.121f, 1.00f);
static const ImGuiWindowFlags     window_flags   = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration;
static const ImGuiInputTextFlags  inputTextFlags = ImGuiInputTextFlags_CallbackEdit;

void safe_early_exit(const string& Message)
{
    cerr << Message.c_str() << endl;
    glfw_window.reset();
    vulkan_device.reset();
    imgui_binding.reset();
    exit(EXIT_FAILURE);
}

int loop_gui(Search* search_prechecked) {
    
    while (!glfwWindowShouldClose(glfw_window->window)) {

        glfwPollEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::GetCurrentContext() != NULL)
        {
            const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

            ImVec2 w_pos = ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y);
            ImVec2 w_size = ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y);

            ImGui::SetNextWindowPos(w_pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(w_size, ImGuiCond_Always);

            ImGui::Begin("Search", nullptr, window_flags);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 5.0f));

            const float search_bar_width = w_size.x * 0.8f;

            const float x_pos = (w_size.x - search_bar_width) * 0.5f;

            ImGui::SetNextItemWidth(search_bar_width);
            ImGui::SetCursorPosX(x_pos);

            ImGui::InputText("##Search Bar", search_prechecked->SearchBarBuffer, IM_ARRAYSIZE(search_prechecked->SearchBarBuffer), inputTextFlags, Search::searchBarInputCallback);

            ImGui::PopStyleVar(3);

            ImGui::SetCursorPosX(x_pos);

            ImVec2 results_area_size = ImVec2(search_bar_width, 350.0f);
            ImGui::BeginChild("##SearchResults", results_area_size, true, ImGuiWindowFlags_NoBackground);

            for (const auto& result : Search::SearchResults) {
                ImGui::Text("%s", result.c_str());
            }

            ImGui::EndChild();

            ImGui::End();

        } else
        {
            safe_early_exit("loop_gui() - Error: missing ImGui context!");
        }

        // Rendering
        ImGui::Render();

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data)
        {
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

            if (!is_minimized) {
                vulkan_device->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
                vulkan_device->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
                vulkan_device->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
                vulkan_device->ClearValue.color.float32[3] = clear_color.w;
                imgui_binding->RenderFrame(draw_data);
                imgui_binding->PresentFrame();
            }
        }
    }

    VkResult err;

    err = vkDeviceWaitIdle(vulkan_device->Device);
    vulkan_device->check_vk_result_dev_friendly(err, "loop_gui");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

int main() {
    Database* db = Database::Get();
    Search* search = Search::Get();
    
    if (db->isValid() && search != nullptr)
    {
        glfw_window = make_shared<GLFW_Window>("Search - demo", window_size.x, window_size.y, glfw_window_hints);

        if (!glfw_window) { safe_early_exit("Error: glfw_window is a nullptr!"); }

        if (!glfwVulkanSupported()) { safe_early_exit("Error: GLFW not supported by Vulkan."); }

        ImVector<const char*> glfw_required_extensions = glfw_window->requiresExtensions_Vulkan();

        vulkan_device = make_shared<VulkanDevice>(glfw_required_extensions, glfw_window->window);

        if (!vulkan_device) { safe_early_exit("Error: vulkan_device is a nullptr!"); }

        imgui_binding = make_unique<ImGui_Binding>(glfw_window, vulkan_device);

        if (!imgui_binding) { safe_early_exit("Error: imgui_binding is a nullptr!"); }

        loop_gui(search);

        Database::Destroy();
        Search::Destroy();

        imgui_binding.reset();
        glfw_window.reset();
        vulkan_device.reset();
        
        return EXIT_SUCCESS;
    } else
    {
        Database::Destroy();
        Search::Destroy();

        imgui_binding.reset();
        glfw_window.reset();
        vulkan_device.reset();
        
        return EXIT_FAILURE;
    }
}

