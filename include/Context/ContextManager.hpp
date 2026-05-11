#ifndef _CONTEXT_MANAGER_HPP
#define _CONTEXT_MANAGER_HPP

#include "Context/EngineContext.hpp"
#include "Spatial/Spatial_Partitioner.hpp"
#include "Input/InputHandler.hpp"
#include "Renderer/Renderer.hpp"
#include "UI/UIManager.hpp"
#include "Core/ResourceManager.hpp"

class ContextManager{
  public:
    void init(EnginePartitioning::Spatial_Partitioner* _spatialPartitioner, EngineInput::InputHandler* _inputHandler,
              EngineRenderer::Renderer* _renderer, EngineUI::UIManager* _uiManager, 
              EngineResource::ResourceManager* _resourceManager){
      spatialPartitioner = _spatialPartitioner;
      inputHandler = _inputHandler;
      renderer = _renderer;
      uiManager = _uiManager;
      resourceManager = _resourceManager;
    }


    void switchContexts(EngineContext& otherContext){
      currentContext = &otherContext;

      spatialPartitioner->reset();
      spatialPartitioner->init(&otherContext);
      if(otherContext.isSetup == false){
        otherContext.sceneManager.init(*resourceManager, spatialPartitioner, &otherContext.ecs);

        for(auto &scene : otherContext.sceneManager.getScenes()){
          for(auto& e : otherContext.ecs.view<RenderableComponent>()){
            EntityFunctions::initRenderResources(e, *resourceManager, &otherContext.ecs);
            EntityFunctions::initSimulationResources(e, spatialPartitioner, &otherContext.ecs);
          }

          renderer->createSceneDescriptorSets(otherContext.sceneManager.getCurrentScene(), &otherContext.ecs);
          otherContext.sceneManager.getCurrentScene()->areEntitiesInitialized = true;          

        }

        renderer->createSceneDescriptorSets(otherContext.sceneManager.getCurrentScene(), &otherContext.ecs);

        otherContext.isSetup = true;
      }

      if(std::binary_search(contexts.begin(), contexts.end(), &otherContext) == false){
        contexts.push_back(&otherContext);
      }

      uiManager->setCameraManager(&otherContext.sceneManager.getCurrentScene()->cameraManager);

      hasContextChanged = true;
 
    }

    void cleanup(){
      for(auto& context : contexts){
          if(!context->isSetup) continue;
          for(auto &scene :  context->sceneManager.getScenes()){
              scene->cleanupEntities();
          }
        }
    }

    void serialize(){
      primaryContext->sceneManager.saveScenes();
    }

    EngineContext* currentContext;
    EngineContext* primaryContext;

    bool hasContextChanged = false;

  private:
    std::vector<EngineContext*> contexts{};

//External variables that must be updated on context change
  EnginePartitioning::Spatial_Partitioner* spatialPartitioner;
  EngineInput::InputHandler* inputHandler;
  EngineRenderer::Renderer* renderer;
  EngineUI::UIManager* uiManager;
  EngineResource::ResourceManager* resourceManager;
};

#endif
