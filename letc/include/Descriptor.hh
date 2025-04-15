#pragma once

#include <ranges>

#include "pch.hh"

#include "spirv_reflect.hh"

#include "Device.hh"

namespace letc
{
    struct DescriptorManager
    {
        const Device &device;
        vk::UniqueDescriptorPool pool;

        std::unordered_map<std::string, std::vector<vk::DescriptorSetLayoutBinding>> layoutBindings;
        std::unordered_map<std::string, vk::UniqueDescriptorSetLayout> setLayouts;

        // maps the set instance back to the set layout so we can retrieve the info for writes
        std::unordered_map<std::string, std::reference_wrapper<std::vector<vk::DescriptorSetLayoutBinding>>> setInfos;
        std::unordered_map<std::string, vk::UniqueDescriptorSet> sets;

        DescriptorManager(const Device &device) : device(device)
        {
            std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1024},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, 1024},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1024},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1024},
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1024},
            };

            vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
            descriptorPoolInfo.setMaxSets(1024);
            descriptorPoolInfo.setPoolSizes(descriptorPoolSizes);
            descriptorPoolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
            this->pool = device.device.createDescriptorPoolUnique(descriptorPoolInfo);
        }

        DescriptorManager &addLayoutBinding(const std::string &setLayoutId, const uint32_t binding,
                                            const vk::DescriptorType type, const vk::ShaderStageFlags stages,
                                            const uint32_t count = 1)
        {
            auto &bindings = layoutBindings[setLayoutId];
            auto duplicate = std::ranges::find_if(
                bindings, [binding](const vk::DescriptorSetLayoutBinding &dslb) { return dslb.binding == binding; });

            if (duplicate != bindings.end())
            {
                *duplicate = vk::DescriptorSetLayoutBinding{binding, type, count, stages};
            }
            else
            {
                bindings.push_back(vk::DescriptorSetLayoutBinding{binding, type, count, stages});
            }
            return *this;
        }

        DescriptorManager &createLayout(const std::string &setLayoutId)
        {
            auto it = layoutBindings.find(setLayoutId);
            if (it != layoutBindings.end())
            {
                setLayouts[setLayoutId] = device.device.createDescriptorSetLayoutUnique(
                    vk::DescriptorSetLayoutCreateInfo{}.setBindings(it->second));
            }
            else
            {
                throw std::runtime_error("no bindings in set");
            }
            return *this;
        }

        // I think making me do it manually will make it easier to code flow?
        // DescriptorManager &createAllLayouts()
        // {
        //     for (const auto &pair : layoutBindings)
        //     {
        //         const std::string &setId = pair.first;
        //         setLayouts[setId] = device.device.createDescriptorSetLayoutUnique(
        //             vk::DescriptorSetLayoutCreateInfo{}.setBindings(pair.second));
        //     }
        //     return *this;
        // }

        DescriptorManager &createSet(const std::string &setLayoutId, const std::string &setId)
        {
            auto layout = setLayouts.find(setLayoutId);
            assertThrow(layout != setLayouts.end(), "layout must be valid");

            vk::DescriptorSetAllocateInfo allocateInfo{};
            allocateInfo.setDescriptorPool(pool.get());
            allocateInfo.setSetLayouts(layout->second.get());
            sets[setId] = std::move(device.device.allocateDescriptorSetsUnique(allocateInfo).at(0));
            setInfos.emplace(setId, std::ref(layoutBindings.at(setLayoutId)));
            return *this;
        }

        DescriptorManager &attachBuffer(const std::string &setId, const uint32_t &binding, const vk::Buffer &buffer,
                                        const vk::DeviceSize &range, const vk::DeviceSize &offset = 0)
        {
            auto set = sets.find(setId);
            assertThrow(set != sets.end(), "set instance is not valid");

            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.setBuffer(buffer);
            bufferInfo.setRange(range);
            bufferInfo.setOffset(offset);

            auto setInfo =
                std::ranges::find_if(setInfos.at(setId).get(), [binding](const vk::DescriptorSetLayoutBinding &dslb) {
                    return dslb.binding == binding;
                });

            vk::WriteDescriptorSet writeDescriptor{};
            writeDescriptor.setDstSet(set->second.get());
            writeDescriptor.setDstBinding(binding);
            writeDescriptor.setDescriptorCount(1);
            writeDescriptor.setDescriptorType(setInfo->descriptorType);
            writeDescriptor.setBufferInfo(bufferInfo);
            device.device.updateDescriptorSets(writeDescriptor, {});

            return *this;
        }

        DescriptorManager &attachImage(const std::string &setId, const uint32_t binding, const vk::ImageView &imageView,
                                       const vk::Sampler &sampler,
                                       const vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            auto set = sets.find(setId);
            assertThrow(set != sets.end(), "set instance is not valid");

            vk::DescriptorImageInfo imageInfo{};
            imageInfo.setImageView(imageView);
            imageInfo.setSampler(sampler);
            imageInfo.setImageLayout(layout);

            auto setInfo =
                std::ranges::find_if(setInfos.at(setId).get(), [binding](const vk::DescriptorSetLayoutBinding &dslb) {
                    return dslb.binding == binding;
                });

            vk::WriteDescriptorSet writeDescriptor{};
            writeDescriptor.setDstSet(set->second.get());
            writeDescriptor.setDstBinding(binding);
            writeDescriptor.setDescriptorCount(1);
            writeDescriptor.setDescriptorType(setInfo->descriptorType);
            writeDescriptor.setImageInfo(imageInfo);
            device.device.updateDescriptorSets(writeDescriptor, {});

            return *this;
        }

        DescriptorManager &attachTexelBuffer(const std::string &setId, const uint32_t binding,
                                             const vk::BufferView &bufferView)
        {
            auto set = sets.find(setId);
            assertThrow(set != sets.end(), "set instance is not valid");

            auto setInfo =
                std::ranges::find_if(setInfos.at(setId).get(), [binding](const vk::DescriptorSetLayoutBinding &dslb) {
                    return dslb.binding == binding;
                });

            vk::WriteDescriptorSet writeDescriptor{};
            writeDescriptor.setDstSet(set->second.get());
            writeDescriptor.setDstBinding(binding);
            writeDescriptor.setDescriptorCount(1);
            writeDescriptor.setDescriptorType(setInfo->descriptorType);
            writeDescriptor.setTexelBufferView(bufferView);
            device.device.updateDescriptorSets(writeDescriptor, {});

            return *this;
        }

        template <typename... Args> std::vector<vk::DescriptorSet> organizeSets(const Args &...setIds)
        {
            std::vector<vk::DescriptorSet> organizedSets;
            organizedSets.reserve(sizeof...(setIds));
            ((organizedSets.push_back(sets.at(std::string(setIds)).get())), ...);
            return organizedSets;
        }
    };

    struct DescriptorLayout
    {
        const Device &device;
        std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindings;
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

        DescriptorLayout(const Device &device) : device(device)
        {
        }

        DescriptorLayout &addBinding(const uint32_t &set, const uint32_t &binding, const vk::DescriptorType &type,
                                     const vk::ShaderStageFlags &stageFlags, const uint32_t &descriptorCount)
        {
            descriptorSetLayoutBindings[set][binding] = vk::DescriptorSetLayoutBinding{}
                                                            .setBinding(binding)
                                                            .setDescriptorType(type)
                                                            .setStageFlags(stageFlags)
                                                            .setDescriptorCount(descriptorCount);

            return *this;
        }

        void generateLayouts()
        {
            descriptorSetLayouts.clear();
            for (const auto &setBindings : descriptorSetLayoutBindings)
            {
                std::vector<vk::DescriptorSetLayoutBinding> bindings;
                for (const auto &bindingPair : setBindings.second)
                {
                    bindings.push_back(bindingPair.second);
                }
                vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
                descriptorSetLayouts.push_back(device.device.createDescriptorSetLayout(layoutInfo));
            }
        }

        ~DescriptorLayout()
        {
            for (const auto &layout : descriptorSetLayouts)
            {
                device.device.destroyDescriptorSetLayout(layout);
            }
        }
    };
} // namespace letc
