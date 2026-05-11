#ifndef _ENTITY_FUNCTIONS_HPP
#define _ENTITY_FUNCTIONS_HPP

#include "Components.hpp"
#include "Core/ResourceManager.hpp"

namespace EnginePartitioning{
    class Spatial_Partitioner;
};

class ECS;

namespace EntityFunctions{
  Entity initEntity(std::string meshPath = "", std::string texturePath = "", std::string name = "", std::string type = "", ECS* ecs = nullptr);

  void initRenderResources(Entity e, EngineResource::ResourceManager &resourceManager, ECS* ecs);

  void initSimulationResources(Entity e, EnginePartitioning::Spatial_Partitioner *spatialPartitioner, ECS* ecs);

  void draw(Entity e, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, ECS* ecs);

  void createBoundingBox(Entity e, ECS* ecs);

  void move(glm::vec3 position, Entity e, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial);

  void rotate(glm::quat rotation, Entity e, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial);

  void scale(glm::vec3 scale, Entity e, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial);
}
#endif
