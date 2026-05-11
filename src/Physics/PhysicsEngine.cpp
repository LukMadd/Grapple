#include "Physics/PhysicsEngine.hpp"
#include "ECS/Components.hpp"
#include "Context/EngineContext.hpp"
#include "Physics/Swept.hpp"
#include "EngineUtility.hpp"
#include <cassert>
#include <unordered_map>
#include <unordered_set>

struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const noexcept {
        auto a = std::min(p.first, p.second);
        auto b = std::max(p.first, p.second);

        size_t h1 = std::hash<Entity>{}(a);
        size_t h2 = std::hash<Entity>{}(b);
        return h1 ^ (h2 << 1);}
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

        std::unordered_set<std::pair<Entity, Entity>, PairHash> checkedPairs;
        std::unordered_map<Entity, AABB> worldBoxes;
        std::unordered_map<Entity, glm::vec3> finalPositions;

        float epsilon = 0.0000001f;

        for(auto e : (*context)->ecs.view<TransformComponent, BoundingBoxComponent, PhysicsComponent, SpatialPartitioningComponent>()){
          auto* t = (*context)->ecs.getComponent<TransformComponent>(e);
          auto* b = (*context)->ecs.getComponent<BoundingBoxComponent>(e);

          worldBoxes[e] = EngineUtility::getWorldAABB(*t, b->localBoundingBox);
        }

        for(auto entity : (*context)->ecs.view<TransformComponent, BoundingBoxComponent, PhysicsComponent, SpatialPartitioningComponent>()){
          auto* transform = (*context)->ecs.getComponent<TransformComponent>(entity);
          auto* boundingBox = (*context)->ecs.getComponent<BoundingBoxComponent>(entity);
          auto* physics = (*context)->ecs.getComponent<PhysicsComponent>(entity);

          if (physics->isStatic) continue;

          glm::vec3 velocity = physics->velocity;

          if(!physics->isStatic){
            velocity += (gravity * deltaTime);
          }

          glm::vec3 startingPosition = transform->position;

          glm::vec3 position = startingPosition;

          float remainingTime = 1.0f;

          //Multiple collision passes to handle colliding with multiple objects in one frame
          for(int i = 0; i < 4; i++){
            float bestT = 1.0f;
            glm::vec3 bestNormal(0.0f);
            bool hitSomething = false;

            glm::vec3 step = velocity * deltaTime;
            step *= remainingTime; 

            AABB testBox = EngineUtility::getWorldAABBWithoutComponent(position, transform->rotation, transform->scale, boundingBox->localBoundingBox);

            for(auto cell : spatialPartitioner->getCells(entity)){
              for(auto other : cell->entities){
                if(other == entity) continue;

                if((*context)->ecs.hasComponents<TransformComponent, BoundingBoxComponent>(other) == false) continue;

                AABB otherBox = worldBoxes[other];

                SweepResult sweep = sweepAABB(testBox, step, otherBox);

                if(!sweep.hit) continue;

                if(sweep.hit && sweep.t < bestT){
                  bestT = sweep.t;
                  bestNormal = sweep.normal;
                  hitSomething = true;
                }
              }
            }
            //move to impact or full step
            position += step * bestT;

            if(!hitSomething){
              break;
            }

            position += bestNormal * epsilon; 

            if(bestNormal.x != 0.0f) velocity.x = 0.0f;
            if(bestNormal.y != 0.0f) velocity.y = 0.0f;
            if(bestNormal.z != 0.0f) velocity.z = 0.0f;

            remainingTime *= (1.0f - bestT);
          }

          physics->velocity = velocity;
          finalPositions[entity] = position;
        }

        for(auto &[entity, position] : finalPositions){
          EntityFunctions::move(position, entity, &(*context)->ecs, spatialPartitioner);
        }
    }
}
