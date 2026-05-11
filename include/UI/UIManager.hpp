#ifndef _UI_MANAGER_HPP
#define _UI_MANAGER_HPP

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "ECS/EntityFunctions.hpp"
#include "EngineUI.hpp"
#include "Context/EngineContext.hpp"
#include "Scene/SceneManager.hpp"
#include "Camera/CameraManager.hpp"
#include "Core/ResourceManager.hpp"
#include "imgui.h"

namespace EngineUI{
    struct UIInfo{
        GLFWwindow* window;
        VkInstance instance;
        VkPipelineCache pipelineCache;
        VkDescriptorPool imGuiDescriptorPool;
        VkRenderPass renderPass;
        EngineCamera::CameraManager *cameraManager;
        EngineResource::ResourceManager *recourseManager;
        EnginePartitioning::Spatial_Partitioner* spatial;
        EngineContext** context = nullptr;
    };

    class UIManager{
        public:
            void initImGui(UIInfo &uiInfoRef);

            void beginFrame(float fps);
            void renderUI(float fps);

            VkDescriptorPool createImGuiDescriptorPool();

            void shutDownImGui(VkDescriptorPool &imGuiDescriptorPool);

            bool shouldDrawBoundingBoxes(){return engineUI.getValueofDrawBoundingBoxes();}

            void setCameraManager(EngineCamera::CameraManager *cameraManager){uiInfo.cameraManager = cameraManager;}

            private:
                EngineUI engineUI;

                UIInfo uiInfo;

                float scale = 16.0f;
                ImGuiStyle baseStyle;
            
            public:
                Entity getSelectedEntity(){return engineUI.getSelectedEntity();}
                EngineScene::Scene* getSelectedScene(){return engineUI.getSelectedScene();}
    };
}

#endif
