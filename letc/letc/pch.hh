#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
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

#define ATHROW(condition, msg)                                                                                         \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        throw std::runtime_error(msg);                                                                                 \
    }
