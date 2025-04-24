#pragma once

#include "letc/pch.hh"

#include "letc/core//device.hh"

namespace letc
{
    class Model
    {
    };

    class ModelCache
    {

      private:
        std::weak_ptr<Device> m_device;
        std::unordered_map<std::filesystem::path, Model> m_cache;
    };

    class Shader
    {
    };

    class ShaderCache
    {
      public:
        ShaderCache(std::weak_ptr<Device> device);
        auto getShader(const std::filesystem::path &path) -> Shader;

      private:
        std::weak_ptr<Device> m_device;
        std::unordered_map<std::filesystem::path, Shader> m_cache;
    };

} // namespace letc
