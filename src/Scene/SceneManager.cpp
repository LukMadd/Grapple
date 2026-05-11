#include "Scene/SceneManager.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

#include "Camera/Camera.hpp"

#include "Core/ResourceManager.hpp"
#include "Debug/Debugger.hpp"
#include "ECS/Components.hpp"
#include "ECS/EntityFunctions.hpp"
#include "nlohmann/json.hpp"
#include "Spatial/Spatial_Partitioner.hpp"

#include "Serialization/Serialization.hpp"

using namespace nlohmann;

constexpr const char* SUB_SYSTEM = "Scene_Manager";


namespace EngineScene{
    SceneManager::SceneManager() : currentID(0), currentSceneIndex(0), resourceManager(nullptr), ecs(nullptr){};

    void SceneManager::addDefaultScene(){
        auto scene = Scene::createScene(currentID, "Default_Scene");
        ecs->setComponentStorage(&scene->componentStorage);
        scene->initBaseScene(*resourceManager, spatialPartitioner, ecs);
        sceneOrder.push_back(currentID); //Pushes the scenes ID into sceneOrder to be used for current scene tracking
        scene->update(ecs);
        scenes[currentID] = std::move(scene);

        currentID++;

        ecs->setComponentStorage(&scenes[currentSceneIndex]->componentStorage);
    }

    void SceneManager::changeScenes(int index){
        if(index >= sceneOrder.size() || index < 0) return;

        int newSceneID = sceneOrder[index];
        auto &scenePtr = scenes.at(newSceneID);

        currentSceneIndex = index;

        spatialPartitioner->reset();
        for(auto& entity : ecs->view<SpatialPartitioningComponent, BoundingBoxComponent>()){
            auto* spatial = ecs->getComponent<SpatialPartitioningComponent>(entity);
            //Helps prevent collision issues when switching back to a previously visited scene
            spatial->assignedToCell = false;
            spatial->cells.clear();
            spatialPartitioner->updateEntityCells(entity);
        }
        
        ecs->setComponentStorage(&scenePtr->componentStorage);
    }

    void SceneManager::init(EngineResource::ResourceManager &resourceManager, 
                            EnginePartitioning::Spatial_Partitioner *spatialPartitioner,
                            ECS* _ecs){
        this->resourceManager = &resourceManager;
        this->spatialPartitioner = spatialPartitioner;
        ecs = _ecs;

        std::vector<std::filesystem::path> files;
        for(auto& entry : std::filesystem::directory_iterator(GRAPPLE_SCENE_DIR))
            if(std::filesystem::is_regular_file(entry.path()) && entry.path().extension() == ".json")
                files.push_back(entry.path());

        std::sort(files.begin(), files.end());     
        
        if(files.empty()){
            addDefaultScene();
        } else{
            for(auto &file : files){
                deserializeScene(file.string());
            }
        }

        ecs->setComponentStorage(&scenes[0]->componentStorage);

        if(!sceneOrder.empty()) currentSceneIndex = 0; 
    }

    void SceneManager::serializeScene(Scene &scene, uint32_t sceneIndex){
        json sceneData;
        sceneData["name"] = scene.name;
        sceneData["index"] = scene.index;
        sceneData["entities"] = json::array();
        sceneData["cameras"] = json::array();

        //Loops through the scenes children and serializes them
        for(auto& entity : ecs->getAllEntities()){
            sceneData["entities"].push_back(serializeEntityData(entity, ecs));
        }

        for(auto camera : scene.cameraManager.getCameras()){
            sceneData["cameras"].push_back(serialzeCamera(camera));
        }

        sceneData["next_entity"] = ecs->getNextEntity();

        std::filesystem::create_directories((GRAPPLE_SCENE_DIR));
        std::ofstream file(std::string(GRAPPLE_SCENE_DIR) + "/scene" + std::to_string(sceneIndex) + ".json");
        if(!file.is_open()) {
            throw std::runtime_error("Failed to open JSON file");
        }

        file << sceneData.dump(4);
        file.close();
    }

