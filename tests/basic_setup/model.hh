#pragma once

#include <letc/letc.hh>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

struct Model
{
    struct Uniform
    {
        glm::mat4 model{1.0f};
        glm::mat4 modelIT{1.0f};
    };

    letc::ObjectBuffer<Uniform> uniform;
    std::shared_ptr<letc::DescriptorSet> ds;

    letc::VectorBuffer<unsigned short> index;
    letc::VectorBuffer<glm::vec3> position;
    letc::VectorBuffer<glm::vec3> normal;

    Model(std::weak_ptr<letc::Allocator> allocator, std::weak_ptr<letc::DescriptorSetLayout> dsl,
          std::weak_ptr<letc::DescriptorSetPool> pool)
        : uniform(allocator, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          index(allocator, 0, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          position(allocator, 0, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          normal(allocator, 0, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU)
    {
    }

    ~Model()
    {
    }

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    // Move Constructor
    Model(Model &&other) noexcept
        : uniform(std::move(other.uniform)),   // ObjectBuffer needs move semantics too! (See note below)
          ds(std::move(other.ds)),             // Move shared_ptr
          index(std::move(other.index)),       // Move VectorBuffer
          position(std::move(other.position)), // Move VectorBuffer
          normal(std::move(other.normal))      // Move VectorBuffer
    {
    }

    // Move Assignment Operator
    Model &operator=(Model &&other) noexcept
    {
        if (this != &other)
        {
            uniform = std::move(other.uniform);
            ds = std::move(other.ds);
            index = std::move(other.index);
            position = std::move(other.position);
            normal = std::move(other.normal);
        }
        return *this;
    }

    auto sync() -> void
    {
        uniform.sync();
        index.sync();
        position.sync();
        normal.sync();
    }

    auto draw(const vk::CommandBuffer c, const vk::PipelineLayout pl) -> void
    {
        c.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pl, 1, 1, &ds->get(), 0, nullptr);

        vk::Buffer vertexBuffers[] = {position.get(), normal.get()};
        vk::DeviceSize offsets[] = {0, 0};
        // Bind only 2 buffers, starting from binding 0
        c.bindVertexBuffers(0, 2, vertexBuffers, offsets); // Change 4 to 2
        c.bindIndexBuffer(index.get(), 0, vk::IndexType::eUint16);
        c.drawIndexed(static_cast<uint32_t>(index->size()), 1, 0, 0, 0);
    }
};

auto printAccInfo(const tinygltf::Accessor &acc) -> void
{
    std::println("Name: {}, Count: {}", acc.type, acc.count);
}

auto printViewInfo(const tinygltf::BufferView &view) -> void
{
    std::println("Length: {}", view.byteLength);
}

auto loadNode(std::vector<Model> &loadedModels, const tinygltf::Model &gltfModel, const tinygltf::Node &node,
              std::weak_ptr<letc::Allocator> allocator, std::weak_ptr<letc::DescriptorSetLayout> layout,
              std::weak_ptr<letc::DescriptorSetPool> pool, glm::mat4 currentTransform) -> void
{
    node.translation;
    node.rotation;
    node.scale;
    
    for (const auto &primitive : gltfModel.meshes[node.mesh].primitives)
    {
        const auto &mesh = gltfModel.meshes[node.mesh];

        // --- Vertex Attribute Data ---
        const unsigned char *basePosPtr = nullptr;
        const unsigned char *baseNorPtr = nullptr;
        const unsigned char *baseTexPtr = nullptr;

        size_t vertexCount = 0; // Number of vertices in this primitive

        // Position Attribute (Required)
        const auto posIt = primitive.attributes.find("POSITION");
        if (posIt == primitive.attributes.end())
        {
            // Handle error: Position attribute is required by glTF specification
            // For example, throw an exception or log an error and skip primitive
            std::cerr << "Error: Primitive missing POSITION attribute." << std::endl;
            continue; // Skip this primitive if positions are missing
        }
        const auto &posAcc = gltfModel.accessors.at(posIt->second);
        const auto &posView = gltfModel.bufferViews.at(posAcc.bufferView);
        const auto &posBuff = gltfModel.buffers.at(posView.buffer);
        vertexCount = posAcc.count; // Get the number of vertices from the position accessor

        // Calculate the starting memory address for position data
        basePosPtr = posBuff.data.data() + posView.byteOffset + posAcc.byteOffset;

        // Add runtime checks for safety (optional but recommended)
        assert(posAcc.type == TINYGLTF_TYPE_VEC3);
        assert(posAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        // Reinterpret cast to the desired pointer type (glm::vec3*)
        // Use const_cast only if you are *sure* you won't modify the data through this pointer,
        // or if your subsequent operations require a non-const pointer for read-only access patterns.
        // If you need to modify, copy the data first.
        glm::vec3 *posPtr = reinterpret_cast<glm::vec3 *>(const_cast<unsigned char *>(basePosPtr));

        // Normal Attribute (Optional)
        glm::vec3 *norPtr = nullptr; // Initialize to nullptr
        const auto norIt = primitive.attributes.find("NORMAL");
        if (norIt != primitive.attributes.end())
        {
            const auto &norAcc = gltfModel.accessors.at(norIt->second);
            const auto &norView = gltfModel.bufferViews.at(norAcc.bufferView);
            const auto &norBuff = gltfModel.buffers.at(norView.buffer);

            // Calculate the starting memory address for normal data
            baseNorPtr = norBuff.data.data() + norView.byteOffset + norAcc.byteOffset;

            // Add runtime checks (optional)
            assert(norAcc.type == TINYGLTF_TYPE_VEC3);
            assert(norAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(norAcc.count == vertexCount); // Normals should match vertex count

            // Reinterpret cast
            norPtr = reinterpret_cast<glm::vec3 *>(const_cast<unsigned char *>(baseNorPtr));
        }

        // Texture Coordinate Attribute (Optional - checking for TEXCOORD_0)
        glm::vec2 *texPtr = nullptr; // Initialize to nullptr
        const auto texIt = primitive.attributes.find("TEXCOORD_0");
        if (texIt != primitive.attributes.end())
        {
            const auto &texAcc = gltfModel.accessors.at(texIt->second);
            const auto &texView = gltfModel.bufferViews.at(texAcc.bufferView);
            const auto &texBuff = gltfModel.buffers.at(texView.buffer);

            // Calculate the starting memory address for texture coordinate data
            baseTexPtr = texBuff.data.data() + texView.byteOffset + texAcc.byteOffset;

            // Add runtime checks (optional)
            assert(texAcc.type == TINYGLTF_TYPE_VEC2);
            assert(texAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(texAcc.count == vertexCount); // Tex coords should match vertex count

            // Reinterpret cast
            texPtr = reinterpret_cast<glm::vec2 *>(const_cast<unsigned char *>(baseTexPtr));
        }

        // --- Index Data (Optional but common) ---
        const unsigned char *baseIndexPtr = nullptr;
        size_t indexCount = 0;
        int indexComponentType = 0; // e.g., TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT

        if (primitive.indices >= 0)
        { // Check if indices are used
            const auto &indexAcc = gltfModel.accessors.at(primitive.indices);
            const auto &indexView = gltfModel.bufferViews.at(indexAcc.bufferView);
            const auto &indexBuff = gltfModel.buffers.at(indexView.buffer);
            indexCount = indexAcc.count;
            indexComponentType = indexAcc.componentType;

            // Calculate the starting memory address for index data
            baseIndexPtr = indexBuff.data.data() + indexView.byteOffset + indexAcc.byteOffset;
        }

        // --- Now you have the pointers and counts ---
        // posPtr: Pointer to the first position (glm::vec3). Total count: vertexCount.
        // norPtr: Pointer to the first normal (glm::vec3) or nullptr. Total count: vertexCount.
        // texPtr: Pointer to the first tex coord (glm::vec2) or nullptr. Total count: vertexCount.
        // baseIndexPtr: Pointer to the first index or nullptr. Total count: indexCount. Type depends on
        // indexComponentType. vertexCount: Number of vertices. indexCount: Number of indices. indexComponentType: Data
        // type of indices (e.g., TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT).

        loadedModels.emplace_back(allocator, layout, pool);
        auto &model = loadedModels.back();

        model.index->resize(indexCount);
        std::memcpy(model.index->data(), baseIndexPtr, sizeof(unsigned short) * indexCount);
        model.index.sync();

        model.position->resize(vertexCount);
        std::memcpy(model.position->data(), posPtr, sizeof(glm::vec3) * vertexCount);
        model.position.sync();

        model.normal->resize(vertexCount);
        std::memcpy(model.normal->data(), norPtr, sizeof(glm::vec3) * vertexCount);
        model.normal.sync();

        model.ds = layout.lock()->build(pool);
        model.ds->attachBuffer("UModel", model.uniform.get(), sizeof(Model::Uniform));

        // Example: Accessing index data (assuming unsigned short indices)
        /*
        if (baseIndexPtr && indexComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
             const uint16_t* indexPtr = reinterpret_cast<const uint16_t*>(baseIndexPtr);
             for (size_t i = 0; i < indexCount; ++i) {
                 uint16_t index = indexPtr[i];
                 // Use index to access vertex data: posPtr[index], norPtr[index], etc.
             }
        }
        */

        // TODO: Use the pointers (posPtr, norPtr, texPtr, baseIndexPtr) and counts
        // (vertexCount, indexCount) along with indexComponentType to create your
        // engine's mesh representation, copy data to GPU buffers using your allocator, etc.
        // Remember to apply the 'currentTransform' (or the calculated nodeTransform if you compute it earlier)
        // to the vertex positions.

        // --- End of added code ---
    }

    for (int childIndex : node.children)
    {
        if (childIndex >= 0 && childIndex < gltfModel.nodes.size())
        {
            loadNode(loadedModels, gltfModel, gltfModel.nodes[childIndex], allocator, layout, pool, currentTransform);
        }
    }
}

inline auto loadModels(const std::filesystem::path &path, std::shared_ptr<letc::DescriptorSetLayout> layout,
                       std::shared_ptr<letc::DescriptorSetPool> pool, std::shared_ptr<letc::Allocator> allocator)
    -> std::shared_ptr<std::vector<Model>>
{
    auto loadedModels = std::make_shared<std::vector<Model>>();
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader([](tinygltf::Image *, const int, std::basic_string<char> *, std::basic_string<char> *, int,
                             int, const unsigned char *, int, void *) { return true; },
                          nullptr);
    std::string err;
    std::string warn;

    bool ret;

    if (path.extension() == ".glb")
    {
        ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
    }
    else if (path.extension() == ".gltf")
    {
        ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
    }
    else
    {
        throw std::runtime_error("unknown file ext");
    }

    if (!warn.empty())
    {
        std::println(std::cerr, "TinyGLTF Warning: {}", warn);
    }
    if (!err.empty())
    {
        std::println(std::cerr, "TinyGLTF Error: {}", err);
    }
    if (!ret)
    {
        std::println(std::cerr, "Failed to load glTF file: {}", path.string());
        return {}; // Return empty vector on failure
    }

    const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        loadNode(*loadedModels, gltfModel, gltfModel.nodes[nodeIndex], allocator, layout, pool,
                 glm::identity<glm::mat4>());
    }

    return loadedModels;
}
