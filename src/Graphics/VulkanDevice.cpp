#include <iostream>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "VulkanDevice.h"

#include "../../extern/imgui/backends/imgui_impl_glfw.h"
#include "../../extern/imgui/backends/imgui_impl_vulkan.h"


#ifdef VULKAN_DEBUG_REPORT    // CMake: target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_DEBUG_REPORT)
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) 
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif


VulkanDevice::VulkanDevice(ImVector<const char*> instance_extensions, GLFWwindow* window) 
{
    Device                  = nullptr;

    ClearValue              = VkClearValue();
    FrameIndex              = 0;
    
    MinImageCount           = 2;
    SwapchainRebuild        = false;

    AllocationCallbacks     = nullptr;
    Instance                = nullptr;
    PhysicalDevice          = nullptr;
    PhysicalDeviceName      = "";
    QueueFamily             = (uint32_t)-1;
    Queue                   = nullptr;
    DebugReportCallbackEXT  = nullptr;
    PipelineCache           = nullptr;
    CommandPool             = nullptr;
    DescriptorPool          = nullptr;

    SurfaceKHR              = nullptr;
    SurfaceFormatKHR        = VkSurfaceFormatKHR();
    PresentModeKHR          = VK_PRESENT_MODE_IMMEDIATE_KHR;
    SwapchainKHR            = nullptr;
    RenderPass              = nullptr;
    Pipeline                = nullptr;
    ClearEnable             = true;

    ImageCount              = 0;
    SemaphoreIndex          = 0;
    Frames                  = nullptr;
    FrameSemaphores         = nullptr;

    Width                   = 0;
    Height                  = 0;

    createInstance(instance_extensions);
    createWindowSurface(window);
    selectPhysicalDevice();
    selectGraphicsQueueFamily();
    createLogicalDevice();
    createDescriptorPool();
    selectSurfaceFormat();
    selectPresentMode();

    VkBool32 supportsWindowingSystems;
    vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, QueueFamily, SurfaceKHR, &supportsWindowingSystems);
    if (supportsWindowingSystems != VK_TRUE) 
    {
        cerr << "VulkanDevice::VulkanDevice() - Error: Physical device does not support windowing systems (which is really, really strange)." << endl;
        exit(EXIT_FAILURE);
    }

    int glfw_w, glfw_h;
    glfwGetFramebufferSize(window, &glfw_w, &glfw_h);
    create_or_resizeWindow(glfw_w, glfw_h, MinImageCount);
}

VulkanDevice::~VulkanDevice() 
{
    vkDestroyDescriptorPool(Device, DescriptorPool, AllocationCallbacks);
    DescriptorPool = nullptr;

#ifdef VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(Instance, DebugReportCallbackEXT, AllocationCallbacks);
    DebugReportCallbackEXT = nullptr;
#endif

    for (uint32_t i = 0; i < ImageCount; ++i) 
    {
        vkWaitForFences(Device, 1, &Frames[i].Fence, VK_TRUE, UINT64_MAX);
        destroyFrame(&Frames[i]);
        destroyFrameSemaphores(&FrameSemaphores[i]);
    }
    Frames = nullptr;
    FrameSemaphores = nullptr;
    
    vkDestroyPipeline(Device, Pipeline, AllocationCallbacks);
    Pipeline = nullptr;

    vkDestroyPipelineCache(Device, PipelineCache, AllocationCallbacks);
    PipelineCache = nullptr;

    vkDestroyRenderPass(Device, RenderPass, AllocationCallbacks);
    RenderPass = nullptr;

    vkDestroyCommandPool(Device, CommandPool, AllocationCallbacks);
    CommandPool = nullptr;

    // vkDestroySwapchainKHR(Device, SwapchainKHR, AllocationCallbacks);
    // SwapchainKHR = nullptr;

    vkDestroyDevice(Device, AllocationCallbacks);
    Device = nullptr;

    vkDestroySurfaceKHR(Instance, SurfaceKHR, nullptr);
    SurfaceKHR = nullptr;

    vkDestroyInstance(Instance, AllocationCallbacks);
    Instance = nullptr;

    delete AllocationCallbacks;
    AllocationCallbacks = nullptr;
}

