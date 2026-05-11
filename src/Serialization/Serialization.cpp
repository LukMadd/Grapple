#include "Serialization/Serialization.hpp"
#include "Camera/Camera.hpp"
#include "ECS/Components.hpp"

json serializeEntityData(const Entity e, ECS* ecs){
    auto* sceneNode = ecs->getComponent<SceneNodeComponent>(e);

    json entityData;
    json jsonData;
    json components = json::array();

    if(ecs->hasComponent<RenderableComponent>(e)){
        auto* renderable = ecs->getComponent<RenderableComponent>(e);
        json json_renderable = json::object();
        json_renderable["component_type"] = "renderable";
        json_renderable["uniform_index"] = renderable->uniformIndex;
        components.push_back(json_renderable);
    }

    if(ecs->hasComponent<TransformComponent>(e)){
        auto* transform = ecs->getComponent<TransformComponent>(e);
        json json_transform = json::object();
        json_transform["component_type"] = "transform";
        json_transform["transform"] = {
            {"position",{transform->position.x,  transform->position.y, transform->position.z}},
            {"rotation",{transform->rotation.w, transform->rotation.x, transform->rotation.y, transform->rotation.z}}, // w,x,y,z
            {"scale", {transform->scale.x, transform->scale.y, transform->scale.z}}
        };
        components.push_back(json_transform);
    }

    if(ecs->hasComponent<MeshComponent>(e)){
        auto* mesh = ecs->getComponent<MeshComponent>(e);
        json json_mesh = json::object();
        json_mesh["component_type"] = "mesh";
        json_mesh["mesh_path"] = mesh->mesh->meshPath;
        components.push_back(json_mesh);
    }

    if(ecs->hasComponent<MaterialComponent>(e)){
        auto* material = ecs->getComponent<MaterialComponent>(e);
        json json_material = json::object();
        json_material["component_type"] = "material";
        json_material["base_color"] = {
            {material->material->getBaseColor().x},
            {material->material->getBaseColor().y},
            {material->material->getBaseColor().z},
            {material->material->getBaseColor().w},
        };
        json_material["roughness"] = material->material->getRoughness();
        json_material["metallic"] = material->material->getMetallic();
        json_material["albedo"] = material->material->getAlbedo();

        json_material["textures"] = json::array();
        for(const auto &texture : material->material->getTextures()){
            json_material["textures"].push_back(texture->texturePath);
        }
        json_material["shader"] = {
            {"vert", material->material->getShader().vertShader},
            {"frag", material->material->getShader().fragShader}
        };

        components.push_back(json_material);
    }

    if(ecs->hasComponent<PhysicsComponent>(e)){
        auto* physics = ecs->getComponent<PhysicsComponent>(e);
        json json_physics = json::object();
        json_physics["component_type"] = "physics";
        json_physics["is_static"] = physics->isStatic; 
        json_physics["velocity"] = {
            physics->velocity.x,
            physics->velocity.y,
            physics->velocity.z
        };
        components.push_back(json_physics);
    }
    
    if(ecs->hasComponent<SpatialPartitioningComponent>(e)){
        json json_partitioning = json::object();
        json_partitioning["component_type"] = "spatial_partitioning";
        components.push_back(json_partitioning);
    }

    if(ecs->hasComponent<SceneNodeComponent>(e)){
        auto* sceneNode = ecs->getComponent<SceneNodeComponent>(e);
        json json_scene_node = json::object();
        json_scene_node["component_type"] = "scene_node";
        json_scene_node["node"] = sceneNode->node;
        json_scene_node["parent"] = sceneNode->parent;
        components.push_back(json_scene_node);
    }

    if(ecs->hasComponent<MetadataComponent>(e)){
        auto* metadata = ecs->getComponent<MetadataComponent>(e);
        json json_metadata = json::object();
        json_metadata["component_type"] = "metadata";
        if(!metadata->name.empty()){
            json_metadata["name"] = metadata->name;
        } else{
            json_metadata["name"] = "Generic";
        }
        json_metadata["uuid"] = metadata->uuid;
        json_metadata["type"] = metadata->type;
        components.push_back(json_metadata);
    }

    if(ecs->hasComponent<BoundingBoxComponent>(e)){
        auto* boundingBox = ecs->getComponent<BoundingBoxComponent>(e);
        json json_bounding_box = json::object();
        json_bounding_box["component_type"] = "bounding_box";
        components.push_back(json_bounding_box);
    }


    jsonData["components"] = components;
    

    entityData["entity"] = jsonData;
    entityData["id"] = e;

    return entityData;
}


json serialzeCamera(std::shared_ptr<EngineCamera::Camera> camera){
    if(!camera->is_initialized){
        return nullptr;
    }

    json cameraData = json::object();

    cameraData["is_initialized"] = camera->is_initialized;

    cameraData["name"] = camera->name;

    cameraData["position"] = {
        camera->position.x, camera->position.y, camera->position.z
    };

    glm::vec3 target_offset = camera->getTargetOffset();
    json target_info = json::object();
    target_info["target_entity"] = camera->getTarget();
    if(target_offset.x != FLT_MAX){
        target_info["target_offset"] = {
            target_offset.x, target_offset.y, target_offset.z
        };
    } else {
        target_info["target_offset"] = {
            0.0f, 2.0f, -10.0f    
        };
    }

    cameraData["target"] = target_info;

    glm::vec3 front = camera->getFront();
    glm::vec3 up = camera->getUp();

    cameraData["front"] = {
        front.x, front.y, front.z
    };

    cameraData["up"] = {
        up.x, up.y, up.z,
    };

    cameraData["fov"] = camera->getFov();
    cameraData["speed"] = camera->getSpeed();
    
    cameraData["rotation"] = {
        {"yaw", camera->getYaw()},
        {"pitch", camera->getPitch()}
    };

    cameraData["sensitivity"] = camera->getSensitivity();

    return cameraData;
}
