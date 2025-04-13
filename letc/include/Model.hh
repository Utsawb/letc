#pragma once

#ifndef LETC_MODEL_HH
#define LETC_MODEL_HH

#include <memory>

#include "pch.hh"

#include "Allocator.hh"
#include "Buffer.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Material.hh"
#include "Pipeline.hh"
#include "Renderable.hh"
#include "Swapchain.hh"

namespace letc
{
    struct ModelRenderer
    {
        const Device &device;
        const Allocator &allocator;

        std::unique_ptr<DescriptorLayout> layout;
        std::unique_ptr<Material> material;
        std::unique_ptr<GraphicsPipeline> pipeline;

        ModelRenderer(const Allocator &allocator, const Device &device, const Swapchain &swapchain,
                      const std::vector<char> &vertexCode, const std::vector<char> &fragmentCode)
            : allocator(allocator), device(device)
        {
            layout = std::make_unique<DescriptorLayout>(device);
            layout->addBinding(0, 0, vk::DescriptorType::eUniformBuffer,
                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 1);
            layout->addBinding(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, 1);
            layout->addBinding(0, 2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 1);
            layout->addBinding(1, 0, vk::DescriptorType::eStorageBuffer,
                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 1);
            layout->generateLayouts();

            material = std::make_unique<Material>(device, allocator, *layout);

            letc::GraphicsPipelineBuilder gpb;
            gpb.addShaderStage(vertexCode, vk::ShaderStageFlagBits::eVertex);
            gpb.addShaderStage(fragmentCode, vk::ShaderStageFlagBits::eFragment);
            gpb.addVertexInputBinding(0, sizeof(glm::vec4), vk::VertexInputRate::eVertex); // Position
            gpb.addVertexInputAttribute(0, 0, vk::Format::eR32G32B32A32Sfloat, 0);
            gpb.addVertexInputBinding(1, sizeof(glm::vec4), vk::VertexInputRate::eVertex); // Normal
            gpb.addVertexInputAttribute(1, 1, vk::Format::eR32G32B32A32Sfloat, 0);
            gpb.addVertexInputBinding(2, sizeof(glm::vec4), vk::VertexInputRate::eVertex); // Tangent
            gpb.addVertexInputAttribute(2, 2, vk::Format::eR32G32B32A32Sfloat, 0);
            gpb.addVertexInputBinding(3, sizeof(glm::vec2), vk::VertexInputRate::eVertex); // UV
            gpb.addVertexInputAttribute(3, 3, vk::Format::eR32G32Sfloat, 0);
            gpb.addPushConstantRange(vk::PushConstantRange{}
                                         .setSize(sizeof(uint32_t))
                                         .setOffset(0)
                                         .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics));
            gpb.setLayout(layout.get());
            gpb.renderingInfo.setColorAttachmentCount(1);
            gpb.renderingInfo.setPColorAttachmentFormats(&swapchain.format.format);
            gpb.setRasterization(gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone));