void VulkanDevice::check_vk_result(VkResult err)
{
    if (static_cast<int>(err) == 0)
    {
        return;
    }

    cerr << "VulkanDevice::check_vk_result_dev_friendly() - Error: VkResult = " << static_cast<int>(err) << endl;

    if (static_cast<int>(err) < 0)
    {
        exit(EXIT_FAILURE);
    }
}

void VulkanDevice::check_vk_result_dev_friendly(VkResult err, const string CallerMsg)
{
    if (static_cast<int>(err) == 0)
    {
        return;
    }

    cerr << "VulkanDevice::check_vk_result_dev_friendly() - " << CallerMsg.c_str() << " Error: VkResult = " << static_cast<int>(err) << endl;

    if (static_cast<int>(err) < 0)
    {
        exit(EXIT_FAILURE);
    }
}

void VulkanDevice::ConsoleLogAvailableInstanceExtensionProperties(const char *pLayerName)
{
    VkResult err = VK_SUCCESS;
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(pLayerName, &properties_count, nullptr);
    properties.resize(properties_count);
    err = vkEnumerateInstanceExtensionProperties(pLayerName, &properties_count, properties.Data);
    check_vk_result_dev_friendly(err, "VulkanDevice::ConsoleLogAvailableInstanceExtensionProperties()");

    for (const VkExtensionProperties& p : properties)
    {
        cerr << "VulkanDevice::ConsoleLogAvailableInstanceExtensionProperties() - Property Name: " << p.extensionName << endl;
    }
}

bool VulkanDevice::IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension) 
{
    /*
    This function checks if an extension is available on the host system.
        - &properties points to a list of Vulkan Extensions that are available on the system 
        - extension is the extension that you want to enable

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result_dev_friendly(err);
    */

    for (const VkExtensionProperties& p : properties)
    {
        if (strcmp(p.extensionName, extension) == 0)
        {
            return true;
        }  
    }
        
    return false;
}

void VulkanDevice::ConsoleLogAvailableInstanceLayerProperties()
{
    VkResult err = VK_SUCCESS;
    uint32_t properties_count;
    ImVector<VkLayerProperties> properties;
    vkEnumerateInstanceLayerProperties(&properties_count, nullptr);
    properties.resize(properties_count);
    err = vkEnumerateInstanceLayerProperties(&properties_count, properties.Data);
    check_vk_result_dev_friendly(err, "VulkanDevice::ConsoleLogAvailableInstanceLayerProperties()");

    for (const VkLayerProperties& p : properties)
    {
        cerr << "VulkanDevice::ConsoleLogAvailableInstanceLayerProperties() - Property Name: " << p.layerName << " Description : " << p.description << endl;
    }
}

void VulkanDevice::create_or_resizeWindow(uint16_t width, uint16_t height, uint32_t min_image_count)
{
    createWindowSwapChain(width, height, min_image_count);
    createWindowCommandBuffers();
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
  
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
        {
            return i;
        }
    }
    cerr << "VulkanDevice::findMemoryType() - Error: unable to find memory type!" << endl;
    exit(EXIT_FAILURE);
}

void VulkanDevice::createWindowSurface(GLFWwindow* window) 
{
    VkResult err = glfwCreateWindowSurface(Instance, window, AllocationCallbacks, &SurfaceKHR);
    check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSurface()");
}

void VulkanDevice::createCommandPool()
{
    VkResult err = VK_SUCCESS;
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = QueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    err = vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool);
    check_vk_result_dev_friendly(err, "VulkanDevice::createCommandPool()");
}

