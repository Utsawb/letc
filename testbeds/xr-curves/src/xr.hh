#pragma once

#include "pch.hh"

#include "Camera.hh"

// moving all the xr code into its own class cause its just boilerplate

struct XrContext
{
    xr::DispatchLoaderDynamic dispatchLoader;
    xr::UniqueDynamicInstance instance;
    xr::SystemId systemId;

    xr::UniqueDynamicSession session;
    xr::UniqueDynamicSwapchain swapchain[2];

    xr::Time predictedDisplayTime = xr::Time{};
    xr::UniqueDynamicSpace leftHandSpace;
    xr::UniqueDynamicSpace rightHandSpace;
    xr::Path leftHandPath;
    xr::Path rightHandPath;
    xr::UniqueDynamicActionSet actionSet;
    xr::UniqueDynamicAction triggerAction;
    xr::UniqueDynamicAction joystickAction;
    xr::UniqueDynamicAction faceButtonAction;
    xr::UniqueDynamicAction handPoseAction;

    std::vector<xr::ViewConfigurationView> viewConfigurationViews;
    xr::UniqueDynamicSpace space;
    long long swapchainFormat;
    std::vector<xr::SwapchainImageVulkanKHR, std::allocator<xr::SwapchainImageVulkanKHR>> swapchainImageVk[2];
    std::vector<vk::UniqueImageView> swapchainImageViews[2];
    xr::EventDataBuffer eventDataBuffer{};
    xr::SessionState sessionState = xr::SessionState::Unknown;

    std::vector<std::unique_ptr<letc::Camera>> cameras;

    XrContext()
    {
        dispatchLoader = xr::DispatchLoaderDynamic{};
        auto xrDebugCallback =
            [](XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes,
               const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) -> XrBool32 {
            std::cerr << "XrDebug: " << pCallbackData->messageId << ": " << pCallbackData->message << std::endl;
            return XR_FALSE;
        };

        xr::DebugUtilsMessengerCreateInfoEXT xrDebugInfo{};
        xrDebugInfo.messageSeverities = xr::DebugUtilsMessageSeverityFlagBitsEXT::AllBits;
        xrDebugInfo.messageTypes = xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits;
        xrDebugInfo.userCallback = xrDebugCallback;
        xrDebugInfo.userData = nullptr;

        xr::ApplicationInfo xrAppInfo{};
        std::memcpy(xrAppInfo.applicationName, "Dev", 4);
        xrAppInfo.applicationVersion = 1;
        std::memcpy(xrAppInfo.engineName, "letc", 5);
        xrAppInfo.engineVersion = 1;
        xrAppInfo.apiVersion = xr::Version{XR_CURRENT_API_VERSION};

        std::vector<const char *> xrInstanceExtensions = {"XR_KHR_vulkan_enable2"};
        xr::InstanceCreateInfo xrInstanceInfo{};
        xrInstanceInfo.createFlags = xr::InstanceCreateFlagBits::None;
        xrInstanceInfo.applicationInfo = xrAppInfo;
        xrInstanceInfo.enabledExtensionCount = static_cast<uint32_t>(xrInstanceExtensions.size());
        xrInstanceInfo.enabledExtensionNames = xrInstanceExtensions.data();
        xrInstanceInfo.enabledApiLayerCount = 0;
        xrInstanceInfo.enabledApiLayerNames = nullptr;
        xrInstanceInfo.next = &xrDebugInfo;

        instance = xr::createInstanceUnique(xrInstanceInfo, dispatchLoader);
        assertThrow(instance, "failed to create xr instance");
        dispatchLoader = xr::DispatchLoaderDynamic{*instance};

        xr::SystemGetInfo xrSystemInfo{};
        xrSystemInfo.formFactor = xr::FormFactor::HeadMountedDisplay;
        xrSystemInfo.next = nullptr;
        systemId = instance->getSystem(xrSystemInfo);
    }
};
