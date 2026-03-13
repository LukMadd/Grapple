#include "Physics/PhysicsEngine.hpp"
#include "ECS/Components.hpp"
#include "EngineContext.hpp"
#include "Physics/Collision.hpp"
#include <cassert>
#include <unordered_set>

struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const noexcept {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

using namespace EnginePartitioning;

namespace EnginePhysics{
    void PhysicsEngine::init(EnginePartitioning::Spatial_Partitioner *spatialPartitioner,
                             EngineContext** engineContext){
        this->spatialPartitioner = spatialPartitioner;
        this->context = engineContext;
    }
  
    void PhysicsEngine::tick(float deltaTime){
        tickCount++;

        std::unordered_set<std::pair<std::string, std::string>, PairHash> checkedPairs;
        for(auto& changedBoundingBox : changedBoundingBoxes){
            spatialPartitioner->updateEntityCells(changedBoundingBox);
        }

        for(auto entity : (*context)->ecs.view<TransformComponent, BoundingBoxComponent, PhysicsComponent, SpatialPartitioningComponent>()){
            auto* transform = (*context)->ecs.getComponent<TransformComponent>(entity);
            auto* boundingBox = (*context)->ecs.getComponent<BoundingBoxComponent>(entity);
            auto* physics = (*context)->ecs.getComponent<PhysicsComponent>(entity);
            auto* spatial = (*context)->ecs.getComponent<SpatialPartitioningComponent>(entity);
            auto* metadata = (*context)->ecs.getComponent<MetadataComponent>(entity);

            if(!physics->isStatic){
                //Applies gravity
                physics->velocity += (gravity * deltaTime) / 10.0f;
                if(physics->velocity.y >= 10.0f){
                    physics->velocity.y = 10.0f;
                }
            }
            glm::vec3 movement = physics->isStatic ? glm::vec3(0.0f) : physics->velocity * deltaTime;
            glm::vec3 newPos = transform->position;

            AABB predictedBox = boundingBox->worldBoundingBox;
            predictedBox.min += movement;
            predictedBox.max += movement;

            glm::vec3 currentMins = {boundingBox->worldBoundingBox.min.x, boundingBox->worldBoundingBox.min.y, boundingBox->worldBoundingBox.min.z};
            glm::vec3 currentMaxes = {boundingBox->worldBoundingBox.max.x, boundingBox->worldBoundingBox.max.y, boundingBox->worldBoundingBox.max.z};

            std::vector<Cell*> cells = spatialPartitioner->getCells(entity);
            for(int i = 0; i < cells.size(); i++){
                for(auto other : cells[i]->entities){

                    if(entity == other) continue;
                                        
                    BoundingBoxComponent otherBoundingBox = *(*context)->ecs.getComponent<BoundingBoxComponent>(other);

                    std::pair<std::string, std::string> dirKey{metadata->uuid, (*context)->ecs.getComponent<MetadataComponent>(other)->uuid};
                    if(!checkedPairs.insert(dirKey).second) continue;

                    float epsilon = 0.0000001f;

                    int collisionDirection = getCollisionDirection(predictedBox, otherBoundingBox.worldBoundingBox, currentMins, currentMaxes);
                    if(collisionDirection != OBJECT_COLLISION_NONE){
                        handleCollision(predictedBox, otherBoundingBox.worldBoundingBox, physics->velocity, 
                                      movement, currentMins, currentMaxes, collisionDirection, epsilon);
                    } else{
                    }

                }
            } 
        newPos += glm::vec3(movement.x, movement.y, movement.z);
        if(newPos != transform->position && !physics->isStatic){
          EntityFunctions::move(newPos, entity, &(*context)->ecs);
        }
        }
    }
}