void VulkanDevice::allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
{
    VkResult err = VK_SUCCESS;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    err = vkCreateBuffer(Device, &bufferInfo, nullptr, &buffer);
    check_vk_result_dev_friendly(err, "VulkanDevice::allocateBuffer() [1]");
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    err = vkAllocateMemory(Device, &allocInfo, nullptr, &bufferMemory);
    check_vk_result_dev_friendly(err, "VulkanDevice::allocateBuffer() [2]");
    vkBindBufferMemory(Device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanDevice::beginSingleTimeCommands(VkCommandPool& commandPool) const
{
    VkResult err = VK_SUCCESS;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    check_vk_result_dev_friendly(err, "VulkanDevice::beginSingleTimeCommands()");
    return commandBuffer;
}

void VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkCommandPool& command_pool, VkDeviceSize size) const
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(command_pool);
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer, command_pool);
}

void VulkanDevice::endSingleTimeCommands(VkCommandBuffer& commandBuffer, VkCommandPool& commandPool) const
{
    VkResult err = VK_SUCCESS;
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(Queue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(Queue);
    vkFreeCommandBuffers(Device, commandPool, 1, &commandBuffer);
    check_vk_result_dev_friendly(err, "VulkanDevice::endSingleTimeCommands()");
}

void VulkanDevice::createWindowSwapChain(uint16_t width, uint16_t height, uint32_t min_image_count) 
{
    VkResult err = VK_SUCCESS;
    VkSwapchainKHR old_swapchain = SwapchainKHR;
    SwapchainKHR = nullptr;
    err = vkDeviceWaitIdle(Device);
    check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [1]");

    for (uint32_t i = 0; i < ImageCount; i++) 
    {
        destroyFrame(&Frames[i]);
        destroyFrameSemaphores(&FrameSemaphores[i]);
    }
    
    IM_FREE(Frames);
    IM_FREE(FrameSemaphores);
    Frames = nullptr;
    FrameSemaphores = nullptr;
    ImageCount = 0;
    if (RenderPass)
    {
        vkDestroyRenderPass(Device, RenderPass, AllocationCallbacks);
    }
        
    if (Pipeline)
    {
        vkDestroyPipeline(Device, Pipeline, AllocationCallbacks);
    }
        

    // If min image count was not specified, request different count of images dependent on selected present mode
    if (min_image_count == 0)
    {
        min_image_count = ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(PresentModeKHR);
    }
        

    // Create SwapchainKHR
    {
        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = SurfaceKHR;
        info.minImageCount = min_image_count;
        info.imageFormat = SurfaceFormatKHR.format;
        info.imageColorSpace = SurfaceFormatKHR.colorSpace;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = PresentModeKHR;
        info.clipped = VK_TRUE;
        info.oldSwapchain = old_swapchain;
        VkSurfaceCapabilitiesKHR cap;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, SurfaceKHR, &cap);
        check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [2]");

        if (info.minImageCount < cap.minImageCount)
        {
            info.minImageCount = cap.minImageCount;
        } else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
        {
            info.minImageCount = cap.maxImageCount;
        }
            
        if (cap.currentExtent.width == 0xffffffff)
        {
            Width = width;
            Height = height;
        } else
        {
            Width = static_cast<uint16_t>(cap.currentExtent.width);
            Height = static_cast<uint16_t>(cap.currentExtent.height);
        }

        info.imageExtent.width = static_cast<uint32_t>(Width);
        info.imageExtent.height = static_cast<uint32_t>(Height);

        err = vkCreateSwapchainKHR(Device, &info, AllocationCallbacks, &SwapchainKHR);
        check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [3]");
        err = vkGetSwapchainImagesKHR(Device, SwapchainKHR, &ImageCount, nullptr);
        check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [4]");
        VkImage backbuffers[16] = {};
        IM_ASSERT(ImageCount >= min_image_count);
        IM_ASSERT(ImageCount < IM_ARRAYSIZE(backbuffers));
        err = vkGetSwapchainImagesKHR(Device, SwapchainKHR, &ImageCount, backbuffers);
        check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [5]");

        IM_ASSERT(Frames == nullptr);
        Frames = (Vulkan_Frame*)IM_ALLOC(sizeof(Vulkan_Frame) * ImageCount);
        FrameSemaphores = (Vulkan_FrameSemaphores*)IM_ALLOC(sizeof(Vulkan_FrameSemaphores) * ImageCount);
        memset(Frames, 0, sizeof(Frames[0]) * ImageCount);
        memset(FrameSemaphores, 0, sizeof(FrameSemaphores[0]) * ImageCount);
        for (uint32_t i = 0; i < ImageCount; i++)
        {
            Frames[i].Backbuffer = backbuffers[i];
        }
            
    }

    if (old_swapchain)
    {
        vkDestroySwapchainKHR(Device, old_swapchain, AllocationCallbacks);
    }
        

    // Create the Render Pass
    {
        VkAttachmentDescription attachment = {};
        attachment.format = SurfaceFormatKHR.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        err = vkCreateRenderPass(Device, &info, AllocationCallbacks, &RenderPass);
        check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [6]");

        // We do not create a pipeline by default as this is also used by examples' main.cpp,
        // but secondary viewport in multi-viewport mode may want to create one with:
        //ImGui_ImplVulkan_CreatePipeline(Device, AllocationCallbacks, nullptr, RenderPass, VK_SAMPLE_COUNT_1_BIT, &Pipeline, bd->Subpass);
    }

    // Create The Image Views
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = SurfaceFormatKHR.format;
        info.components.r = VK_COMPONENT_SWIZZLE_R;
        info.components.g = VK_COMPONENT_SWIZZLE_G;
        info.components.b = VK_COMPONENT_SWIZZLE_B;
        info.components.a = VK_COMPONENT_SWIZZLE_A;
        VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        info.subresourceRange = image_range;
        for (uint32_t i = 0; i < ImageCount; i++)
        {
            Vulkan_Frame* fd = &Frames[i];
            info.image = fd->Backbuffer;
            err = vkCreateImageView(Device, &info, AllocationCallbacks, &fd->BackbufferView);
            check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [7]");
        }
    }

    // Create Framebuffer
    {
        VkImageView attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = RenderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = Width;
        info.height = Height;
        info.layers = 1;
        for (uint32_t i = 0; i < ImageCount; i++)
        {
            Vulkan_Frame* fd = &Frames[i];
            attachment[0] = fd->BackbufferView;
            err = vkCreateFramebuffer(Device, &info, AllocationCallbacks, &fd->Framebuffer);
            check_vk_result_dev_friendly(err, "VulkanDevice::createWindowSwapChain() [8]");
        }
    }
}

