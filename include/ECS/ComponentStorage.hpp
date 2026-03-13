#ifndef COMPONENT_STORAGE_HPP
#define COMPONENT_STORAGE_HPP

#include "Components.hpp"
#include "Debug/Debugger.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <sys/types.h>
#include <unordered_map>


//Component Accessor 
template<typename T>
struct ComponentAccessor;

struct ComponentStorage{
    std::vector<Entity> entities;

    std::unordered_map<uint32_t, std::vector<Entity>> componentMap;

    std::vector<TransformComponent> transforms;
    std::vector<Entity> transformEntities; //For efficient removals
    std::unordered_map<Entity, size_t> transformIndices;

    std::vector<MeshComponent> meshes;
    std::vector<Entity> meshEntities;
    std::unordered_map<Entity, size_t> meshIndices;

    std::vector<MaterialComponent> materials;
    std::vector<Entity> materialEntities;
    std::unordered_map<Entity, size_t> materialIndices;

    std::vector<PhysicsComponent> physics;
    std::vector<Entity> physicsEntities;
    std::unordered_map<Entity, size_t> physicsIndices;

    std::vector<BoundingBoxComponent> boundingBoxes;
    std::vector<Entity> boundingBoxEntities;
    std::unordered_map<Entity, size_t> boundingBoxIndices;

    std::vector<SpatialPartitioningComponent> spatials;
    std::vector<Entity> spatialEntities;
    std::unordered_map<Entity, size_t> spatialIndices;

    std::vector<RenderableComponent> renderables;
    std::vector<Entity> renderableEntities;
    std::unordered_map<Entity, size_t> renderableIndices;

    std::vector<SceneNodeComponent> sceneNodes;
    std::vector<Entity> sceneNodeEntities;
    std::unordered_map<Entity, size_t> sceneNodeIndices;

    std::vector<MetadataComponent> metadata;
    std::vector<Entity> metadataEntities;
    std::unordered_map<Entity, size_t> metadataIndices;

    template<typename Component, typename... Args>
    Component* addComponent(Entity e, Args&&... args){
        auto &vec = ComponentAccessor<Component>::getVector(this);
        auto &map = ComponentAccessor<Component>::getMap(this);
        auto &entityVec = ComponentAccessor<Component>::getEntityVector(this);

        vec.emplace_back(std::forward<Args>(args)...);
        entityVec.push_back(e);
        map[e] = vec.size() - 1;

        uint32_t hashKey = ComponentAccessor<Component>::hash;
        auto &compEntities = componentMap[hashKey];
        auto it = std::lower_bound(compEntities.begin(), compEntities.end(), e);
        if(it == compEntities.end() || *it != e){
            compEntities.insert(it, e);
        }

        return &vec[map[e]];
    }

    template<typename T>
    auto& getIndices(){
        return ComponentAccessor<T>::getMap(this);
    }

    template<typename T>
    auto &getVector(){
        return ComponentAccessor<T>::getVector(this);
    }

    template<typename... Components>
    uint32_t getHash(){
       return (ComponentAccessor<Components>::hash | ...);
    }

    template<typename... Components>
    std::vector<Entity> getEntitiesWith(){
        if(sizeof...(Components) == 0){DEBUGGER_LOG(WARNING, "Size of components is empty!", "ECS");}

        const auto* smallest = &ComponentAccessor<typename std::tuple_element<0, std::tuple<Components...>>::type>::getEntityVector(this);
  
        const std::vector<const std::vector<Entity>*> vecPtrs{ &ComponentAccessor<Components>::getEntityVector(this)... };
        for(const auto* vec : vecPtrs) {
            if(vec->size() < smallest->size()) smallest = vec;
        }

        if(smallest->size() <= 0){
          DEBUGGER_LOG(ERROR, "No valid entities with given components!", "ECS");
          return {std::numeric_limits<Entity>::min()}; //Error return
        }

        std::vector<Entity> result;
        result.reserve(smallest->size());

        for(Entity e : *smallest){
          bool hasAllComponents = (... && (ComponentAccessor<Components>::getMap(this).count(e) > 0));
          if(hasAllComponents){result.push_back(e);}
        }
        return result;
    }

    template<typename Component>
    void removeComponent(Entity e){
        auto &vec = ComponentAccessor<Component>::getVector(this);
        auto &entities = ComponentAccessor<Component>::getEntityVector(this);
        auto &map =ComponentAccessor<Component>::getMap(this);

        auto it = map.find(e);
        if(it == map.end()) return;

        size_t index = it->second;
        size_t lastIndex = vec.size() - 1;

        if(index != lastIndex){
            std::swap(vec[index], vec[lastIndex]);
            std::swap(entities[index], entities[lastIndex]);
            map[entities[index]] = index;
        }

        vec.pop_back();
        entities.pop_back();
        map.erase(it);
    }

    public:
        uint32_t nextEntity = 1;
};


//Specializations
template<>
struct ComponentAccessor<TransformComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->transforms; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->transformEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->transformIndices; }
    static inline uint32_t hash = 1u << 0;
};

template<>
struct ComponentAccessor<MeshComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->meshes; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->meshEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->meshIndices; }
    static inline uint32_t hash = 1u << 1;
};

template<>
struct ComponentAccessor<MaterialComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->materials; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->materialEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->materialIndices; }
    static inline uint32_t hash = 1u << 2;
};

template<>
struct ComponentAccessor<PhysicsComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->physics; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->physicsEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->physicsIndices; }
    static inline uint32_t hash = 1u << 3;
};

template<>
struct ComponentAccessor<BoundingBoxComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->boundingBoxes; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->boundingBoxEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->boundingBoxIndices; }
    static inline uint32_t hash = 1u << 4;
};

template<>
struct ComponentAccessor<SpatialPartitioningComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->spatials; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->spatialEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->spatialIndices; }
    static inline uint32_t hash = 1u << 5;
};

template<>
struct ComponentAccessor<RenderableComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->renderables; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->renderableEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->renderableIndices; }
    static inline uint32_t hash = 1u << 6;
};

template<>
struct ComponentAccessor<SceneNodeComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->sceneNodes; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->sceneNodeEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->sceneNodeIndices; }
    static inline uint32_t hash = 1u << 7;
};

template<>
struct ComponentAccessor<MetadataComponent> {
    static auto &getVector(ComponentStorage* storage){ return storage->metadata; }
    static auto &getEntityVector(ComponentStorage* storage){ return storage->metadataEntities; }
    static auto &getMap(ComponentStorage* storage){ return storage->metadataIndices; }
    static inline uint32_t hash = 1u << 8;
};

#endif
