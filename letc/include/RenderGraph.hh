#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <taskflow/taskflow.hpp>

#include "Device.hh"
#include "Pipeline.hh"
#include "Renderable.hh"

namespace letc
{
    enum GraphResourceType
    {
        Renderable,
        UniformBuffer,
        DynamicUniformBuffer,
        StorageBuffer,
        DynamicStorageBuffer,
    };

    struct GraphResource
    {
        std::string id;
        GraphResourceType type;
        union
        {
            IRenderable *renderable;

        };
    };

    struct GraphPipeline
    {
        std::string id;
        std::unique_ptr<IPipeline> pipeline;
    };

    class RenderGraph
    {
        const Device &device;

        std::unordered_map<std::string, tf::Task> tasks;
        tf::Executor executer;
        tf::Taskflow taskflow;

        vk::CommandBuffer *commandBuffer = nullptr;

        RenderGraph(const Device &device) : device(device)
        {
        }

        void addResource(const GraphResource &graphResource)
        {
        }

        void addTask()
        {
        }
    };
}; // namespace letc