void VulkanDevice::createWindowCommandBuffers() {
    VkResult err = VK_SUCCESS;
    for (uint32_t i = 0; i < ImageCount; i++) 
    {
        Vulkan_Frame* fd = &Frames[i];
        Vulkan_FrameSemaphores* fsd = &FrameSemaphores[i];
        {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            info.queueFamilyIndex = QueueFamily;
            err = vkCreateCommandPool(Device, &info, AllocationCallbacks, &fd->CommandPool);
            check_vk_result_dev_friendly(err, "createWindowCommandBuffers() [1]");
        }
        {
            VkCommandBufferAllocateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.commandPool = fd->CommandPool;
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandBufferCount = 1;
            err = vkAllocateCommandBuffers(Device, &info, &fd->CommandBuffer);
            check_vk_result_dev_friendly(err, "createWindowCommandBuffers() [2]");
        }
        {
            VkFenceCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            err = vkCreateFence(Device, &info, AllocationCallbacks, &fd->Fence);
            check_vk_result_dev_friendly(err, "createWindowCommandBuffers() [3]");
        }
        {
            VkSemaphoreCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            err = vkCreateSemaphore(Device, &info, AllocationCallbacks, &fsd->ImageAcquiredSemaphore);
            check_vk_result_dev_friendly(err, "createWindowCommandBuffers() [4]");
            err = vkCreateSemaphore(Device, &info, AllocationCallbacks, &fsd->RenderCompleteSemaphore);
            check_vk_result_dev_friendly(err, "createWindowCommandBuffers() [5]");
        }
    }
}

