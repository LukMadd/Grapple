#include "Renderer/Renderer.hpp"
#include "Renderer/Instance.hpp"
#include "Core/ResourceManager.hpp"
#include "Misc/ObjectGlobals.hpp"
#include "Renderer/RendererGlobals.hpp"
#include "Renderer/ValidationLayers.hpp"
#include "ECS/EntityFunctions.hpp"
#include <cstdint>

namespace EngineRenderer{
    Renderer::Renderer() : appWindow(){}

    void Renderer::initVulkan(){
        appWindow.initWindow(WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT, "My_Vulkan_Engine");
        window = appWindow.getWindow();
        glfwSetWindowUserPointer(appWindow.getWindow(), this);
        appSwapChain.initSwapChain(window);
        appInstance.createInstance(instance);
        appWindow.createSurface(instance, window,surface);
        appInstance.pickPhysicalDevice(instance, surface);
        queueFamilyIndices = appQueue.FindQueueFamily(physicalDevice, surface);
        EngineRenderer::device = appInstance.createLogicalDevice(surface, queueFamilyIndices);
        vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
        appSwapChain.createSwapChain(surface, queueFamilyIndices);
        swapChain = appSwapChain.getSwapChain();
        appSwapChain.createImageViews();
        renderFinishedSemaphores.resize(appSwapChain.swapChainImages.size());        
        appPipeline.createRenderPass(appSwapChain.swapChainImageFormat);
        appCommand.createCommandPool(surface, queueFamilyIndices);
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(appSwapChain.swapChainImages.size(), VK_NULL_HANDLE);
        initQueryPool();
    }

    void Renderer::initQueryPool(){
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = 2; 
        VkResult result = vkCreateQueryPool(device, &queryPoolInfo, nullptr, &queryPool);
        if(result != VK_SUCCESS){
            throw std::runtime_error("Failed to create query pool!");
        }
        setObjectName(device,(uint64_t)queryPool, VK_OBJECT_TYPE_QUERY_POOL, "Main_Query_Pool");
    }

    float Renderer::getGpuFPS(){
        vkQueueWaitIdle(graphicsQueue);

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        auto timestampPeriod = properties.limits.timestampPeriod;

        uint64_t timestamps[2];
        vkGetQueryPoolResults(device, queryPool, 0, 2, sizeof(timestamps), timestamps, sizeof(uint64_t),
                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        
        float frameTimeNS = (timestamps[1] - timestamps[0]) * timestampPeriod;
        float fps = 1e9 / frameTimeNS;

        return fps;
    }
    
    void Renderer::initObjectResources(EngineResource::ResourceManager &resourceManager){
        defaultResources.init(resourceManager);
        uniformBufferCommand.createDescriptorSetLayout(descriptorSetLayout);
        appPipeline.createPipelines(appSwapChain.swapChainExtent, descriptorSetLayout);
        multiSampler.createColorResources(appSwapChain.swapChainImageFormat, appSwapChain.swapChainExtent);
        depthBuffer.createDepthResources(appSwapChain.swapChainExtent, depthBuffer.depthImage, depthBuffer.depthImageMemory, depthBuffer.depthImageView);
        appSwapChain.createFramebuffers(appPipeline.renderPass, depthBuffer.depthImageView, multiSampler.colorImageView);
        uniformBufferCommand.createUniformBuffers(MAX_FRAMES_IN_FLIGHT, sizeof(UniformBufferObject), uniformBuffers, uniformBuffersMemory, uniformBuffersMapped);
        uniformBufferCommand.createUniformBuffers(MAX_FRAMES_IN_FLIGHT, objectUboStride * MAX_OBJECTS, objectUniformBuffers, objectUniformBuffersMemory, objectUniformBuffersMapped);
        uniformBufferCommand.createDescriptorPool(MAX_FRAMES_IN_FLIGHT, descriptorPool);
        appCommand.createCommandBuffers(commandbuffers, MAX_FRAMES_IN_FLIGHT);
        createSyncObjects();
    }

    void Renderer::createSceneDescriptorSets(Scene *scene, ECS* ecs){
        if(!descriptorSetLayout){
            uniformBufferCommand.createDescriptorSetLayout(descriptorSetLayout);
        }
        uniformBufferCommand.createDescriptorSets(MAX_FRAMES_IN_FLIGHT, uniformBuffers, objectUniformBuffers, descriptorSetLayout, descriptorPool, scene->descriptorSets, scene->getSceneTextures(ecs));
        scene->areDescriptorSetsInitialized = true;
    }

    void Renderer::createSyncObjects(){
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS){
                throw std::runtime_error("Failed to create semaphores!");
            }
        }   

        for(size_t i = 0; i < renderFinishedSemaphores.size(); i++){
            if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS){
                throw std::runtime_error("Failed to create renderFinished semaphore!");
            }
        }
    }
    
    void Renderer::drawFrame(Scene *scene, ECS* ecs, FrameFlags frameFlags){
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            framebuffersrResized = false;
            appSwapChain.recreateSwapChain(surface, queueFamilyIndices, appPipeline.renderPass, depthBuffer, multiSampler, appWindow.getWindow());
            swapChain = appSwapChain.getSwapChain();
            return;
        } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        if(imagesInFlight[imageIndex] != VK_NULL_HANDLE){
            vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandbuffers[currentFrame], 0);

        if(!wireFrameModeEnabled){
            appCommand.recordCommandBuffers(ecs, scene, commandbuffers[currentFrame], 
                                            imageIndex, appSwapChain, appPipeline, 
                                            currentFrame, vertexBuffer, indexBuffer, queryPool, 
                                            frameFlags.shouldDrawBoundingBoxes, objectUboStride);
        } else{
            appCommand.recordCommandBuffers(ecs, scene, commandbuffers[currentFrame], 
                                            imageIndex, appSwapChain, appPipeline, 
                                            currentFrame, vertexBuffer, indexBuffer, queryPool, 
                                            frameFlags.shouldDrawBoundingBoxes, objectUboStride);
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandbuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        if(submitResult != VK_SUCCESS){
            throw std::runtime_error("Failed to submit queue error code: " + std::to_string(submitResult) + "!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffersrResized){
            framebuffersrResized = false;
            appSwapChain.recreateSwapChain(surface, queueFamilyIndices, appPipeline.renderPass, depthBuffer,multiSampler, appWindow.getWindow());
            swapChain = appSwapChain.getSwapChain();
            return;
        } else if(result != VK_SUCCESS){
            throw std::runtime_error("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::cleanup(){
        appSwapChain.cleanupSwapChain(depthBuffer, multiSampler);
        depthBuffer.cleanup();

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        for(size_t i = 0; i < renderFinishedSemaphores.size(); i++){
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        }

        appCommand.cleanup();

        appPipeline.cleanupPipelines();

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        for(int i = 0; i < uniformBuffers.size(); i++){
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        }
        for(int i = 0; i < uniformBuffersMemory.size(); i++){
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
        for(int i = 0; i < objectUniformBuffers.size(); i++){
            vkDestroyBuffer(device, objectUniformBuffers[i], nullptr);
        }
        for(int i = 0; i < objectUniformBuffersMemory.size(); i++){
            vkFreeMemory(device, objectUniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        uniformBufferCommand.cleanup();

        vkDestroyQueryPool(device, queryPool, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(destroyFunc){
            destroyFunc(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
}
