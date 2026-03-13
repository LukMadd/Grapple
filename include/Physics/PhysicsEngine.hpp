
#include <cstdint>
#include "ECS/Components.hpp"

#include "Spatial/Spatial_Partitioner.hpp"

namespace EnginePhysics{
    class PhysicsEngine{
        public:
            void init(EnginePartitioning::Spatial_Partitioner *spatialPartitioner,
                      EngineContext** engineContext);

            void tick(float deltaTime);

            float getTickRate() {return tickRate;}

            uint64_t getTickCount() {return tickCount;}

            std::vector<Entity>& getChangedBoundingBoxes(){return changedBoundingBoxes;}

        private:
            uint64_t currentTick;

            const float tickRate = 16.66666667 / 1000.0f; //60 updates a second

            const glm::vec3 gravity = glm::vec3(0.0, -9.81, 0.0); //Goes down 1/10 unit a tick

            uint64_t tickCount = 0;

            EnginePartitioning::Spatial_Partitioner *spatialPartitioner;

            EngineContext** context = nullptr;

            std::vector<Entity> changedBoundingBoxes;
    };
}
