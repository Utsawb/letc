#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "spirv_reflect.hh"

#include "Device.hh"

namespace letc
{
    struct ShaderManager
    {
        const Device &device;
        std::unordered_map<std::string, vk::UniqueShaderModule> shaders;

        ShaderManager(const Device &device) : device(device)
        {
        }

        ShaderManager &loadShader(const std::string &id, const std::vector<uint32_t> &code, const std::string &entry)
        {
            spv_reflect::ShaderModule sm{code};


            return *this;
        }
    };
}; // namespace letc
