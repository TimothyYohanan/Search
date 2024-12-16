#include <iostream>
#include <cstdlib>

#include "ImGuiBinding.h"

#include "../../extern/imgui/backends/imgui_impl_glfw.h"
#include "../../extern/imgui/backends/imgui_impl_vulkan.h"


ImGui_Binding::ImGui_Binding(shared_ptr<GLFW_Window> window, shared_ptr<VulkanDevice> device, vector<string> ttf_font_file_paths, float font_size)  : vulkan_device{device}, glfw_window{window}
{
    if (!vulkan_device) {
        cerr << "ImGui_Binding::ImGui_Binding() - Error: vulkan_device is a nullptr!" << endl;
        exit(EXIT_FAILURE);
    }
    
    if (!glfw_window) {
        cerr << "ImGui_Binding::ImGui_Binding() - Error: glfw_window is a nullptr!" << endl;
        exit(EXIT_FAILURE);
    }

    setContext(ttf_font_file_paths, font_size);
}

ImGui_Binding::~ImGui_Binding() 
{
    vulkan_device.reset();
    glfw_window.reset();
}

void ImGui_Binding::RenderFrame(ImDrawData* draw_data) const 
{
    VkResult err = VK_SUCCESS;

    VkSemaphore image_acquired_semaphore  = vulkan_device->FrameSemaphores[vulkan_device->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = vulkan_device->FrameSemaphores[vulkan_device->SemaphoreIndex].RenderCompleteSemaphore;

    err = vkAcquireNextImageKHR(vulkan_device->Device, vulkan_device->SwapchainKHR, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &vulkan_device->FrameIndex);

    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        vulkan_device->SwapchainRebuild = true;
        return;
    }

    vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [1]");

    /*
    * POTENTIALLY DANGEROUS.
    *
    * I'm reviewing this code a long time after originally working it, and I'm noticing this pointer is not explicitely checked.
    * I recall doing a lot of copying and pasting from imgui's own examples, and I *think* this is something that I copied from their examples.
    * 
    * The first few times it is used is preceded by 'err = ...,' which seems good.
    * 
    * Starting at VkRenderPassBeginInfo info, it's not obvious to me that it is being checked any more.
    * 
    * The gui is not too important for this project, and I'm not going to dig into this now.
    * 
    * Just wanted to put a note of warning here in case I publish this.
    */
    Vulkan_Frame* fd = &vulkan_device->Frames[vulkan_device->FrameIndex];

    {
        err = vkWaitForFences(vulkan_device->Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [2]");

        err = vkResetFences(vulkan_device->Device, 1, &fd->Fence);
        vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [3]");
    }
    {
        err = vkResetCommandPool(vulkan_device->Device, fd->CommandPool, 0);
        vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [4]");
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [5]");
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = vulkan_device->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = vulkan_device->Width;
        info.renderArea.extent.height = vulkan_device->Height;
        info.clearValueCount = 1;
        info.pClearValues = &vulkan_device->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [6]");
        err = vkQueueSubmit(vulkan_device->Queue, 1, &info, fd->Fence);
        vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::RenderFrame() [7]");
    }
}

void ImGui_Binding::PresentFrame() const 
{
    VkResult err = VK_SUCCESS;

    if (vulkan_device->SwapchainRebuild)
    {
        return;
    }
        
    VkSemaphore render_complete_semaphore = vulkan_device->FrameSemaphores[vulkan_device->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &vulkan_device->SwapchainKHR;
    info.pImageIndices = &vulkan_device->FrameIndex;
    err = vkQueuePresentKHR(vulkan_device->Queue, &info);

    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        vulkan_device->SwapchainRebuild = true;
        return;
    }

    vulkan_device->check_vk_result_dev_friendly(err, "ImGui_Binding::PresentFrame()");
    vulkan_device->SemaphoreIndex = (vulkan_device->SemaphoreIndex + 1) % vulkan_device->ImageCount; // Now we can use the next set of semaphores
}

void ImGui_Binding::setContext(vector<string> ttf_font_file_paths, float font_size) const 
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    (void)io;                                                 // Suppresses warnings if io is unused
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();                                 // Setup Dear ImGui style

    // Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(glfw_window->window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkan_device->Instance;
    init_info.PhysicalDevice = vulkan_device->PhysicalDevice;
    init_info.Device = vulkan_device->Device;
    init_info.QueueFamily = vulkan_device->QueueFamily;
    init_info.Queue = vulkan_device->Queue;
    init_info.PipelineCache = vulkan_device->PipelineCache;
    init_info.DescriptorPool = vulkan_device->DescriptorPool;
    init_info.RenderPass = vulkan_device->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = vulkan_device->MinImageCount;
    init_info.ImageCount = vulkan_device->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = vulkan_device->AllocationCallbacks;
    init_info.CheckVkResultFn = vulkan_device->check_vk_result;

    ImGui_ImplVulkan_Init(&init_info);

    VkCommandPool    command_pool    =  vulkan_device->Frames[vulkan_device->FrameIndex].CommandPool;
    VkCommandBuffer  command_buffer  =  vulkan_device->beginSingleTimeCommands(command_pool);

    if (!ttf_font_file_paths.empty())
    {
        for (const string& font_file : ttf_font_file_paths)
        {
            io.Fonts->AddFontFromFileTTF(font_file.c_str(), font_size);
        }
    }

    ImGui_ImplVulkan_CreateFontsTexture();

    vulkan_device->endSingleTimeCommands(command_buffer, command_pool);
}
