#include "Engine.hpp"
#include "Camera/CameraManager.hpp"
#include "ECS/Components.hpp"
#include "Input/Input.hpp"
#include "Input/InputBindings.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RendererGlobals.hpp"
#include "Misc/ObjectGlobals.hpp"
#include "EngineUtility.hpp"
#include "ECS/EntityFunctions.hpp"
#include "Physics/Raycast.hpp"
#include "EngineGlobals.hpp"
#include "Spatial/Spatial_Partitioner.hpp"
#include "Context/ContextManager.hpp"
#include <algorithm>
#include <filesystem>

using namespace EngineInput;

namespace Engine{
    GrappleEngine::GrappleEngine() : renderer(){};

    void GrappleEngine::init(){
      std::filesystem::create_directories(GRAPPLE_SCENE_DIR); //Creates scenes directory if not present

      EngineUtility::initDebugSubSystems();

      renderer.initVulkan();

      spatialPartitioner.init(contextManager.currentContext);

      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

      VkDeviceSize minUboAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
      VkDeviceSize stride = sizeof(ObjectUBO);
      if (stride % minUboAlignment != 0) {
          stride += minUboAlignment - (stride % minUboAlignment);
      }

      renderer.setObjectUboStride(stride);

      renderer.initObjectResources(resourceManager);
  
      devContext.name = "Developer";
      contextManager.init(&spatialPartitioner, &inputHandler, &renderer, &uiManager, &resourceManager);
      contextManager.switchContexts(devContext);
      contextManager.primaryContext = &devContext;

      window = renderer.window;

      EngineRenderer::DirectionalLight sun;
      sun.direction = glm::normalize(glm::vec3(0.3f, -1.0f, 0.5f));
      sun.color = glm::vec3(1.0f, 0.5f, 0.2f);
      sun.intensity = 0.4f;
      lights.addDirectionalLight(sun);

      imguiPool = uiManager.createImGuiDescriptorPool();
      
      EngineUI::UIInfo uiInfo{};
      uiInfo.imGuiDescriptorPool = imguiPool;
      uiInfo.window = window;
      uiInfo.instance = renderer.getInstance();
      uiInfo.pipelineCache = nullptr;
      uiInfo.renderPass = renderer.getRenderPass();
      uiInfo.cameraManager = &contextManager.currentContext->sceneManager.getScenes()[0]->cameraManager;
      uiInfo.recourseManager = &resourceManager;
      uiInfo.spatial = &spatialPartitioner;
      uiInfo.context = &contextManager.currentContext;

      uiManager.initImGui(uiInfo);

      Input::get().init(window, &contextManager.currentContext);
      Input::get().setCallBacks();
      actionManager.setupDeveloperBindings();
      physicsEngine.init(&spatialPartitioner, &contextManager.currentContext);

      renderer.giveDebugRenderer(&debugRenderer);
    }

    //Small extra checks to ensure everything is up-to-date without cluttering main function
    void GrappleEngine::MainLoopExtraChecks(){
        //Updates the binding if the camera has changed
        if(contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.hasCameraChanged || contextManager.hasContextChanged){
            auto bindings = InputBindings::getDeveloperBindings(contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera().get(), 
                                                                 renderer.window, &actionManager);
            inputHandler.setBindings(bindings);
        }

        if(contextManager.currentContext->sceneManager.getCurrentScene()->areDescriptorSetsInitialized == false){
          renderer.createSceneDescriptorSets(contextManager.currentContext->sceneManager.getCurrentScene(), &contextManager.currentContext->ecs);
        }

        if(contextManager.currentContext->sceneManager.getCurrentScene()->areEntitiesInitialized == false){
          for(auto& e : contextManager.currentContext->ecs.view<RenderableComponent>()){
            EntityFunctions::initRenderResources(e, resourceManager, &contextManager.currentContext->ecs);
            EntityFunctions::initSimulationResources(e, &spatialPartitioner, &contextManager.currentContext->ecs);
          }

          renderer.createSceneDescriptorSets(contextManager.currentContext->sceneManager.getCurrentScene(), &contextManager.currentContext->ecs);
          contextManager.currentContext->sceneManager.getCurrentScene()->areEntitiesInitialized = true;          
        }

        while(contextManager.currentContext->sceneManager.getCurrentScene()->renderInitQueue.empty() == false){
            auto entity = contextManager.currentContext->sceneManager.getCurrentScene()->renderInitQueue.back();
            EntityFunctions::initRenderResources(entity, resourceManager, &contextManager.currentContext->ecs);

            contextManager.currentContext->sceneManager.getCurrentScene()->renderInitQueue.pop_back();
        }    

         while(contextManager.currentContext->sceneManager.getCurrentScene()->simulationInitQueue.empty() == false){
          auto entity = contextManager.currentContext->sceneManager.getCurrentScene()->simulationInitQueue.back();
          EntityFunctions::initSimulationResources(entity, &spatialPartitioner, &contextManager.currentContext->ecs);
          contextManager.currentContext->sceneManager.getCurrentScene()->simulationInitQueue.pop_back();
        }    


    }

