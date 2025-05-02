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

    Model(Model &&other) noexcept
        : uniform(std::move(other.uniform)), ds(std::move(other.ds)), index(std::move(other.index)),
          position(std::move(other.position)), normal(std::move(other.normal))
    {
    }

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
        c.bindVertexBuffers(0, 2, vertexBuffers, offsets);
        c.bindIndexBuffer(index.get(), 0, vk::IndexType::eUint16);
        c.drawIndexed(static_cast<uint32_t>(index->size()), 1, 0, 0, 0);
    }
};

auto loadNode(std::vector<Model> &loadedModels, const tinygltf::Model &gltfModel, const tinygltf::Node &node,
              std::weak_ptr<letc::Allocator> allocator, std::weak_ptr<letc::DescriptorSetLayout> layout,
              std::weak_ptr<letc::DescriptorSetPool> pool, glm::mat4 currentTransform) -> void
{
    const auto &tRaw = node.translation;
    const auto &rRaw = node.rotation;
    const auto &sRaw = node.scale;

    gltfModel.images;

    if (node.translation.size() > 0)
    {
        currentTransform = glm::translate(currentTransform, glm::vec3{tRaw[0], tRaw[1], tRaw[2]});
    }
    if (node.rotation.size() > 0)
    {
    }
    if (node.scale.size() > 0)
    {
        currentTransform = glm::scale(currentTransform, glm::vec3{sRaw[0], sRaw[1], sRaw[2]});
    }

    for (const auto &primitive : gltfModel.meshes[node.mesh].primitives)
    {
        const auto &mesh = gltfModel.meshes[node.mesh];

        const unsigned char *basePosPtr = nullptr;
        const unsigned char *baseNorPtr = nullptr;
        const unsigned char *baseTexPtr = nullptr;

        size_t vertexCount = 0;

        const auto posIt = primitive.attributes.find("POSITION");
        if (posIt == primitive.attributes.end())
        {
            std::cerr << "Error: Primitive missing POSITION attribute." << std::endl;
            continue;
        }
        const auto &posAcc = gltfModel.accessors.at(posIt->second);
        const auto &posView = gltfModel.bufferViews.at(posAcc.bufferView);
        const auto &posBuff = gltfModel.buffers.at(posView.buffer);
        vertexCount = posAcc.count;

        basePosPtr = posBuff.data.data() + posView.byteOffset + posAcc.byteOffset;

        assert(posAcc.type == TINYGLTF_TYPE_VEC3);
        assert(posAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        glm::vec3 *posPtr = reinterpret_cast<glm::vec3 *>(const_cast<unsigned char *>(basePosPtr));

        glm::vec3 *norPtr = nullptr;
        const auto norIt = primitive.attributes.find("NORMAL");
        if (norIt != primitive.attributes.end())
        {
            const auto &norAcc = gltfModel.accessors.at(norIt->second);
            const auto &norView = gltfModel.bufferViews.at(norAcc.bufferView);
            const auto &norBuff = gltfModel.buffers.at(norView.buffer);

            baseNorPtr = norBuff.data.data() + norView.byteOffset + norAcc.byteOffset;

            assert(norAcc.type == TINYGLTF_TYPE_VEC3);
            assert(norAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(norAcc.count == vertexCount);

            norPtr = reinterpret_cast<glm::vec3 *>(const_cast<unsigned char *>(baseNorPtr));
        }

        glm::vec2 *texPtr = nullptr;
        const auto texIt = primitive.attributes.find("TEXCOORD_0");
        if (texIt != primitive.attributes.end())
        {
            const auto &texAcc = gltfModel.accessors.at(texIt->second);
            const auto &texView = gltfModel.bufferViews.at(texAcc.bufferView);
            const auto &texBuff = gltfModel.buffers.at(texView.buffer);

            baseTexPtr = texBuff.data.data() + texView.byteOffset + texAcc.byteOffset;

            assert(texAcc.type == TINYGLTF_TYPE_VEC2);
            assert(texAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(texAcc.count == vertexCount);

            texPtr = reinterpret_cast<glm::vec2 *>(const_cast<unsigned char *>(baseTexPtr));
        }

        const unsigned char *baseIndexPtr = nullptr;
        size_t indexCount = 0;
        int indexComponentType = 0;

        if (primitive.indices >= 0)
        {
            const auto &indexAcc = gltfModel.accessors.at(primitive.indices);
            const auto &indexView = gltfModel.bufferViews.at(indexAcc.bufferView);
            const auto &indexBuff = gltfModel.buffers.at(indexView.buffer);
            indexCount = indexAcc.count;
            indexComponentType = indexAcc.componentType;

            baseIndexPtr = indexBuff.data.data() + indexView.byteOffset + indexAcc.byteOffset;
        }

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
        model.uniform->model = currentTransform;
        model.uniform->modelIT = glm::inverseTranspose(currentTransform);
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
        return {};
    }

    const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        loadNode(*loadedModels, gltfModel, gltfModel.nodes[nodeIndex], allocator, layout, pool,
                 glm::identity<glm::mat4>());
    }

    return loadedModels;
}
