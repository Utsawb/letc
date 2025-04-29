#pragma once

#include "letc/pch.hh"

namespace letc
{
    class Instance;

    class InstanceBuilder
    {
      public:
        InstanceBuilder();

        auto setAppName(const std::string &appName) -> InstanceBuilder &;
        auto setAppVer(const uint32_t &appVer) -> InstanceBuilder &;
        auto setEngineName(const std::string &engineName) -> InstanceBuilder &;
        auto setEngineVer(const uint32_t &engineVer) -> InstanceBuilder &;
        auto setApiVer(const uint32_t &apiVer) -> InstanceBuilder &;
        auto addExtension(const std::string &extension) -> InstanceBuilder &;

        auto build() -> std::shared_ptr<Instance>;

      private:
        friend Instance;

        std::string m_appName;
        uint32_t m_appVer;
        std::string m_engineName;
        uint32_t m_engineVer;
        uint32_t m_apiVer;
        std::unordered_set<std::string> m_extensions;
    };

    class Instance
    {
      public:
        ~Instance();
        auto get() -> vk::Instance;
        auto getApiVer() -> uint32_t;

      private:
        friend InstanceBuilder;
        InstanceBuilder m_instanceBuilder;

        vk::Instance m_handle;
    };
} // namespace letc
