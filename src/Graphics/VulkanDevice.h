#pragma once

#include <vulkan/vulkan.h>

#include "../../extern/imgui/imgui.h"

using namespace std;


//#define IMGUI_UNLIMITED_FRAME_RATE
// #ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
// #endif

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);
#endif // IMGUI_VULKAN_DEBUG_REPORT

class ImGui_Binding;

struct Vulkan_Frame {
    VkCommandPool       CommandPool;
    VkCommandBuffer     CommandBuffer;
    VkFence             Fence;
    VkImage             Backbuffer;
    VkImageView         BackbufferView;
    VkFramebuffer       Framebuffer;
};

struct Vulkan_FrameSemaphores {
    VkSemaphore         ImageAcquiredSemaphore;
    VkSemaphore         RenderCompleteSemaphore;
};

class VulkanDevice {
public:
    friend class ImGui_Binding;

    VulkanDevice(ImVector<const char*> instance_extensions, GLFWwindow* window);
    ~VulkanDevice();

    VkDevice        Device;

    VkClearValue    ClearValue;
    uint32_t        FrameIndex;  // Current frame being rendered to (0 <= FrameIndex < FrameInFlightCount)
    
    int             MinImageCount;
    bool            SwapchainRebuild;

    /*
    * DO NOT USE THIS FUNCTION UNLESS NECESSARY
    * Use the one below, and pass it a CallerMsg.
    * That way, you have more information when something breaks.
    */
    static void     check_vk_result(VkResult err);

    static void     check_vk_result_dev_friendly(VkResult err, const string CallerMsg);

    static void     ConsoleLogAvailableInstanceExtensionProperties(const char *pLayerName = nullptr);

    static bool     IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension);

    static void     ConsoleLogAvailableInstanceLayerProperties();

    virtual void    create_or_resizeWindow(uint16_t width, uint16_t height, uint32_t min_image_count);

protected:
    VkAllocationCallbacks*    AllocationCallbacks;
    VkInstance                Instance;
    VkPhysicalDevice          PhysicalDevice;      // !!! NOTE: This is not explicitely cleaned up in the destructor. This is not production code. !!!
    string                    PhysicalDeviceName;
    uint32_t                  QueueFamily;
    VkQueue                   Queue;               // !!! NOTE: This is not explicitely cleaned up in the destructor. This is not production code. !!!
    VkDebugReportCallbackEXT  DebugReportCallbackEXT;
    VkPipelineCache           PipelineCache;
    VkCommandPool             CommandPool;
    VkDescriptorPool          DescriptorPool;

    VkSurfaceKHR              SurfaceKHR;
    VkSurfaceFormatKHR        SurfaceFormatKHR;
    VkPresentModeKHR          PresentModeKHR;
    VkSwapchainKHR            SwapchainKHR;        // !!! NOTE: This is not explicitely cleaned up in the destructor. This is not production code. !!!
    VkRenderPass              RenderPass;
    VkPipeline                Pipeline;            // The window pipeline may uses a different VkRenderPass than the one passed in ImGui_ImplVulkan_InitInfo
    bool                      ClearEnable;

    uint32_t                  ImageCount;          // Number of simultaneous in-flight frames (returned by vkGetSwapchainImagesKHR, usually derived from MinImageCount)
    uint32_t                  SemaphoreIndex;      // Current set of swapchain wait semaphores we're using (needs to be distinct from per frame data)
    Vulkan_Frame*             Frames;              // Careful. There's a bit of magic going on here. We're keeping track of the ImageCount in order to manage this memory. I would do this differently for a production gui.
    Vulkan_FrameSemaphores*   FrameSemaphores;     // Same note as with Frames.

    uint16_t                  Width;
    uint16_t                  Height;

    virtual uint32_t          findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    virtual void              createWindowSurface(GLFWwindow* window);

    virtual void              createCommandPool();

    virtual void              allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;

    virtual VkCommandBuffer   beginSingleTimeCommands(VkCommandPool& commandPool) const;

    virtual void              copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkCommandPool& command_pool, VkDeviceSize size) const;

    virtual void              endSingleTimeCommands(VkCommandBuffer& commandBuffer, VkCommandPool& commandPool) const;

    virtual void              createWindowSwapChain(uint16_t width, uint16_t height, uint32_t min_image_count);

    virtual void              createWindowCommandBuffers();

    virtual void              destroyFrame(Vulkan_Frame* p);

    virtual void              destroyFrameSemaphores(Vulkan_FrameSemaphores* p);

private:
    /*
    * This is an important one.
    * If you know what platform you're targetting, there are things to change here.
    * There are some hints (commented out lines) in the function definition.
    */
    virtual void              createInstance(ImVector<const char*>& instance_extensions);

    virtual void              selectPhysicalDevice();

    virtual void              selectGraphicsQueueFamily();

    virtual void              createLogicalDevice();

    virtual void              createDescriptorPool();

    virtual void              selectSurfaceFormat();
    
    virtual void              selectPresentMode();
};

