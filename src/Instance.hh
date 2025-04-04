#pragma once

#include <vulkan/vulkan.hpp>
#ifndef LETC_INSTANCE_HH
#define LETC_INSTANCE_HH

#include "pch.hh"

namespace letc
{
    static VKAPI_ATTR uint32_t VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
                                                        const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData)
    {
        std::string severity;
        switch (messageSeverity)
        {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
            severity = "VERBOSE";
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            severity = "INFO";
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            severity = "WARNING";
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            severity = "ERROR";
            break;
        default:
            severity = "UNKNOWN";
            break;
        }

        std::string types;
        if (messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
            types += "GENERAL";
        if (messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
            types += "VALIDATION";
        if (messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            types += "PERFORMANCE";
        if (messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
            types += "DEVICE_ADDRESS_BINDING";

        std::cerr << "DebugMessenger: " << severity << ": " << types << ": " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    struct InstanceBuilder
    {
        bool debug;
        vk::ApplicationInfo applicationInfo;
        std::vector<const char *> instanceExtensions;
        std::vector<const char *> validationLayers;
        std::vector<const char *> debugExtensions;
        std::vector<vk::ValidationFeatureEnableEXT> validationFeatures;
        vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo;

        InstanceBuilder()
        {
            debug = false;

            /*
                Application Information
            */
            applicationInfo.setPApplicationName("Dev");
            applicationInfo.setApplicationVersion(vk::makeApiVersion(0, 0, 1, 0));
            applicationInfo.setPEngineName("Little Engine That Could");
            applicationInfo.setEngineVersion(vk::makeApiVersion(0, 0, 1, 0));
            applicationInfo.setApiVersion(vk::ApiVersion13);

            /*
                Instance Level Extensions
            */
            instanceExtensions.push_back(vk::KHRSurfaceExtensionName);
            instanceExtensions.push_back(vk::KHRGetSurfaceCapabilities2ExtensionName);

            debugExtensions.push_back(vk::EXTDebugUtilsExtensionName);

            /*
                Validation Layers and Features
            */
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
            validationFeatures.push_back(vk::ValidationFeatureEnableEXT::eDebugPrintf);

            /*
                DebugMessenger
            */
            debugMessengerInfo.setMessageSeverity(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            debugMessengerInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                              vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding);
            debugMessengerInfo.setPfnUserCallback(&debugCallback);
        }

        InstanceBuilder &setDebug(const bool &debug)
        {
            this->debug = debug;
            return *this;
        }

        InstanceBuilder &setApplicationInfo(const vk::ApplicationInfo &applicationInfo)
        {
            this->applicationInfo = applicationInfo;
            return *this;
        }

        InstanceBuilder &addInstanceExtension(const char *extension)
        {
            instanceExtensions.push_back(extension);
            return *this;
        }

        InstanceBuilder &addValidationLayer(const char *layer)
        {
            validationLayers.push_back(layer);
            return *this;
        }

        InstanceBuilder &addDebugExtension(const char *extension)
        {
            debugExtensions.push_back(extension);
            return *this;
        }

        InstanceBuilder &addValidationFeature(const vk::ValidationFeatureEnableEXT &feature)
        {
            validationFeatures.push_back(feature);
            return *this;
        }

        InstanceBuilder &setDebugMessengerInfo(const vk::DebugUtilsMessengerCreateInfoEXT &debugMessengerInfo)
        {
            this->debugMessengerInfo = debugMessengerInfo;
            return *this;
        }
    };

    struct Instance
    {
        vk::Instance instance;
        const InstanceBuilder instanceBuilder;

        operator const vk::Instance &()
        {
            return instance;
        }

        Instance(InstanceBuilder ib) : instanceBuilder(ib)
        {
            std::span<const char *> requiredExtensions = vkfw::getRequiredInstanceExtensions();
            ib.instanceExtensions.insert(ib.instanceExtensions.end(), requiredExtensions.begin(),
                                         requiredExtensions.end());

            /*
                InstanceCreate
            */
            vk::InstanceCreateInfo instanceCreateInfo{};
            vk::ValidationFeaturesEXT validationFeaturesInfo{};
            if (ib.debug)
            {
                ib.instanceExtensions.insert(ib.instanceExtensions.end(), ib.debugExtensions.begin(),
                                             ib.debugExtensions.end());
                instanceCreateInfo.setPEnabledLayerNames(ib.validationLayers);
                validationFeaturesInfo.setEnabledValidationFeatures(ib.validationFeatures);
                instanceCreateInfo.setPNext(&ib.debugMessengerInfo);
                ib.debugMessengerInfo.setPNext(&validationFeaturesInfo);
            }
            instanceCreateInfo.setPApplicationInfo(&ib.applicationInfo);
            instanceCreateInfo.setPEnabledExtensionNames(ib.instanceExtensions);

            instance = vk::createInstance(instanceCreateInfo);
        }

        ~Instance()
        {
            instance.destroy();
        }
    };

}; // namespace letc

#endif // LETC_INSTANCE_HH
