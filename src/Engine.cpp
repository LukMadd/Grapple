#include "Engine.hpp"
#include "Camera/CameraManager.hpp"
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
#include <algorithm>
#include <filesystem>

using namespace EngineInput;

namespace Engine{
    GrappleEngine::GrappleEngine() : renderer(){};

    void GrappleEngine::switchContexts(EngineContext& newContext){
      currentContext = &newContext;

      spatialPartitioner.reset();
      spatialPartitioner.init(&newContext);
      if(newContext.isSetup == false){
        newContext.sceneManager.init(resourceManager, &spatialPartitioner, &newContext.ecs);

        for(auto &scene : newContext.sceneManager.getScenes()){
            renderer.initEntities(*scene, resourceManager, spatialPartitioner, &newContext.ecs);
        }

        renderer.createSceneDescriptorSets(newContext.sceneManager.getCurrentScene(), &newContext.ecs);

        newContext.isSetup = true;
      }

      if(std::binary_search(contexts.begin(), contexts.end(), &newContext) == false){
        contexts.push_back(&newContext);
      }

      uiManager.setCameraManager(&newContext.sceneManager.getCurrentScene()->cameraManager);

      hasContextChanged = true;
    }

    void GrappleEngine::init(){
      std::filesystem::create_directories(GRAPPLE_SCENE_DIR); //Creates scenes directory if not present

      EngineUtility::initDebugSubSystems();

      renderer.initVulkan();

      spatialPartitioner.init(currentContext);

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
      switchContexts(devContext);

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
      uiInfo.cameraManager = &currentContext->sceneManager.getScenes()[0]->cameraManager;
      uiInfo.recourseManager = &resourceManager;
      uiInfo.changedBoundingBoxes = &physicsEngine.getChangedBoundingBoxes();
      uiInfo.context = &currentContext;

      uiManager.initImGui(uiInfo);

      Input::get().init(window, &currentContext);
      Input::get().setCallBacks();
      actionManager.setupDeveloperBindings();
      physicsEngine.init(&spatialPartitioner, &currentContext);

      renderer.giveDebugRenderer(&debugRenderer);
    }

    //Small extra checks to ensure everything is up-to-date without cluttering main function
    void GrappleEngine::MainLoopExtraChecks(){
        //Updates the binding if the camera has changed
        if(currentContext->sceneManager.getCurrentScene()->cameraManager.hasCameraChanged || hasContextChanged){
            auto bindings = InputBindings::getDeveloperBindings(currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera().get(), 
                                                                 renderer.window, &actionManager);
            inputHandler.setBindings(bindings);
        }
    }

    void GrappleEngine::RendererMainLoop(float deltaTime){
        glfwPollEvents();
        drawCallCountLastFrame = drawCallCount;
        drawCallCount = 0;

        Input::get().inputCamera = currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera().get();

        currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->giveExtent(renderer.getSwapChainExtent());

        currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->updateCamera(deltaTime, actionManager);

        static float rawFps = 0.0f;
        static float smoothFPS = 0.0f;
        if(rawFps != 0.0f){
            smoothFPS = fpsManager.smoothFPS(rawFps);
        }

        Input::get().selectedEntity = uiManager.getSelectedEntity();
        Input::get().inputScene = currentContext->sceneManager.getCurrentScene();

        uiManager.renderUI(smoothFPS);

        if(currentContext->sceneManager.getCurrentScene()->areEntitiesInitialized == false){
            renderer.initEntities(*currentContext->sceneManager.getCurrentScene(), resourceManager, spatialPartitioner, &currentContext->ecs);
            renderer.createSceneDescriptorSets(currentContext->sceneManager.getCurrentScene(), &currentContext->ecs);
        }

        if(currentContext->sceneManager.getCurrentScene()->newEntities.empty() == false){
            while(currentContext->sceneManager.getCurrentScene()->newEntities.empty() == false){
                auto entity = currentContext->sceneManager.getCurrentScene()->newEntities.back();
                EntityFunctions::initResources(entity, resourceManager, &spatialPartitioner, &currentContext->ecs);
                currentContext->sceneManager.getCurrentScene()->newEntities.pop_back();
            }    
        }

        if(currentContext->sceneManager.getCurrentScene()->areDescriptorSetsInitialized == false){
            renderer.createSceneDescriptorSets(currentContext->sceneManager.getCurrentScene(), &currentContext->ecs);
        }

        EngineRenderer::UniformBufferObject ubo{};
        ubo.view = currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->getViewMatrix();
        if(!lights.getDirectionalLights().empty()){
            const auto& directionalLight = lights.getDirectionalLights()[0];
            ubo.lightDir = glm::vec4(glm::normalize(directionalLight.direction), 0.0f);
            ubo.lightColorIntensity = glm::vec4(directionalLight.color, directionalLight.intensity);
        }

        ubo.proj = currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->getProjection();
        ubo.cameraPos = glm::vec4(currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->position, 0.0f); //Updates the camera position
        renderer.updateUniformBuffers(ubo, currentContext->sceneManager.getCurrentScene()->cameraManager.getCurrentCamera()->getFov()); //Sends the uniform buffer object down to the uniform buffer manager for it to be processed
        
        auto* mappedBuffer = reinterpret_cast<ObjectUBO*>(renderer.getObjectUniformBuffersMapped()[renderer.getCurrentFrame()]);

        uint32_t i = 0;
        for(auto entity : currentContext->ecs.view<RenderableComponent, MaterialComponent>()){
            auto* entity_material = currentContext->ecs.getComponent<MaterialComponent>(entity);
            auto* entity_renderable = currentContext->ecs.getComponent<RenderableComponent>(entity);

            ObjectUBO objectUbo{};
            objectUbo.hasTexture = entity_material->material->getTextures().empty() ? 0 : 1;
            objectUbo.baseColor  = entity_material->material->getBaseColor();
            int index = 0;
            if(!entity_material->material->getTextures().empty()){
                auto sceneTextures = currentContext->sceneManager.getCurrentScene()->getSceneTextures(&currentContext->ecs);
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
            currentContext->sceneManager.getCurrentScene()->drawBoundingBoxes(&debugRenderer, &currentContext->ecs);
        }

        FrameFlags frameFlags{};
        frameFlags.shouldDrawBoundingBoxes = uiManager.shouldDrawBoundingBoxes();

        renderer.drawFrame(currentContext->sceneManager.getCurrentScene(), &currentContext->ecs, frameFlags);
        rawFps = renderer.getGpuFPS();
    }

    void GrappleEngine::mainLoop(){
        auto lastTime = std::chrono::high_resolution_clock::now();

        while(!glfwWindowShouldClose(window)){            
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            lastTime = currentTime;

            MainLoopExtraChecks();

            inputHandler.update(window, actionManager, currentContext->sceneManager);

            float accumulator;

            float physicsStep = physicsEngine.getTickRate();

            currentContext->sceneManager.getCurrentScene()->update(&currentContext->ecs); //Updates the current frame's children with it's matrix and so forth

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
        for(auto& context : contexts){
          if(!context->isSetup) continue;
          context->sceneManager.saveScenes(); //Serializes all the scenes loaded
          for(auto &scene :  context->sceneManager.getScenes()){
              scene->cleanupEntities();
          }
        }
        resourceManager.cleanup(device);
        defaultResources.cleanupDefault(); //Destroys the default recourses
        uiManager.shutDownImGui(imguiPool);
        renderer.cleanup();
    }
}
