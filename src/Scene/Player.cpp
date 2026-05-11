#include "Scene/Player.hpp"
#include "ECS/Components.hpp"
#include "ECS/EntityFunctions.hpp"
#include "EngineGlobals.hpp"
#include "Debug/Debugger.hpp"

using namespace EnginePartitioning;

Player3D::Player3D(std::shared_ptr<Camera> camera, ECS* ecs){
  if(!camera){
    DEBUGGER_LOG(WARNING, "No camera given to Player3D initializer", "Engine");
  }
  if(!ecs){
    DEBUGGER_LOG(WARNING, "No ECS given to Player3D initializer", "Engine");
  }

  Entity player = ecs->createEntity("Cube.obj", "", "Player", "");
  ecs->addComponents<RenderableComponent, BoundingBoxComponent, SceneNodeComponent, SpatialPartitioningComponent, PhysicsComponent>(player);

  auto* player_node = ecs->getComponent<SceneNodeComponent>(player);
  player_node->node = player;
  auto* player_transform = ecs->getComponent<TransformComponent>(player);
  player_transform->position = glm::vec3(0.0f, 15.0f, 0.0f);
  player_transform->scale = glm::vec3(0.5f, 1.10f, 0.5f);
  auto* player_material = ecs->getComponent<MaterialComponent>(player);
  player_material->material->setBaseColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
  auto* floor_physics = ecs->getComponent<PhysicsComponent>(player);
  floor_physics->isStatic = false;

  entity = player;
  transform = player_transform;

  camera->name = "Player_Camera";
  camera->setTarget(entity, transform);
  camera->setTargetOffset(glm::vec3(0.0f, 7.5f, -30.0f));
}

glm::vec3 Player3D::getPosition(){
  if(!transform){
    return glm::vec3(FLT_MIN);
  }
  return transform->position;
}

void Player3D::move(glm::vec3 position, ECS* ecs, Spatial_Partitioner* spatial) const{
  EntityFunctions::move(position, entity, ecs, spatial); 
}