void VulkanDevice::destroyFrame(Vulkan_Frame* p)
{
    if(p)
    {
        vkDestroyFence(Device, p->Fence, AllocationCallbacks);
        p->Fence = nullptr;

        vkFreeCommandBuffers(Device, p->CommandPool, 1, &p->CommandBuffer);
        p->CommandBuffer = nullptr;

        vkDestroyCommandPool(Device, p->CommandPool, AllocationCallbacks);
        p->CommandPool = nullptr;

        vkDestroyImageView(Device, p->BackbufferView, AllocationCallbacks);
        p->BackbufferView = nullptr;

        vkDestroyImage(Device, p->Backbuffer, AllocationCallbacks);
        p->Backbuffer = nullptr;

        vkDestroyFramebuffer(Device, p->Framebuffer, AllocationCallbacks);
        p->Framebuffer = nullptr;

        p = nullptr;
    }
}

void VulkanDevice::destroyFrameSemaphores(Vulkan_FrameSemaphores* p) 
{
    if(p)
    {
        vkDestroySemaphore(Device, p->ImageAcquiredSemaphore, AllocationCallbacks);
        p->ImageAcquiredSemaphore = nullptr;

        vkDestroySemaphore(Device, p->RenderCompleteSemaphore, AllocationCallbacks);
        p->RenderCompleteSemaphore = nullptr;

        p = nullptr;
    }
}

void VulkanDevice::createInstance(ImVector<const char*>& instance_extensions)
{
    VkResult err = VK_SUCCESS;
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    // Enumerate available extensions
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
    check_vk_result_dev_friendly(err, "createInstance() [1]");

    // Enable required extensions
    if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
        
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    // ConsoleLogAvailableInstanceExtensionProperties();
    // ConsoleLogAvailableInstanceLayerProperties();

    // Enabling validation layers
    const char* layers[1];
//     // layers[0] = "VK_KHR_portability_subset";
// #ifdef VULKAN_DEBUG_REPORT
//     layers[0] = "VK_LAYER_KHRONOS_validation";
//     create_info.enabledLayerCount = 1;
//     // create_info.ppEnabledLayerNames = layers;
//     instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
// #endif
    create_info.ppEnabledLayerNames = layers;

    // Create Vulkan Instance
    create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.Size);
    create_info.ppEnabledExtensionNames = instance_extensions.Data;
    err = vkCreateInstance(&create_info, AllocationCallbacks, &Instance);
    check_vk_result_dev_friendly(err, "createInstance() [2]");

    // Setup the debug report callback
#ifdef VULKAN_DEBUG_REPORT
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugReportCallbackEXT");
    IM_ASSERT(vkCreateDebugReportCallbackEXT != nullptr);
    VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
    debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_report_ci.pfnCallback = debug_report;
    debug_report_ci.pUserData = nullptr;
    err = vkCreateDebugReportCallbackEXT(Instance, &debug_report_ci, AllocationCallbacks, &DebugReportCallbackEXT);
    check_vk_result_dev_friendly(err, "createInstance() [3]");
#endif
}

void VulkanDevice::selectPhysicalDevice() 
{
    uint32_t gpu_count;
    VkResult err = vkEnumeratePhysicalDevices(Instance, &gpu_count, nullptr);
    check_vk_result_dev_friendly(err, "selectPhysicalDevice() [1]");
    IM_ASSERT(gpu_count > 0);

    ImVector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    err = vkEnumeratePhysicalDevices(Instance, &gpu_count, gpus.Data);
    check_vk_result_dev_friendly(err, "selectPhysicalDevice() [2]");

    // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
    // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
    // dedicated GPUs) is out of scope of this sample.
    for (VkPhysicalDevice& device : gpus) 
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
        {
            PhysicalDevice = device;
            PhysicalDeviceName = properties.deviceName;
            return;
        }
    }
    // Use first GPU (Integrated) is a Discrete one is not available.
    if (gpu_count > 0) 
    {
        PhysicalDevice = gpus[0];
    }

    return;
}

void VulkanDevice::selectGraphicsQueueFamily() 
{
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &count, nullptr);
    VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &count, queues);
    for (uint32_t i = 0; i < count; i++)
    {
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            QueueFamily = i;
            break;
        }
    }
        
    free(queues);
    IM_ASSERT(QueueFamily != (uint32_t)-1);
}

