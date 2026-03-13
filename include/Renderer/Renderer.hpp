#ifndef _RENDERER_HPP
#define _RENDERER_HPP

#include <cstdint>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "Debug/DebugRenderer.hpp"
#include "Window.hpp"
#include "Instance.hpp"
#include "Pipeline.hpp"
#include "Command.hpp"
#include "Vertex.hpp"
#include "UniformBuffer.hpp"
#include "RendererUtilities.hpp"

#include "Scene/Scene.hpp"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

struct FrameFlags{
    bool shouldDrawBoundingBoxes = false;
};

using namespace EngineScene;
namespace EngineRenderer {
    class Renderer{
        private:
            void createSyncObjects();
            void initQueryPool();

            Window appWindow;
            
            VkSurfaceKHR surface;

            VkInstance instance;
            Instance appInstance{instance};

            Queue appQueue;

            SwapChain appSwapChain;
            VkSwapchainKHR swapChain;

            PipelineManager appPipeline;

            VkQueryPool queryPool;

            Vertex vertexCommand;
            VertexBuffer vertexBufferCommand;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            IndexBuffer indexBufferCommand;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;

            UniformBuffer uniformBufferCommand;
            VkDescriptorSetLayout descriptorSetLayout;
            VkDescriptorPool descriptorPool;
            std::vector<VkBuffer> uniformBuffers;
            std::vector<VkDeviceMemory> uniformBuffersMemory;
            std::vector<void*> uniformBuffersMapped;

            std::vector<VkBuffer> objectUniformBuffers;
            std::vector<VkDeviceMemory> objectUniformBuffersMemory;
            std::vector<void*> objectUniformBuffersMapped;

            VkDeviceSize objectUboStride;

            Command appCommand;
            std::vector<VkCommandBuffer> commandbuffers;

            std::vector<VkSemaphore> imageAvailableSemaphores;
            std::vector<VkSemaphore> renderFinishedSemaphores;
            std::vector<VkFence> inFlightFences;
            std::vector<VkFence> imagesInFlight;

            uint32_t currentFrame = 0;

            public: 
                Renderer();

                GLFWwindow* window = nullptr;
                bool framebuffersrResized = false;

                void initVulkan();
                void initObjectResources(EngineResource::ResourceManager &resourceManager);
                void initEntities(Scene &scene, EngineResource::ResourceManager &resourceManager, 
                                 EnginePartitioning::Spatial_Partitioner &SpatialPartitioner, ECS* ecs);
                void cleanup();
                void drawFrame(Scene *scene, ECS* ecs, FrameFlags frameFlags);
                void createSceneDescriptorSets(Scene *scene, ECS* ecs);

                void updateUniformBuffers(UniformBufferObject &ubo, float fov){
                    uniformBufferCommand.updateUniformBuffers(ubo, fov, currentFrame, appSwapChain.swapChainExtent, uniformBuffersMapped);
                }

                void updateObjectUniformBuffers(ObjectUBO &objectUBO){
                    uniformBufferCommand.updateObjectUniformBuffers(objectUBO, objectUniformBuffersMapped, currentFrame);
                }
                void giveDebugRenderer(DebugRenderer *debugRenderer){
                    appCommand.giveDebugRenderer(debugRenderer);
                }

                float getGpuFPS();
                VkInstance& getInstance(){return instance;}
                VkDescriptorPool& getDescriptorPool(){return descriptorPool;}
                VkRenderPass& getRenderPass(){return appPipeline.renderPass;}
                VkCommandBuffer& getCurrentCommandBuffer(){return commandbuffers[currentFrame];}
                VkExtent2D getSwapChainExtent(){return appSwapChain.swapChainExtent;}
                int getCurrentFrame() {return currentFrame;}
                void setObjectUboStride(VkDeviceSize objectUboStride){this->objectUboStride = objectUboStride;}
                VkDeviceSize getObjectUboStride(){return objectUboStride;}
                std::vector<void*>& getObjectUniformBuffersMapped(){return objectUniformBuffersMapped;}
    };
}

#endif
