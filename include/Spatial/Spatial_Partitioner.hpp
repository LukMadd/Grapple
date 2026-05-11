#ifndef _SPATIAL_PARTITIONER
#define _SPATIAL_PARTITIONER


#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "ECS/Components.hpp"
#include "Context/EngineContext.hpp"

constexpr int CELL_SIZE = 10;

namespace EnginePartitioning{
    struct Cell{
        std::unordered_set<Entity> entities;
    };

    class Spatial_Partitioner{
        private:
            std::unordered_map<uint64_t, Cell> grid;

            void removeEntity(Entity entity);
        
        public:
            void init(EngineContext* context){current_context = context;};

            void addEntity(Entity e);

            void registerEntity(Entity entity, std::vector<uint64_t> &cellKeys);

            void updateEntityCells(Entity entity);

            std::vector<uint64_t> getCellKeys(Entity entity, AABB* world = nullptr);

            std::vector<Cell*> getCells(Entity entity);

            std::vector<Cell*> getCellsFromAABB(AABB& world);

            void reRegisterEntity(Entity entity, std::vector<uint64_t> &cellKeys);

            void reset() {grid.clear();}

            private:
                EngineContext* current_context;
    };
}

#endif
