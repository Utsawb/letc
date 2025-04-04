#pragma once

#ifndef PCH_HH
#define PCH_HH

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_shared.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "openxr/openxr.h"

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

#endif // PCH_HH
