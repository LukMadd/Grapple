#include "Renderer/Command.hpp"
#include "ECS/Components.hpp"
#include "Renderer/Pipeline.hpp"
#include "Renderer/RendererGlobals.hpp"
#include "Renderer/ValidationLayers.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "Renderer/UniformBuffer.hpp"
#include "Debug/DebugRenderer.hpp"
#include <cstdint>

namespace EngineRenderer{
    void Command::drawLines(VkCommandBuffer commandBuffer, PipelineManager pipelineManager, EngineCamera::Camera *camera){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineManager.pipelines["DEBUG_PIPELINE"]);

        debugRenderer->setupLines(vertexBuffer, indexBuffer, camera->getViewMatrix(), camera->getFront());
        if(vertexBuffer.buffer == VK_NULL_HANDLE){
          return;
        }

        VkDeviceSize offsets[] = {0};

        uint32_t indexCount = static_cast<uint32_t>(indexBuffer.indices.size());        

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        glm::mat4 proj = camera->getProjection();
        proj[1][1]*=-1;
        glm::mat4 view = camera->getViewMatrix();

        Matricies mats{};
        mats.proj = proj;
        mats.view = view;

        vkCmdPushConstants(commandBuffer, pipelineManager.debugLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matricies), 
                  &mats);

        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

        vertexBuffer.vertices.clear();
        indexBuffer.indices.clear();
    }

    void Command::createCommandPool(VkSurfaceKHR surface, QueueFamilyIndices queueFamilyIndices){
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VkResult result = vkCreateCommandPool(device,&poolInfo, nullptr, &commandPool);
        if(result != VK_SUCCESS){
            throw std::runtime_error("Failed to create command pool error code: " + std::to_string(result) + "!");
        }
        setObjectName(device, (uint64_t)commandPool, VK_OBJECT_TYPE_COMMAND_POOL, "Main_Command_Pool");
    }

    void Command::cleanup(){
        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyBuffer(device, vertexBuffer.buffer, nullptr);
        vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
        vkFreeMemory(device, vertexBuffer.bufferMemory, nullptr);
        vkFreeMemory(device, indexBuffer.bufferMemory, nullptr);
    }

    void Command::createCommandBuffers(std::vector<VkCommandBuffer> &commandBuffers, int MAX_FRAMES_IN_FLIGHT){
        if(commandPool == VK_NULL_HANDLE){
            throw std::runtime_error("Command pool invalid");
        }
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        VkResult result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
        if(result != VK_SUCCESS){
            throw std::runtime_error("Failed to allocate command buffers error code " + std::to_string(result) + "!");
        }
    }

    void Command::recordCommandBuffers(
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
        VkDeviceSize objectUboStride){

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        
        VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if(beginResult != VK_SUCCESS){
            throw std::runtime_error("Failed to begin command buffers error code " + std::to_string(beginResult) + "!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = pipelineManager.renderPass;
        renderPassInfo.framebuffer = swapChain.swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};        
        clearValues[0].color = {{0.39f, 0.58f, 0.93f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdResetQueryPool(commandBuffer, queryPool, 0, 2);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, 0);

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        if(wireFrameModeEnabled){
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.graphicsPipelineWireFrame);
        } else{
            for(auto& entity : ecs->view<RenderableComponent, MaterialComponent>()){
                auto entity_material = ecs->getComponent<MaterialComponent>(entity);
                // MAKE IT NOT RE BIND A PIPELINE
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                        *pipelineManager.pipelines[entity_material->material->getShader().fragShader.c_str()]);
            }
        }
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChain.swapChainExtent.width);
        viewport.height = static_cast<float>(swapChain.swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChain.swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        for(auto& entity : ecs->view<RenderableComponent>()){
            if(!scene->descriptorSets[0]){
                throw std::runtime_error("Invalid descriptor sets for binding!");
            }

            uint32_t uniformIndex = ecs->getComponent<RenderableComponent>(entity)->uniformIndex;


            uint32_t dynamicOffset = uniformIndex * objectUboStride;
            vkCmdBindDescriptorSets(commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineManager.pipelineLayout,
                                    0, 1, &scene->descriptorSets[currentFrame],
                                    1, &dynamicOffset);
           
            EntityFunctions::draw(entity, commandBuffer, pipelineManager.pipelineLayout, ecs);

            drawCallCount++;
        }

        ImDrawData* imguiDrawData = ImGui::GetDrawData();
        if (imguiDrawData && imguiDrawData->CmdListsCount > 0) {
            ImGui_ImplVulkan_RenderDrawData(imguiDrawData, commandBuffer);
        }

        if(shouldDrawBoundingBoxes){
            drawLines(commandBuffer, pipelineManager, scene->cameraManager.getCurrentCamera().get());
        }
        
        vkCmdEndRenderPass(commandBuffer);

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);

        VkResult endResult = vkEndCommandBuffer(commandBuffer);
        if(endResult != VK_SUCCESS){
            throw std::runtime_error("Failed to record command buffers error code " + std::to_string(endResult) + "!");
        }
    }
}