            pipeline = std::make_unique<GraphicsPipeline>(device, gpb);
        }
    };

    struct Model
    {
        std::vector<unsigned> index;
        std::vector<glm::vec4> position;
        std::vector<glm::vec4> normal;
        std::vector<glm::vec4> tangent;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec4> color;
        std::vector<std::array<unsigned, 4>> joints;
        std::vector<glm::vec4> weights;

        std::unique_ptr<Buffer> indexBuffer;
        std::unique_ptr<Buffer> positionBuffer;
        std::unique_ptr<Buffer> normalBuffer;
        std::unique_ptr<Buffer> tangentBuffer;
        std::unique_ptr<Buffer> uvBuffer;
        std::unique_ptr<Buffer> colorBuffer;
        std::unique_ptr<Buffer> jointsBuffer;
        std::unique_ptr<Buffer> weightsBuffer;

        struct UniformBuffer
        {
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 modelInvTranspose = glm::mat4(1.0f);
            glm::vec4 attributeFlags1 = glm::vec4(0.0f);
            glm::vec4 attributeFlags2 = glm::vec4(0.0f);
        };
        UniformBuffer uniform;

        std::vector<vk::Buffer> validBuffers;

        Model(const Allocator &allocator, const std::filesystem::path &modelPath)
        {
            Assimp::Importer importer;

            // const aiScene *scene = importer.ReadFile(
            //     modelPath.string(), aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
            //                             aiProcess_Triangulate | aiProcess_GenNormals |
            //                             aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality |
            //                             aiProcess_GenUVCoords | aiProcess_FlipUVs);

            const aiScene *scene = importer.ReadFile(modelPath.string(), aiProcess_Triangulate | aiProcess_GenNormals |
                                                                             aiProcess_ImproveCacheLocality |
                                                                             aiProcess_GenUVCoords | aiProcess_FlipUVs);

            assertThrow(scene, "failed to load model: " + modelPath.string());
            assertThrow(scene->mNumMeshes == 1, "model should only have one mesh");
            aiMesh *mesh = scene->mMeshes[0];

            if (mesh->HasFaces())
            {
                for (size_t i = 0; i < mesh->mNumFaces; i++)
                {
                    index.push_back(mesh->mFaces[i].mIndices[0]);
                    index.push_back(mesh->mFaces[i].mIndices[1]);
                    index.push_back(mesh->mFaces[i].mIndices[2]);
                }
                indexBuffer =
                    std::make_unique<Buffer>(allocator, index.size() * sizeof(unsigned),
                                             vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            if (mesh->HasPositions())
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    position.push_back({mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f});
                }
                positionBuffer =
                    std::make_unique<Buffer>(allocator, position.size() * sizeof(glm::vec4),
                                             vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
                validBuffers.push_back(positionBuffer->buffer);
            }

            if (mesh->HasNormals())
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    normal.push_back({mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 1.0f});
                }
                normalBuffer =
                    std::make_unique<Buffer>(allocator, normal.size() * sizeof(glm::vec4),
                                             vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
                validBuffers.push_back(normalBuffer->buffer);
            }

            if (mesh->HasTangentsAndBitangents())
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    tangent.push_back({mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f});
                }
                tangentBuffer =
                    std::make_unique<Buffer>(allocator, tangent.size() * sizeof(glm::vec4),
                                             vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
                validBuffers.push_back(tangentBuffer->buffer);
            }

            if (mesh->HasTextureCoords(0))
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    uv.push_back({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y});
                }
                uvBuffer =
                    std::make_unique<Buffer>(allocator, uv.size() * sizeof(glm::vec2),
                                             vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
                validBuffers.push_back(uvBuffer->buffer);
            }

            if (mesh->HasVertexColors(0))
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    color.push_back(
                        {mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a});
                }
                colorBuffer =
                    std::make_unique<Buffer>(allocator, color.size() * sizeof(glm::vec4),
                                             vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
                validBuffers.push_back(colorBuffer->buffer);
            }
        }

        void cpyAttributes()
        {
            uniform.modelInvTranspose = glm::transpose(glm::inverse(uniform.model));

            if (indexBuffer)
            {
                indexBuffer->cpy(index.data(), index.size() * sizeof(unsigned));
            }
            if (positionBuffer)
            {
                positionBuffer->cpy(position.data(), position.size() * sizeof(glm::vec4));
            }
            if (normalBuffer)
            {
                normalBuffer->cpy(normal.data(), normal.size() * sizeof(glm::vec4));
            }
            if (tangentBuffer)
            {
                tangentBuffer->cpy(tangent.data(), tangent.size() * sizeof(glm::vec4));
            }
            if (uvBuffer)
            {
                uvBuffer->cpy(uv.data(), uv.size() * sizeof(glm::vec2));
            }
            if (colorBuffer)
            {
                colorBuffer->cpy(color.data(), color.size() * sizeof(glm::vec4));
            }
        }

        void draw(const vk::CommandBuffer &commandBuffer)
        {
            commandBuffer.bindIndexBuffer(indexBuffer->buffer, 0, vk::IndexType::eUint32);
            commandBuffer.bindVertexBuffers(0, validBuffers.size(), validBuffers.data(),
                                            std::vector<vk::DeviceSize>(validBuffers.size(), 0).data());
            commandBuffer.drawIndexed(index.size(), 1, 0, 0, 0);
        }
    };

    struct Models : IRenderable
    {
        std::vector<std::unique_ptr<Model>> models;
        std::unique_ptr<BufferVector<Model::UniformBuffer>> uniforms;
        vk::PipelineLayout layout;

        Models(const Allocator &allocator, const std::size_t &size)
        {
            uniforms = std::make_unique<BufferVector<Model::UniformBuffer>>(
                allocator, size * sizeof(Model::UniformBuffer), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void addModel(std::unique_ptr<Model> model)
        {
            models.push_back(std::move(model));
            uniforms->vector.resize(models.size());
        }

        void updateUniforms()
        {
            for (size_t i = 0; i < models.size(); ++i)
            {
                uniforms->vector[i] = models[i]->uniform;
            }
            uniforms->sync();
        }

        void sync()
        {
            updateUniforms();
            for (auto &m : models)
            {
                m->cpyAttributes();
            }
        }

        void addLayout(vk::PipelineLayout &layout)
        {
            this->layout = layout;
        }

        virtual void draw(const vk::CommandBuffer &commandBuffer) override
        {
            updateUniforms();
            for (uint32_t i = 0; i < models.size(); ++i)
            {
                commandBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(uint32_t), &i);
                models.at(i)->draw(commandBuffer);
            }
        }
    };
}; // namespace letc

#endif // LETC_MODEL_HH