    void GrappleEngine::RendererMainLoop(float deltaTime){
        glfwPollEvents();
        drawCallCountLastFrame = drawCallCount;
        drawCallCount = 0;

        Input::get().inputCamera = contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera().get();

        contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->giveExtent(renderer.getSwapChainExtent());

        contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->updateCamera(deltaTime, actionManager);

        static float rawFps = 0.0f;
        static float smoothFPS = 0.0f;
        if(rawFps != 0.0f){
            smoothFPS = fpsManager.smoothFPS(rawFps);
        }

        Input::get().selectedEntity = uiManager.getSelectedEntity();
        Input::get().inputScene = contextManager.currentContext->sceneManager.getCurrentScene();

        uiManager.renderUI(smoothFPS);

        EngineRenderer::UniformBufferObject ubo{};
        ubo.view = contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->getViewMatrix();
        if(!lights.getDirectionalLights().empty()){
            const auto& directionalLight = lights.getDirectionalLights()[0];
            ubo.lightDir = glm::vec4(glm::normalize(directionalLight.direction), 0.0f);
            ubo.lightColorIntensity = glm::vec4(directionalLight.color, directionalLight.intensity);
        }

        ubo.proj = contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->getProjection();
        ubo.cameraPos = glm::vec4(contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->position, 0.0f); //Updates the camera position
        renderer.updateUniformBuffers(ubo, contextManager.currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->getFov()); //Sends the uniform buffer object down to the uniform buffer manager for it to be processed
        
        auto* mappedBuffer = reinterpret_cast<ObjectUBO*>(renderer.getObjectUniformBuffersMapped()[renderer.getCurrentFrame()]);

        uint32_t i = 0;
        for(auto entity : contextManager.currentContext->ecs.view<RenderableComponent, MaterialComponent>()){
            auto* entity_material = contextManager.currentContext->ecs.getComponent<MaterialComponent>(entity);
            auto* entity_renderable = contextManager.currentContext->ecs.getComponent<RenderableComponent>(entity);

            ObjectUBO objectUbo{};
            objectUbo.hasTexture = entity_material->material->getTextures().empty() ? 0 : 1;
            objectUbo.baseColor = entity_material->material->getBaseColor();
            int index = 0;
            if(!entity_material->material->getTextures().empty()){
                auto sceneTextures = contextManager.currentContext->sceneManager.getCurrentScene()->getSceneTextures(&contextManager.currentContext->ecs);
                auto it = std::find(sceneTextures.begin(), sceneTextures.end(), entity_material->material->getTextures()[0]);
                if(it != sceneTextures.end()){
                    index = static_cast<int>(std::distance(sceneTextures.begin(), it));
                } else{
                    index = 0; //Fallback if texture was not found
                }
            }
            objectUbo.textureIndex = std::min(index, (int)MAX_TEXTURES - 1);
            
            memcpy((char*)renderer.getObjectUniformBuffersMapped()[renderer.getCurrentFrame()] + i * renderer.getObjectUboStride(), &objectUbo, sizeof(ObjectUBO));

            entity_renderable->uniformIndex = static_cast<uint32_t>(i);

            i++;
        }

        if(uiManager.shouldDrawBoundingBoxes()){
            contextManager.currentContext->sceneManager.getCurrentScene()->drawBoundingBoxes(&debugRenderer, &contextManager.currentContext->ecs);
        }

        FrameFlags frameFlags{};
        frameFlags.shouldDrawBoundingBoxes = uiManager.shouldDrawBoundingBoxes();

        renderer.drawFrame(contextManager.currentContext->sceneManager.getCurrentScene(), &contextManager.currentContext->ecs, frameFlags);
        rawFps = renderer.getGpuFPS();
    }

    void GrappleEngine::mainLoop(){
        auto lastTime = std::chrono::high_resolution_clock::now();

        float accumulator = 0.0f;
        while(!glfwWindowShouldClose(window)){            
          auto currentTime = std::chrono::high_resolution_clock::now();
          float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
          lastTime = currentTime;

          float physicsStep = physicsEngine.getTickRate();

          contextManager.currentContext->sceneManager.getCurrentScene()->update(&contextManager.currentContext->ecs); //Updates the current frame's children with it's matrix and so forth

          MainLoopExtraChecks();

          inputHandler.update(window, actionManager, contextManager.currentContext->sceneManager);

          accumulator+=deltaTime;
          while(accumulator >= physicsStep){
              physicsEngine.tick(physicsStep);
              accumulator-=physicsEngine.getTickRate();
          }
          
          RendererMainLoop(deltaTime); 
        }
        vkDeviceWaitIdle(EngineRenderer::device);
    }

    void GrappleEngine::cleanup(){
        vkDeviceWaitIdle(device);
        resourceManager.cleanup(device);
        defaultResources.cleanupDefault(); //Destroys the default recourses
        uiManager.shutDownImGui(imguiPool);
        renderer.cleanup();
        contextManager.serialize();
        contextManager.cleanup();
    }
}
