#ifndef _COMMAND_HPP
#define _COMMAND_HPP


#include <vulkan/vulkan.hpp>
#include "Renderer/Pipeline.hpp"
#include "Scene/Scene.hpp"
#include "SwapChain.hpp"
#include "Queue.hpp"
#include "Debug/DebugRenderer.hpp"

using namespace EngineScene;

namespace EngineRenderer{
    class Command{
        public:
            void drawLines(VkCommandBuffer commandBuffer, PipelineManager pipelineManager, EngineCamera::Camera *camera);

            void createCommandPool(VkSurfaceKHR surface, QueueFamilyIndices queueFamilyIndices);

            void cleanup();

            void createCommandBuffers(std::vector<VkCommandBuffer> &commandBuffers, int MAX_FRAMES_IN_FLIGHT);

            void giveDebugRenderer(DebugRenderer *_debugRenderer){
                debugRenderer = _debugRenderer;
            }

            void recordCommandBuffers(
                ECS* ecs, 
                Scene* scene,
                VkCommandBuffer commandBuffer, 
                uint32_t imageIndex, 
                SwapChain swapChain, 
                PipelineManager &pipelineManager,
                uint32_t currentFrame, 
                VkBuffer &vertexBuffer, 
                VkBuffer &indexBuffer, 
                VkQueryPool &queryPool,
                bool shouldDrawBoundingBoxes,
                VkDeviceSize objectUboStride);

    private:
        DebugRenderer *debugRenderer;

        //For debug renderering
        VertexBuffer vertexBuffer;
        IndexBuffer indexBuffer;
    };
}

#endif
