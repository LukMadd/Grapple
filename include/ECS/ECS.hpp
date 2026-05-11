#ifndef ECS_HPP
#define ECS_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include "Components.hpp"
#include "ComponentStorage.hpp"
#include "EntityFunctions.hpp"

#define DebugSystem "ECS";

struct ECS{
    void addEntity(Entity e){
      componentStorage->addEntity(e);
    }

    template<typename Component, typename... Args>
    Component* addComponent(Entity e, Args&&... args){
        return componentStorage->addComponent<Component>(e, std::forward<Args>(args)...);
    }

    void removeEntity(Entity e){
      componentStorage->removeEntity(e);
    }

    template<typename... Components>
    void addComponents(Entity e){
        (addComponent<Components>(e), ...);
    }

    template<typename Component>
    Component* getComponent(Entity e){
      if(e == std::numeric_limits<uint32_t>::max()){
        throw std::runtime_error("Invalid entity");
      }
      return componentStorage->getComponent<Component>(e);
    }

    template<typename... Components>
    auto& getComponents(Entity e) {
        return std::tie(getComponent<Components>(e)...);
    }

    template<typename Component>
    bool hasComponent(Entity e){
      return componentStorage->hasComponent<Component>(e);
    }

    template<typename... Components>
    bool hasComponents(Entity e){
      return (... && hasComponent<Components>(e));
    }

    template<typename... Components>
    std::vector<Entity> view(){
        if(!componentStorage){
          throw std::runtime_error("NO COMPONENT STORAGE!!!");
        }
        
        return componentStorage->getEntitiesWith<Components...>();
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
          entityID = availableIDs.at(0);
        } else{
            entityID = componentStorage->nextEntity;
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

    std::vector<Entity> getAllEntities(){
      return componentStorage->getAllEntities();
    } 

    private:
        ComponentStorage* componentStorage = nullptr; //A bit dangerous but a pretty easy fix if it comes to it
        std::vector<uint32_t> availableIDs{};
    };

#endif
