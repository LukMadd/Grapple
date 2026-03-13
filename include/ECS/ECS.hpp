#ifndef ECS_HPP
#define ECS_HPP

#include <cstdint>
#include <sys/types.h>
#include <vector>
#include "Components.hpp"
#include "ComponentStorage.hpp"
#include "EntityFunctions.hpp"
#include <iostream>

#define DebugSystem "ECS";

struct ECS{
    void addEntity(Entity e){
        componentStorage->entities.push_back(e);
    }

    template<typename Component, typename... Args>
    Component* addComponent(Entity e, Args&&... args){
        return componentStorage->addComponent<Component>(e, std::forward<Args>(args)...);
    }

    void removeEntity(Entity e){
        componentStorage->entities.erase(std::remove(componentStorage->entities.begin(), componentStorage->entities.end(), e), 
                                                     componentStorage->entities.end());

        //There is definetly a better way to do this, I just don't know what it is
        if(hasComponent<TransformComponent>(e)) componentStorage->removeComponent<TransformComponent>(e);
        if(hasComponent<MeshComponent>(e)) componentStorage->removeComponent<MeshComponent>(e);
        if(hasComponent<MaterialComponent>(e)) componentStorage->removeComponent<MaterialComponent>(e);
        if(hasComponent<PhysicsComponent>(e)) componentStorage->removeComponent<PhysicsComponent>(e);
        if(hasComponent<BoundingBoxComponent>(e)) componentStorage->removeComponent<BoundingBoxComponent>(e);
        if(hasComponent<SpatialPartitioningComponent>(e)) componentStorage->removeComponent<SpatialPartitioningComponent>(e);
        if(hasComponent<RenderableComponent>(e)) componentStorage->removeComponent<RenderableComponent>(e);
        if(hasComponent<SceneNodeComponent>(e)) componentStorage->removeComponent<SceneNodeComponent>(e);
        if(hasComponent<MetadataComponent>(e)) componentStorage->removeComponent<MetadataComponent>(e);
    }

    template<typename... Components>
    void addComponents(Entity e){
        (addComponent<Components>(e), ...);
    }

    template<typename Component>
    Component* getComponent(Entity e){
        auto& vec = ComponentAccessor<Component>::getVector(componentStorage);
        auto& map = ComponentAccessor<Component>::getMap(componentStorage);
        
        auto it = map.find(e);
        if(it != map.end()){
            return &vec.at(it->second);
        }
        return nullptr;
    }

    template<typename... Components>
    auto& getComponents(Entity e) {
        return std::tie(getComponent<Components>(e)...);
    }

    template<typename T>
    bool hasComponent(Entity e){
        auto& map = componentStorage->getIndices<T>();
        return map.find(e) != map.end();
    }

    template<typename... Components>
    std::vector<Entity> view(){
        if(!componentStorage){
          throw std::runtime_error("NO COMPONENT STORAGE!!!");
        }
        if constexpr (sizeof...(Components) == 1) {
            uint32_t hash = (componentStorage->getHash<Components>() | ...);
            auto it = componentStorage->componentMap.find(hash);
            if(it != componentStorage->componentMap.end()){
                return it->second;
            } else{
                return {};
            }
        } else {
            return componentStorage->getEntitiesWith<Components...>();
        }
    }

    template<typename... Components, typename Func>
    void foreach(Func func){
        for(auto entity : view<Components...>()){
            func(getComponent<Components>(entity)...);
        }
    }

    Entity createEntity(std::string meshPath = "", std::string texturePath = "", std::string name = "", std::string type = ""){
        return EntityFunctions::initEntity(meshPath, texturePath, name, type, this);
    }

    void setComponentStorage(ComponentStorage* storage){
        componentStorage = storage;
    }

    ComponentStorage* getComponentStorage(){
        return componentStorage;
    }

    uint32_t getAvailableEntityID(){
        uint32_t entityID;
        if(!availableIDs.empty()){
          std::cout << "AVAILABLE" << std::endl;   
          entityID = availableIDs[0];
        } else{
            std::cout << "NOT AVAILABLE" << std::endl;
            entityID = componentStorage->nextEntity;
            componentStorage->nextEntity++;
        }
        return entityID;
    }

    uint32_t& getNextEntity(){
        return componentStorage->nextEntity;
    }

    bool isComponentStorageSet(){
        if(componentStorage){
            return true;
        } else{
            return false;
        }
    }

    uint32_t getTotalEntities(){
        return componentStorage->entities.size();
    }

    private:
        ComponentStorage* componentStorage = nullptr; //A bit dangerous but a pretty easy fix if it comes to it
        std::vector<uint32_t> availableIDs;
    };

#endif
