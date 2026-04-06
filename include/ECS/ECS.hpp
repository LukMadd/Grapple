#ifndef ECS_HPP
#define ECS_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include "Components.hpp"
#include "ComponentStorage.hpp"
#include "Debug/Debugger.hpp"
#include "EntityFunctions.hpp"

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

        componentStorage->removeEntity(e);
    }

    template<typename... Components>
    void addComponents(Entity e){
        (addComponent<Components>(e), ...);
    }

    template<typename Component>
    Component* getComponent(Entity e){
        auto& vec = componentStorage->component_map.get_components<Component>();
        size_t indice = componentStorage->indices.get_component_indice<Component>(e);
       
        if(indice == std::numeric_limits<size_t>::max() || indice >= vec.size()){
          DEBUGGER_LOG(CRITICAL, "Invalid indice!", "ECS");
           return nullptr;
        }
        return &vec.at(indice);
    }

    template<typename... Components>
    auto& getComponents(Entity e) {
        return std::tie(getComponent<Components>(e)...);
    }

    template<typename Component>
    bool hasComponent(Entity e){
        auto& indices = componentStorage->indices.get_component_indice_map<Component>();
        return indices.find(e) != indices.end();
    }

    template<typename... Components>
    std::vector<Entity> view(){
        if(!componentStorage){
          throw std::runtime_error("NO COMPONENT STORAGE!!!");
        }
        if constexpr (sizeof...(Components) == 1) {
            return componentStorage->entity_component_map.get_entities_of_type<Components...>();
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
          entityID = availableIDs[0];
        } else{
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