    void SceneManager::deserializeScene(const std::string& filename){
        std::ifstream file(filename);
        if(!file.is_open()) throw std::runtime_error("Failed to open file " + filename + " for deseralization");

        json sceneData;
        file >> sceneData;

        std::unique_ptr<Scene> scene = Scene::createScene(sceneData["index"], sceneData["name"]);
        ecs->setComponentStorage(&scene->componentStorage);
        ecs->getNextEntity() = sceneData["next_entity"];

        for(auto &jsonObj : sceneData["entities"]){
            Entity e = deserializeEntity(jsonObj, ecs);
            EntityFunctions::initRenderResources(e, *resourceManager, ecs);
            EntityFunctions::initSimulationResources(e, spatialPartitioner, ecs);
            if(ecs->hasComponent<TransformComponent>(e)){
                auto* transform = ecs->getComponent<TransformComponent>(e);
                EntityFunctions::move(transform->position, e, ecs, spatialPartitioner);
                EntityFunctions::rotate(glm::quat(transform->rotation[0], transform->rotation[1], transform->rotation[2], transform->rotation[3]), e, ecs, spatialPartitioner);
                EntityFunctions::scale(transform->scale, e, ecs, spatialPartitioner);
            }
            
        }

        for(auto &jsonCam : sceneData["cameras"]){
            EngineCamera::Camera *camera = deserializeCamera(jsonCam, ecs);
            scene->cameraManager.pushCamera(std::shared_ptr<EngineCamera::Camera>(camera));
        }

        sceneOrder.push_back(scene->index);

        scenes[scene->index] = (std::move(scene));

        currentID++;
    }

    void SceneManager::deleteScene(Scene *scene){
        if(getScenes().size() == 1){
            DEBUGGER_LOG(WARNING, "Cannot delete a scene when only one scene is present", SUB_SYSTEM);
            return;
        }
        int sceneID = scene->index;

        scenes.erase(sceneID);

        auto it = std::find(sceneOrder.begin(), sceneOrder.end(), sceneID);
        if(it != sceneOrder.end()) {
            size_t removedIndex = std::distance(sceneOrder.begin(), it);
            sceneOrder.erase(it);

            if(currentSceneIndex > removedIndex){
                currentSceneIndex--;
            } else if(currentSceneIndex == removedIndex){
                currentSceneIndex = std::min(currentSceneIndex, (int)sceneOrder.size() - 1);
            }
        }

        std::string filename = std::string(GRAPPLE_SCENE_DIR) + "scene" + std::to_string(sceneID) + ".json";
        if(std::filesystem::exists(filename)){
            std::filesystem::remove(filename);
        }

        ecs->setComponentStorage(&scenes[currentSceneIndex]->componentStorage);
    }

    void SceneManager::saveScenes(){
        for(auto& [id, scene] : scenes){
            serializeScene(*scene, id);
        }
    }

     std::vector<Scene*> SceneManager::getScenes(){
        std::vector<Scene*> scenesVector;

        for(auto &scene : scenes){
            scenesVector.push_back(scene.second.get());
        }

        return scenesVector;
    }

    Scene* SceneManager::getCurrentScene(){
        if(sceneOrder.empty()){
            DEBUGGER_LOG(ERROR, "Scene order is empty! Returning nullptr", SUB_SYSTEM);
            return nullptr;
        }
        int id = sceneOrder[currentSceneIndex];
        auto it = scenes.find(id);
        if(it == scenes.end()){
            DEBUGGER_LOG(ERROR, "Invliad currentSceneIndex! Returning nullptr", SUB_SYSTEM);
            return nullptr;
        }

        return scenes.at(sceneOrder[currentSceneIndex]).get();
    }

    Scene* SceneManager::getScene(int id){
        auto it = scenes.find(id);
        if(it != scenes.end()) return it->second.get();
        return nullptr;
    }
}
