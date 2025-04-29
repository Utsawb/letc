#include "instance.hh"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace letc
{
    InstanceBuilder::InstanceBuilder()
    {
        m_appName = "dev";
        m_apiVer = 0;
        m_engineName = "letc";
        m_engineVer = 0;
        m_apiVer = vk::ApiVersion13;
    }

    auto InstanceBuilder::setAppName(const std::string &appName) -> InstanceBuilder &
    {
        m_appName = appName;
        return *this;
    }

    auto InstanceBuilder::setAppVer(const uint32_t &appVer) -> InstanceBuilder &
    {
        m_apiVer = appVer;
        return *this;
    }

    auto InstanceBuilder::setEngineName(const std::string &engineName) -> InstanceBuilder &
    {
        m_engineName = engineName;
        return *this;
    }

    auto InstanceBuilder::setEngineVer(const uint32_t &engineVer) -> InstanceBuilder &
    {
        m_engineVer = engineVer;
        return *this;
    }

    auto InstanceBuilder::setApiVer(const uint32_t &apiVer) -> InstanceBuilder &
    {
        m_apiVer = apiVer;
        return *this;
    }

    auto InstanceBuilder::addExtension(const std::string &extension) -> InstanceBuilder &
    {
        m_extensions.insert(extension);
        return *this;
    }

    auto InstanceBuilder::build() -> std::shared_ptr<Instance>
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        std::span<const char *> windowExtensions = vkfw::getRequiredInstanceExtensions();

        vk::InstanceCreateInfo instanceInfo{};

        std::vector<const char *> charExt;
        charExt.reserve(m_extensions.size());
        std::ranges::for_each(m_extensions, [&charExt](const auto &s) { charExt.push_back(s.c_str()); });
        std::ranges::for_each(windowExtensions, [&charExt](const auto &c) { charExt.push_back(c); });
        instanceInfo.setPEnabledExtensionNames(charExt);

        auto instance = std::make_shared<Instance>();
        instance->m_instanceBuilder = *this;
        instance->m_handle = vk::createInstance(instanceInfo);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->m_handle);

        return instance;
    }

    Instance::~Instance()
    {
        m_handle.destroy();
    }

    auto Instance::get() -> vk::Instance
    {
        return m_handle;
    }

    auto Instance::getApiVer() -> uint32_t
    {
        return m_instanceBuilder.m_apiVer;
    }
} // namespace letc
