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

    letc::VectorBuffer<unsigned int> index;
    letc::VectorBuffer<glm::vec4> position;
    letc::VectorBuffer<glm::vec4> normal;
    letc::VectorBuffer<glm::vec2> texcoord;

    Model(std::weak_ptr<letc::Allocator> allocator, std::weak_ptr<letc::DescriptorSetLayout> dsl,
          std::weak_ptr<letc::DescriptorSetPool> pool)
        : uniform(allocator, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          index(allocator, 0, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          position(allocator, 0, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          normal(allocator, 0, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU),
          texcoord(allocator, 0, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU)
    {
    }

    auto sync() -> void
    {
        uniform.sync();
        index.sync();
        position.sync();
        normal.sync();
        texcoord.sync();
    }

    auto draw(const vk::CommandBuffer c, const vk::PipelineLayout pl) -> void
    {
        c.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pl, 1, 1, &ds->get(), 0, nullptr);

        vk::Buffer vertexBuffers[] = {position.get(), normal.get(), texcoord.get()};
        vk::DeviceSize offsets[] = {0, 0, 0, 0};
        c.bindVertexBuffers(0, 4, vertexBuffers, offsets);
        c.bindIndexBuffer(index.get(), 0, vk::IndexType::eUint32);
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
    for (const auto &primitive : gltfModel.meshes[node.mesh].primitives)
    {
        const auto &posAcc = gltfModel.accessors.at(primitive.attributes.at("POSITION"));
        const auto &norAcc = gltfModel.accessors.at(primitive.attributes.at("NORMAL"));
        const auto &texAcc = gltfModel.accessors.at(primitive.attributes.at("TEXCOORD_0"));

        const auto &posView = gltfModel.bufferViews[posAcc.bufferView];
        const auto &norView = gltfModel.bufferViews[norAcc.bufferView];
        const auto &texView = gltfModel.bufferViews[texAcc.bufferView];

        const auto &posBuff = gltfModel.buffers[posView.buffer];
        const auto &norBuff = gltfModel.buffers[norView.buffer];
        const auto &texBuff = gltfModel.buffers[texView.buffer];

        const auto &posRaw = posBuff.data.data();
        const auto &norRaw = norBuff.data.data();
        const auto &texRaw = texBuff.data.data();


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
    -> std::vector<Model>
{
    std::vector<Model> loadedModels;
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader([](tinygltf::Image *, const int, std::basic_string<char> *, std::basic_string<char> *, int,
                             int, const unsigned char *, int, void *) { return true; },
                          nullptr);
    std::string err;
    std::string warn;
    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);

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
        return loadedModels; // Return empty vector on failure
    }

    const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        loadNode(loadedModels, gltfModel, gltfModel.nodes[nodeIndex], allocator, layout, pool,
                 glm::identity<glm::mat4>());
    }

    return loadedModels;
}
