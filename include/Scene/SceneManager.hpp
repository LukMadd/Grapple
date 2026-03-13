#ifndef _SCENE_MANAGER_HPP
#define _SCENE_MANAGER_HPP

#include "Scene.hpp"
#include "Core/ResourceManager.hpp"
#include <cstdint>
#include <memory>

//Forward declaration to prevent circular reference errors from EngineContext.hpp including Spatial_Partitioner.hpp
//and Spatial_Partitioner.hpp including SceneManager.hpp
namespace EnginePartitioning{
  class Spatial_Partitioner;
}

//Also here to prevent circular references
class EngineContext;

namespace EngineScene{
    class SceneManager{
        public:
            SceneManager();

            void init(EngineResource::ResourceManager &resourceManager, 
                     EnginePartitioning::Spatial_Partitioner *spatialPartitioner,
                     ECS* _ecs);

            void addDefaultScene();
            void changeScenes(int index);

            std::vector<Scene*> getScenes();
            Scene* getCurrentScene();
            int getCurrentSceneIndex(){return currentSceneIndex;}

            void serializeScene(Scene &scene, uint32_t sceneIndex);
            void deserializeScene(const std::string& filename);
            
            void deleteScene(Scene *scene);

            Scene* getScene(int id);

            void saveScenes();
            void cleanup();

            SceneManager& operator=(const SceneManager& other){
              if(this == &other){
                return *this;
              }

              resourceManager = other.resourceManager;
              spatialPartitioner = other.spatialPartitioner;
              sceneOrder = other.sceneOrder;
              currentSceneIndex = other.currentSceneIndex;
              currentID = other.currentID;
              //Deep copies scenes, some scene managers may have to be revisited at some point
              //so moving it is not an option
              for(auto& [id, scene] : other.scenes){
                this->scenes[id] = std::make_unique<Scene>(*scene);
              }

              return *this;
            }

        private:
            EngineResource::ResourceManager *resourceManager;

            EnginePartitioning::Spatial_Partitioner *spatialPartitioner;

            std::unordered_map<int, std::unique_ptr<Scene>> scenes;

            std::vector<int> sceneOrder;

            int currentSceneIndex = 0;

            int currentID = 0;

            ECS* ecs;
    };
}

#endif
