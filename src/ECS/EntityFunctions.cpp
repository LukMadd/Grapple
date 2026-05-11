#include "ECS/EntityFunctions.hpp"
#include "ECS/ECS.hpp"
#include "Spatial/Spatial_Partitioner.hpp" 
#include "Debug/Debugger.hpp"
#include "ECS/Components.hpp"
#include "Misc/Material.hpp"
#include "Misc/UUID.hpp"
#include "ECS/ECS.hpp"
#include "EngineGlobals.hpp"

#define SUB_SYSTEM "Entity_Functions"

#include <cstdint>

using namespace EnginePartitioning;

namespace EntityFunctions{
  Entity initEntity(std::string meshPath, std::string texturePath, std::string name, std::string type, ECS* ecs){
    if(!ecs) DEBUGGER_LOG(WARNING, "Invalid ECS", SUB_SYSTEM)  

    uint32_t entityID = ecs->getAvailableEntityID();
    Entity entity = entityID;
    ecs->addEntity(entity);
    ecs->addComponent<TransformComponent>(entity);
    
    auto* metadataComponent = ecs->addComponent<MetadataComponent>(entity);
    metadataComponent->uuid = generateUUID();
    if(!name.empty()){
        metadataComponent->name = name;
    }
    if(!type.empty()){
        metadataComponent->type = type;
    }

    if(!meshPath.empty()){
        MeshComponent* meshComponent = ecs->addComponent<MeshComponent>(entity);
        meshComponent->mesh->meshPath = meshPath;
    }

    MaterialComponent* materialComponent = ecs->addComponent<MaterialComponent>(entity);
    materialComponent->material = std::make_shared<Material>();
    if(!texturePath.empty()){
        materialComponent->material->addPendingTexture(texturePath);
    }

    return entity;
  }

  //ALWAYS INITIALIZE RENDER RESOURCES BEFORE SIMULATION RESOURCES
  void initRenderResources(Entity e, EngineResource::ResourceManager &resourceManager, ECS* ecs){
      auto* entity_mesh = ecs->getComponent<MeshComponent>(e);
      if(!entity_mesh->mesh->meshPath.empty()){
          entity_mesh->mesh = resourceManager.load<Mesh>(entity_mesh->mesh->meshPath);
      }

      auto* entity_material = ecs->getComponent<MaterialComponent>(e);
      if(!entity_material->material->getPendingTextures().empty()){
          entity_material->material->getTextures().clear();
          for(const auto& texturePath : entity_material->material->getPendingTextures()){
              auto texture = resourceManager.load<Texture>(texturePath);
              if(texture){
                  entity_material->material->addTexture(texture);
                  entity_material->hasTexture = true;
              }
          }
      } else{
          for(auto& texture : entity_material->material->getTextures()){
              if(texture->texturePath.empty()){
                  continue;
              }
              if(!texture->textureSampler || !texture->textureImage){
                  texture = resourceManager.load<Texture>(texture->texturePath);
              }
          }
      }
  }

  void initSimulationResources(Entity e, EnginePartitioning::Spatial_Partitioner *spatialPartitioner, ECS* ecs){
    if(ecs->hasComponent<BoundingBoxComponent>(e)){
      auto* boundingBox = ecs->getComponent<BoundingBoxComponent>(e);
      if(!boundingBox->localBoundingBox.isInitialized){
        createBoundingBox(e, ecs);
      }
    }

    if(ecs->hasComponent<SpatialPartitioningComponent>(e)){
      spatialPartitioner->addEntity(e);
    }

  }

  void draw(Entity e, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, ECS* ecs){
      if(!ecs->hasComponent<MeshComponent>(e)){
          DEBUGGER_LOG(WARNING, "Invalid entity for drawing, no mesh component!", "Renderer");
          return;
      }

      auto* entity_transform = ecs->getComponent<TransformComponent>(e);
      auto* entity_mesh = ecs->getComponent<MeshComponent>(e);
      VkDeviceSize offsets[] = {0};   

      if(entity_mesh->mesh->vertexBuffer.buffer == VK_NULL_HANDLE){
        return;
      }

      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &entity_mesh->mesh->vertexBuffer.buffer, offsets);
      vkCmdBindIndexBuffer(commandBuffer, entity_mesh->mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

      vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &entity_transform->modelMatrix);

      vkCmdDrawIndexed(commandBuffer, entity_mesh->mesh->indexCount, 1, 0, 0, 0);
  }

  void createBoundingBox(Entity e, ECS* ecs){
      auto* entity_mesh = ecs->getComponent<MeshComponent>(e);
      auto* entity_transform = ecs->getComponent<TransformComponent>(e);
      auto* entity_AABB = ecs->getComponent<BoundingBoxComponent>(e);

      entity_AABB->localBoundingBox.min = glm::vec3(FLT_MAX);
      entity_AABB->localBoundingBox.max = glm::vec3(-FLT_MAX);

      if(entity_mesh->mesh->vertexBuffer.vertices.empty()){
        return;
      }

      for(const auto& v : entity_mesh->mesh->vertexBuffer.vertices){
          entity_AABB->localBoundingBox.min = glm::min(entity_AABB->localBoundingBox.min, v.pos);
          entity_AABB->localBoundingBox.max = glm::max(entity_AABB->localBoundingBox.max, v.pos);
      }

      entity_AABB->localBoundingBox.isInitialized = true;
  }

  void move(glm::vec3 position, Entity e, ECS* ecs, Spatial_Partitioner* spatial){
      auto* transform = ecs->getComponent<TransformComponent>(e);
      auto* boundingBox = ecs->getComponent<BoundingBoxComponent>(e);

      transform->position = position;

      glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position);
      model *= glm::mat4_cast(transform->rotation);
      model = glm::scale(model, transform->scale);

      transform->modelMatrix = model;

      if(!boundingBox->localBoundingBox.isInitialized){
        return; // Returning instead of creating to make the flow of the bounding box data easier to follow
      }
      spatial->updateEntityCells(e);
  }

  void rotate(glm::quat rotation, Entity e, ECS* ecs, Spatial_Partitioner* spatial){
      auto* transform = ecs->getComponent<TransformComponent>(e);
      auto* boundingBox = ecs->getComponent<BoundingBoxComponent>(e);

      transform->rotation = glm::quat(rotation);

      glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position);
      model *= glm::mat4_cast(transform->rotation);
      model = glm::scale(model, transform->scale);

      transform->modelMatrix = model;
      
      if(!boundingBox->localBoundingBox.isInitialized){
        return;
      }
      spatial->updateEntityCells(e);;
  }

  void scale(glm::vec3 scale, Entity e, ECS* ecs, Spatial_Partitioner* spatial){
      auto* transform = ecs->getComponent<TransformComponent>(e);
      auto* boundingBox = ecs->getComponent<BoundingBoxComponent>(e);

      transform->scale = scale;

      glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position);
      model *= glm::mat4_cast(transform->rotation);
      model = glm::scale(model, transform->scale);

      transform->modelMatrix = model;

      if(!boundingBox->localBoundingBox.isInitialized){
        return;
      }
      spatial->updateEntityCells(e);
  }
}
