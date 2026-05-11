#include "Scene/Scene.hpp"
#include "Debug/DebugRenderer.hpp"
#include "ECS/Components.hpp"
#include "ECS/ECS.hpp"
#include "ECS/EntityFunctions.hpp"
#include "EngineUtility.hpp"
#include "Misc/Material.hpp"
#include "Misc/UUID.hpp"
#include "Scene/Player.hpp"
#include "EngineGlobals.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace EngineScene{
    Scene::Scene(const std::string &name, int index) : name(name), index(index){};

    void Scene::initBaseScene(EngineResource::ResourceManager &resourceManager, EnginePartitioning::Spatial_Partitioner* spatial, 
                              ECS* ecs){
        cameraManager.checkIfCamerasEmpty();
       
        Entity floor = ecs->createEntity("Cube.obj", "", "Base_Scene_Floor", "");
        ecs->addComponents<RenderableComponent, BoundingBoxComponent, SceneNodeComponent, SpatialPartitioningComponent, PhysicsComponent>(floor);
        auto* floor_node = ecs->getComponent<SceneNodeComponent>(floor);
        floor_node->node = floor;
        auto* floor_tranform = ecs->getComponent<TransformComponent>(floor);
        floor_tranform->position = glm::vec3(0,0, 0);
        floor_tranform->scale = glm::vec3(10.0f);
        floor_tranform->scale.y = 0.5f;
        auto* floor_material = ecs->getComponent<MaterialComponent>(floor);
        floor_material->material->setBaseColor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto* floor_physics = ecs->getComponent<PhysicsComponent>(floor);
        floor_physics->isStatic = true;
        EntityFunctions::scale(floor_tranform->scale, floor, ecs, spatial);

        Entity cube = ecs->createEntity("Cube.obj", "", "Base_Scene_Cube", "");
        ecs->addComponents<RenderableComponent, BoundingBoxComponent, SceneNodeComponent, SpatialPartitioningComponent, PhysicsComponent>(cube);
        auto* cube_node = ecs->getComponent<SceneNodeComponent>(cube);
        cube_node->node = cube;
        auto* cube_tranform = ecs->getComponent<TransformComponent>(cube);
        cube_tranform->position = glm::vec3(0,10, 0);
        auto* cube_material = ecs->getComponent<MaterialComponent>(cube);
        cube_material->material->setBaseColor(glm::vec4(0.2f, 0.8f, 0.2f, 0.3f));
        auto* cube_physics = ecs->getComponent<PhysicsComponent>(cube);
        cube_physics->isStatic = false;
    }

    void Scene::addDefaultEntity(ECS* ecs){
        Entity object = ecs->getAvailableEntityID();

        ecs->addComponents<MeshComponent, RenderableComponent, TransformComponent, BoundingBoxComponent, SceneNodeComponent, 
                           SpatialPartitioningComponent, PhysicsComponent, MaterialComponent, MetadataComponent>(object);
        auto object_node = ecs->getComponent<SceneNodeComponent>(object);
        object_node->node = object;
        auto object_mesh = ecs->getComponent<MeshComponent>(object);
        object_mesh->mesh->meshPath = "sphere.obj";
        auto object_transform = ecs->getComponent<TransformComponent>(object);
        object_transform->position = glm::vec3(0, 10, 0);
        object_transform->scale = glm::vec3(0.1f);
        auto object_material = ecs->getComponent<MaterialComponent>(object);
        if(!object_material->material){
            object_material->material = std::make_shared<Material>();
        }
        object_material->material->setBaseColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));
        auto object_metadata = ecs->getComponent<MetadataComponent>(object);
        object_metadata->name = "Default_Object";
        object_metadata->uuid = generateUUID();
        auto object_physics = ecs->getComponent<PhysicsComponent>(object);
        object_physics->isStatic = false;

        simulationInitQueue.push_back(object);
        renderInitQueue.push_back(object);
    }

    void Scene::removeEntity(Entity e, ECS* ecs){
        auto simQueueIt = std::find(simulationInitQueue.begin(), simulationInitQueue.end(), e);
        if(simQueueIt != simulationInitQueue.end()){
            simulationInitQueue.erase(simQueueIt);
        }

        auto renderQueueIt = std::find(renderInitQueue.begin(), renderInitQueue.end(), e);
        if(renderQueueIt != renderInitQueue.end()){
          renderInitQueue.erase(renderQueueIt);
        }
        
        ecs->removeEntity(e);
    }

    void Scene::drawBoundingBoxes(DebugRenderer *debugRenderer, ECS* ecs){
        ecs->foreach<RenderableComponent, BoundingBoxComponent, TransformComponent>([&debugRenderer](RenderableComponent* r, BoundingBoxComponent* b, TransformComponent* t){
          if(b->localBoundingBox.isInitialized){
            AABB worldBoundingBox = EngineUtility::getWorldAABB(*t, b->localBoundingBox);
            debugRenderer->drawBoundingBox(worldBoundingBox);
          }
        });
    }

    std::vector<std::shared_ptr<Texture>> Scene::getSceneTextures(ECS* ecs){
        std::vector<std::shared_ptr<Texture>> sceneTextures;

        ecs->foreach<MaterialComponent>([&sceneTextures](MaterialComponent* m){
            for(auto& texture : m->material->getTextures()){
                if(texture->isValid()){
                    sceneTextures.push_back(texture);
                }
            }
        });

        return sceneTextures;
    }

    void Scene::cleanupEntities(){
        componentStorage.entities.clear();
    }

    std::unique_ptr<Scene> Scene::createScene(int id, const std::string &name){
        std::unique_ptr<Scene> scene = std::make_unique<Scene>(name, id);
        return scene;
    }

     void Scene::update(ECS* ecs){
        ecs->foreach<SceneNodeComponent>([this, &ecs](SceneNodeComponent* n){
            if(n->parent == nullEntity){
                sceneNode.update(n->node, glm::mat4(1.0f), ecs);
            }
        });
     }
}
