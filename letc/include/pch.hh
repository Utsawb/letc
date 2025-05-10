#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <iterator>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_shared.hpp>
#include <vulkan/vulkan_structs.hpp>

#define XR_USE_GRAPHICS_API_VULKAN

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "openxr.hpp"

#include <vkfw/vkfw.hpp>

#include "vk_mem_alloc.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define assertThrow(condition, msg)                                                                                    \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        throw std::runtime_error(msg);                                                                                 \
    }

inline std::vector<char> readFile(const std::filesystem::path &path)
{
    std::ifstream fileStream(path, std::ios::binary);
    assertThrow(fileStream, "Failed to open file: " + path.string());

    fileStream.seekg(0, std::ios::end);
    std::streampos fileSize = fileStream.tellg();
    assertThrow(fileSize > 0, "Failed to determine file size: " + path.string());

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    fileStream.seekg(0, std::ios::beg);
    fileStream.read(buffer.data(), fileSize);

    return buffer;
}
