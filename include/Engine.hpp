#include "Core/ResourceManager.hpp"
#include "Scene/SceneManager.hpp"
#include "Renderer/Renderer.hpp"
#include "Input/InputHandler.hpp"
#include "Input/Action.hpp"
#include "Renderer/Lighting.hpp"
#include "UI/UIManager.hpp"
#include "UI/FPSManager.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Debug/DebugRenderer.hpp"
#include "EngineContext.hpp"


#include <GLFW/glfw3.h>
#include <cstddef>


namespace Engine{
    struct GrappleEngine{    
        EngineResource::ResourceManager resourceManager;
        EngineInput::ActionManager actionManager;
        EngineInput::InputHandler inputHandler;
        EnginePhysics::PhysicsEngine physicsEngine;
        EngineUI::UIManager uiManager;
        EngineUI::FPSManager fpsManager; //Might just become debug manager in the future

        GrappleEngine();
    
        void init();

        void MainLoopExtraChecks();

        void RendererMainLoop(float deltaTime);

        void SaveScenes(){
            currentContext->sceneManager.saveScenes();
        }

        void mainLoop();

        void cleanup();
  
        void switchContexts(EngineContext& newContext);

        private:
          GLFWwindow *window = nullptr;
          EngineRenderer::Renderer renderer;
          EngineRenderer::Lighting lights;
          VkDescriptorPool imguiPool;

          EnginePartitioning::Spatial_Partitioner spatialPartitioner;
          DebugRenderer debugRenderer;

          uint32_t currentSceneIndex = 0;
          size_t totalObjects;

          EngineContext* currentContext = nullptr;

          EngineContext devContext;
  
          std::vector<EngineContext*> contexts = {&devContext}; //For looping over the engine contexts

          bool hasContextChanged = false;
    };
}
