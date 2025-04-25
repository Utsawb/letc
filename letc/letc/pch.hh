#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>

#include <vkfw/vkfw.hpp>

#include <vk_mem_alloc.h>

#include <tiny_gltf.h>

#include <shader-slang/slang.h>
#include <shader-slang/slang-com-ptr.h>
#include <shader-slang/slang-com-helper.h>

#define ATHROW(condition, msg)                                                                                         \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        throw std::runtime_error(msg);                                                                                 \
    }

namespace letc
{
    inline auto readFile(const std::filesystem::path &path) -> std::vector<char>
    {
        std::ifstream fileStream(path, std::ios::binary);
        ATHROW(fileStream, std::format("failed to open file {}", path.c_str()));

        fileStream.seekg(0, std::ios::end);
        std::streampos fileSize = fileStream.tellg();
        ATHROW(fileSize > 0, "Failed to determine file size: " + path.string());

        std::vector<char> buffer(static_cast<size_t>(fileSize));
        fileStream.seekg(0, std::ios::beg);
        fileStream.read(buffer.data(), fileSize);

        return buffer;
    }
} // namespace letc
