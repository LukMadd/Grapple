#include "ECS/Components.hpp"
#include "ECS/EntityFunctions.hpp"
#include "Misc/Material.hpp"
#include "Serialization/Serialization.hpp"
#include <iostream>


Entity deserializeEntity(const nlohmann::json& jsonData, ECS* ecs){
    Entity entity = jsonData["id"];
    ecs->addEntity(entity);

    json components = jsonData["entity"]["components"];
    for(const auto& component : components){
        if(component.contains("component_type")){

            if(component["component_type"] == "renderable"){
                auto* renderable = ecs->addComponent<RenderableComponent>(entity);
                renderable->uniformIndex = component["uniform_index"];
            }

            else if(component["component_type"] == "mesh"){
                auto* mesh = ecs->addComponent<MeshComponent>(entity);
                mesh->mesh->meshPath = component["mesh_path"];
            }

            else if(component["component_type"] == "material"){
                auto* material = ecs->addComponent<MaterialComponent>(entity);
                material->material = std::make_shared<Material>();
                Shader shader;
                shader.vertShader = component["shader"]["vert"];
                shader.fragShader = component["shader"]["frag"];
                material->material->setAlbedo(component["albedo"]);
                material->material->setBaseColor(glm::vec4(
                    component["base_color"][0][0],
                    component["base_color"][1][0],
                    component["base_color"][2][0],
                    component["base_color"][3][0]
                ));
                material->material->setMetallic(component["metallic"]);
                material->material->setRoughness(component["roughness"]);
                material->material->setShader(shader);
                if(!component["textures"].empty()){
                    material->hasTexture = true;
                    for(const auto& texture : component["textures"]){
                        material->material->addPendingTexture(texture);
                    }
                }
            }

            else if(component["component_type"] == "physics"){
                auto* physics = ecs->addComponent<PhysicsComponent>(entity);
                physics->isStatic = component["is_static"];
                physics->velocity = glm::vec3(component["velocity"][0], component["velocity"][1], component["velocity"][2]);
            }

            else if(component["component_type"] == "spatial_partitioning"){
                auto* spatial_partioner = ecs->addComponent<SpatialPartitioningComponent>(entity);
            }

            else if(component["component_type"] == "scene_node"){
                auto* scene_node = ecs->addComponent<SceneNodeComponent>(entity);
                scene_node->node = component["node"];
                scene_node->parent = component["parent"];
            }

            else if(component["component_type"] == "metadata"){
                auto* metadata = ecs->addComponent<MetadataComponent>(entity);
                metadata->name = component["name"];
                metadata->type = component["type"];
                metadata->uuid = component["uuid"];
            }
            
            else if(component["component_type"] == "transform"){
                auto* transform = ecs->addComponent<TransformComponent>(entity);
                json json_transform = component["transform"];
                transform->position = glm::vec3(json_transform["position"][0], json_transform["position"][1], json_transform["position"][2]); //x,y,z
                transform->rotation = glm::quat(json_transform["rotation"][3], json_transform["rotation"][0], json_transform["rotation"][1], json_transform["rotation"][2]); //w,x,y,z
                transform->scale = glm::vec3(json_transform["scale"][0], json_transform["scale"][1], json_transform["scale"][2]);
            }

            else if(component["component_type"] == "bounding_box"){
                auto* bounding_box = ecs->addComponent<BoundingBoxComponent>(entity);
                if(ecs->hasComponent<MeshComponent>(entity)){
                  EntityFunctions::createBoundingBox(entity, ecs);
                }
            }

        }
    }

    return entity;
}

SceneNodeComponent* deserializeNode(Scene &scene, const json& jsonNode, ECS* ecs){
    SceneNodeComponent *node = new SceneNodeComponent();
    node->node = nullEntity;

    TransformComponent transformComponent;

    if(jsonNode.contains("transform")){
        const auto &transform = jsonNode["transform"];
        if(transform.contains("position") && transform["position"].size() >= 3)
            transformComponent.position = glm::vec3(transform["position"][0], transform["position"][1], transform["position"][2]);
        if(transform.contains("rotation") && transform["rotation"].size() >= 4)
            transformComponent.rotation = glm::quat(transform["rotation"][3], transform["rotation"][0], transform["rotation"][1], transform["rotation"][2]); // w,x,y,z
        if(transform.contains("scale") && transform["scale"].size() >= 3)
            transformComponent.scale = glm::vec3(transform["scale"][0], transform["scale"][1], transform["scale"][2]);
    }

    if(jsonNode.contains("node")){
        const json &jsonData = jsonNode["node"];
        deserializeEntity(jsonData, ecs);
    }

    return node;
}

Camera* deserializeCamera(const nlohmann::json& jsonData, ECS* ecs){
    EngineCamera::Camera* camera = new EngineCamera::Camera();

    camera->is_initialized = jsonData["is_initialized"];

    camera->name = jsonData["name"];
    
    camera->getFov() = jsonData["fov"];
    
    camera->getFront().x = jsonData["front"][0];
    camera->getFront().y = jsonData["front"][1];
    camera->getFront().z = jsonData["front"][2];

    camera->getUp().x = jsonData["up"][0];
    camera->getUp().y = jsonData["up"][1];
    camera->getUp().z = jsonData["up"][2];

    if(jsonData["target"]["target_entity"] != nullEntity){
        Entity entity = jsonData["target"]["target_entity"];
        if(ecs->hasComponent<TransformComponent>(entity)){
            auto* transform = ecs->getComponent<TransformComponent>(entity);
            glm::vec3 offset = {jsonData["target"]["target_offset"][0], jsonData["target"]["target_offset"][1], 
                             jsonData["target"]["target_offset"][2]};

            camera->setTarget(entity, transform);
            camera->setTargetOffset({offset});
        }
    }

    camera->position.x = jsonData["position"][0];
    camera->position.y = jsonData["position"][1];
    camera->position.z = jsonData["position"][2];

    camera->getYaw() = jsonData["rotation"]["yaw"];
    camera->getPitch() = jsonData["rotation"]["pitch"];

    camera->getSensitivity() = jsonData["sensitivity"];

    camera->getSpeed() = jsonData["speed"];

    return camera;
}