void VulkanDevice::createLogicalDevice() 
{
    VkResult err = VK_SUCCESS;

    // Create Logical Device (with 1 queue)
    ImVector<const char*> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");

    // Enumerate physical device extension
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
        device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    const float queue_priority[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = QueueFamily;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
    create_info.ppEnabledExtensionNames = device_extensions.Data;
    err = vkCreateDevice(PhysicalDevice, &create_info, AllocationCallbacks, &Device);
    check_vk_result_dev_friendly(err, "createLogicalDevice()");
    vkGetDeviceQueue(Device, QueueFamily, 0, &Queue);
}

void VulkanDevice::createDescriptorPool() 
{
    VkResult err = VK_SUCCESS;

    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    err = vkCreateDescriptorPool(Device, &pool_info, AllocationCallbacks, &DescriptorPool);
    check_vk_result_dev_friendly(err, "createDescriptorPool()");
}

void VulkanDevice::selectSurfaceFormat() 
{
    const VkFormat prefereredSurfaceImageFormat[] = 
    { 
        VkFormat::VK_FORMAT_B8G8R8A8_UNORM, 
        VkFormat::VK_FORMAT_R8G8B8A8_UNORM, 
        VkFormat::VK_FORMAT_B8G8R8_UNORM, 
        VkFormat::VK_FORMAT_R8G8B8_UNORM
    };

    const VkColorSpaceKHR prefereredSurfaceColorSpace = VkColorSpaceKHR::VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    VkSurfaceFormatKHR preferedFormat = {prefereredSurfaceImageFormat[0], prefereredSurfaceColorSpace};

    uint32_t avail_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, SurfaceKHR, &avail_count, nullptr);
    ImVector<VkSurfaceFormatKHR> availableFormats;
    availableFormats.resize((int)avail_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, SurfaceKHR, &avail_count, availableFormats.Data);

    if (avail_count == 1) 
    { 
        SurfaceFormatKHR = (availableFormats[0].format == VK_FORMAT_UNDEFINED) ? preferedFormat : availableFormats[0]; 
        return; 
    } else {                   // use first available format found
        for (int i = 0; i < IM_ARRAYSIZE(prefereredSurfaceImageFormat); i++)
        {
            for (uint32_t j = 0; j < avail_count; j++)
            {
                if (availableFormats[j].format == prefereredSurfaceImageFormat[i] && availableFormats[j].colorSpace == prefereredSurfaceColorSpace)
                {
                    SurfaceFormatKHR = availableFormats[j];
                    return;
                }   
            }  
        }
            
        SurfaceFormatKHR = availableFormats[0];    // If none of the preferered image formats could be found, use the first available
        return;
    }
}

void VulkanDevice::selectPresentMode() 
{
    /* 
        VK_PRESENT_MODE_IMMEDIATE_KHR (no vsync)
        VK_PRESENT_MODE_FIFO_KHR (vsync)             <- synchronizes to the system's refresh-rate, unless otherwise specified with semaphores and fences
        VK_PRESENT_MODE_MAILBOX_KHR (triple buffering)

        Arange these in order from most prefered, to least prefered. The first one that is found to be available on the system will be used.
    */
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
    int present_modes_ct = IM_ARRAYSIZE(present_modes);

    uint32_t avail_count = 0;
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, SurfaceKHR, &avail_count, nullptr);
    ImVector<VkPresentModeKHR> avail_modes;
    avail_modes.resize((int)avail_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, SurfaceKHR, &avail_count, avail_modes.Data);

    for (int i = 0; i < present_modes_ct; i++)
    {
        for (uint32_t j = 0; j < avail_count; j++)
        {
            if (present_modes[i] == avail_modes[j])
            {
                PresentModeKHR = present_modes[i];
                return;
            }   
        }    
    }
        
    PresentModeKHR = VK_PRESENT_MODE_FIFO_KHR; // Always available
    return;
}

#endif
#endif

