#ifndef ENGINE_CONTEXT_HPP
#define ENGINE_CONTEXT_HPP

#include "Scene/SceneManager.hpp"
#include "ECS/ECS.hpp"
#include <stdexcept>

using namespace EngineScene;

struct EngineContext{
  EngineContext() : sceneManager(){};

  void copyState(const EngineContext& otherContext){
    sceneManager = otherContext.sceneManager;
    assert(sceneManager.getCurrentScene());
    ecs.setComponentStorage(&sceneManager.getCurrentScene()->componentStorage);
    if(ecs.getComponentStorage() == nullptr){
      throw std::runtime_error("COMPONENT STORAGE NOT SET\n");
    }
  }

  SceneManager sceneManager;
  ECS ecs;

  std::string name;
  bool isSetup = false;
};

#endif
