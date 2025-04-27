#include "letc/core/shaders.hh"
#include "letc/core/device.hh" // Include Device header for vk::Device access

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_hlsl.hpp> // Include for potentially getting names if needed

#include <map> // Use std::map to keep sets sorted by index

namespace letc
{

    ShaderManager::ShaderManager(std::weak_ptr<Device> device)
    {
        m_device = device;
    }

    auto ShaderManager::add(const std::filesystem::path &path, const std::string &entry,
                            const vk::ShaderStageFlagBits &stage) -> ShaderManager &
    {
        // Ensure the device pointer is valid
        auto device_ptr = m_device.lock();
        ATHROW(device_ptr, "Device pointer is expired in ShaderManager");
        vk::Device logicalDevice = device_ptr->getLogical(); //

        // Read shader code if not already cached
        if (m_codes.find(path) == m_codes.end())
        {
            m_codes.try_emplace(path, readFile(path)); //
        }
        const auto &shader_code = m_codes.at(path);

        // Check if this specific shader entry point is already processed
        auto shader_key = std::make_pair(path, entry);
        if (m_shaders.count(shader_key))
        {
            // Potentially update stages if the same shader is added for a different stage?
            // For now, just return if already present.
            return *this;
        }

        // --- Compile and Reflect ---
        spirv_cross::Compiler compiler(shader_code);
        auto resources = compiler.get_shader_resources();

        // Use std::map to store layouts, ensuring sets are ordered by index
        // Key: Set Index, Value: ResourceSetLayout internal data (bindings)
        std::map<uint32_t, std::unordered_map<uint32_t, ResourceLayout>> set_layouts_map;

        auto process_resource = [&](const spirv_cross::Resource &r, vk::DescriptorType type) {
            uint32_t set = compiler.get_decoration(r.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(r.id, spv::DecorationBinding);
            const spirv_cross::SPIRType &basetype = compiler.get_type(r.base_type_id);

            // Determine array count (if it's an array)
            uint32_t count = 1;
            if (!basetype.array.empty())
            {
                // If it's an array, check if the size is specified (literal)
                if (basetype.array_size_literal[0])
                {
                    count = basetype.array[0]; // Size is directly the value
                }
                else
                {
                    // If size comes from a specialization constant or is unbounded (0),
                    // handle appropriately. For simplicity, default to 1 or handle error.
                    // Vulkan requires bounded arrays unless specific extensions are used.
                    // Assuming count=1 for unbounded for now, adjust as needed.
                    count = 1; // Or potentially query specialization constant if needed
                    // ATHROW(basetype.array[0] > 0, "Unsupported unbounded descriptor array or specialization constant
                    // size.");
                }
                // If count is 0 (potentially unbounded in SPIR-V), treat as 1 in Vulkan unless dynamically sized.
                if (count == 0)
                    count = 1;
            }

            set_layouts_map[set][binding] = ResourceLayout{
                .name = compiler.get_name(r.id).empty() ? r.name : compiler.get_name(r.id), // Prefer decorated name
                .binding = binding,
                .type = type,
                .stages = stage, // Add the current stage flag
                .count = count};
        };

        // --- Process Resources ---
        for (const auto &r : resources.uniform_buffers)
        {
            process_resource(r, vk::DescriptorType::eUniformBuffer);
        }

        for (const auto &r : resources.storage_buffers)
        {
            process_resource(r, vk::DescriptorType::eStorageBuffer);
        }

        for (const auto &r : resources.sampled_images)
        {
            // Combined image samplers, separate images, separate samplers need different types
            const spirv_cross::SPIRType &type = compiler.get_type(r.type_id);
            vk::DescriptorType descriptorType;
            if (type.image.dim == spv::DimBuffer)
            {
                descriptorType = vk::DescriptorType::eUniformTexelBuffer; // Or StorageTexelBuffer if written to?
            }
            else
            {
                // Heuristic: Assume combined if name suggests it or default
                descriptorType = vk::DescriptorType::eCombinedImageSampler;
                // Could add logic to check for separate image/sampler types if needed
            }
            process_resource(r, descriptorType);
        }
        for (const auto &r : resources.storage_images)
        {
            vk::DescriptorType descriptorType;
            const spirv_cross::SPIRType &type = compiler.get_type(r.type_id);
            if (type.image.dim == spv::DimBuffer)
            {
                descriptorType = vk::DescriptorType::eStorageTexelBuffer;
            }
            else
            {
                descriptorType = vk::DescriptorType::eStorageImage;
            }
            process_resource(r, descriptorType);
        }
        // Add other resource types (separate images, samplers, input attachments, etc.) as needed

        // --- Process Push Constants ---
        std::vector<vk::PushConstantRange> push_constant_ranges;
        for (const auto &r : resources.push_constant_buffers)
        {
            const auto &type = compiler.get_type(r.base_type_id);
            size_t size = compiler.get_declared_struct_size(type); // Get size from type

            // Get offset if available (might require HLSL specific reflection or naming convention)
            // spirv-cross might not directly provide offset easily for standard Vulkan GLSL SPIR-V
            // Often offset is 0 for the first/only push constant block. Assume 0 for now.
            uint32_t offset = 0;
            // Attempt to get offset if decorated (less common in GLSL)
            if (compiler.has_decoration(r.id, spv::DecorationOffset))
            {
                offset = compiler.get_decoration(r.id, spv::DecorationOffset);
            }

            // Check active stages for this push constant block
            // This is slightly complex as SPIRV-Cross gives ranges. We need the union.
            auto active_ranges = compiler.get_active_buffer_ranges(r.id);
            // For a push constant, it's generally active across the entire stage it's declared in.
            vk::ShaderStageFlags push_stages = stage; // Start with the current stage

            push_constant_ranges.push_back(
                vk::PushConstantRange{}.setStageFlags(push_stages).setOffset(offset).setSize(size));
        }

        // --- Create Descriptor Set Layouts ---
        std::vector<ResourceSetLayout> final_layouts;
        final_layouts.reserve(set_layouts_map.size());

        for (auto const &[set_index, layout_map] : set_layouts_map)
        {
            std::vector<vk::DescriptorSetLayoutBinding> bindings;
            bindings.reserve(layout_map.size());
            ResourceSetLayout current_set_layout_data;            // To store ResourceLayout structs
            current_set_layout_data.resourceLayouts = layout_map; // Copy map content

            for (auto const &[binding_index, resource_layout] : layout_map)
            {
                bindings.push_back(vk::DescriptorSetLayoutBinding{}
                                       .setBinding(resource_layout.binding)
                                       .setDescriptorType(resource_layout.type)
                                       .setDescriptorCount(resource_layout.count)
                                       .setStageFlags(resource_layout.stages)
                                       .setImmutableSamplers(nullptr));
            }

            auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{}
                                  .setFlags(vk::DescriptorSetLayoutCreateFlags{})
                                  .setBindings(bindings);

            current_set_layout_data.layout = logicalDevice.createDescriptorSetLayout(layoutInfo);
            final_layouts.push_back(std::move(current_set_layout_data));
        }

        Shader s;
        s.m_entry = entry;
        s.m_stage = stage;
        s.m_layouts = std::move(final_layouts);
        s.m_push = std::move(push_constant_ranges);

        auto moduleInfo = vk::ShaderModuleCreateInfo{}.setCode(shader_code);
        s.m_module = device_ptr->getLogical().createShaderModule(moduleInfo);
        m_shaders.emplace(shader_key, s);

        return *this;
    }

    // --- getShader Implementation (Example) ---
    auto ShaderManager::getShader(const std::filesystem::path &path, const std::string &entry) -> Shader
    {
        auto it = m_shaders.find({path, entry});
        ATHROW(it != m_shaders.end(), std::format("Shader not found: {} entry {}", path.string(), entry));
        return it->second; // Return a copy or const reference as appropriate
    }

} // namespace letc
