#ifndef COMPONENT_STORAGE_HPP
#define COMPONENT_STORAGE_HPP

#include <algorithm>
#include <memory>
#include <typeindex>
#include <vector>
#include <unordered_map>
#include "Components.hpp"
#include "Debug/Debugger.hpp"

using Indice = size_t;

struct IStorage{
  std::vector<Entity> entities;
  std::unordered_map<Entity, Indice> map;

  virtual size_t size() = 0; //RETURNS THE SIZE OF THE ENTITY VECTOR
  virtual void add_entity(Entity entity, Indice index) = 0;
  virtual void remove_entity(Entity entity) = 0;
  virtual bool hasEntity(Entity entity) = 0;

  virtual ~IStorage() = default;
  virtual std::unique_ptr<IStorage> clone() const = 0;
};

template<typename T>
struct Storage : IStorage{
  std::vector<T> components;
  std::vector<Entity> entities;
  std::unordered_map<Entity ,Indice> map;
  
  size_t size() override{
    return entities.size();
  }

  void add_entity(Entity entity, Indice index) override{
    if(!std::binary_search(entities.begin(), entities.end(), entity)){
      entities.push_back(entity);
    }

    map[entity] = index;

  }

  void remove_entity(Entity entity) override{
    auto it = map.find(entity);
    if(it == map.end()){
      return;
    } else{
      size_t lastIndex = components.size() - 1;
      size_t indexToRemove = it->second; 

      std::swap(components[indexToRemove], components[lastIndex]);
      components.pop_back();
    }

    if(std::binary_search(entities.begin(), entities.end(), entity)){
      size_t lastIndex = entities.size() - 1;
      size_t indexToRemove = entity;

      Entity movedEntity = entities[lastIndex];

      std::swap(entities[indexToRemove], entities[lastIndex]);
      entities.pop_back();

      if(lastIndex != indexToRemove){
        map[movedEntity] = indexToRemove;
      }
    }
  }

  bool hasEntity(Entity entity) override{
    return map.count(entity) > 0; 
  }

  std::unique_ptr<IStorage> clone() const override{
    return std::make_unique<Storage<T>>(*this);
  }

};

struct ComponentStorage{
  public:
    std::vector<Entity> entities{}; //Whole list of entities,fast iteration
    std::unordered_map<Entity, Indice> entityLookup; //For fast lookups

    void addEntity(Entity e){
      if(entityLookup.find(e) == entityLookup.end()){
        entities.push_back(e);
        entityLookup[e] = entities.size() - 1;
      }

      nextEntity++;
    }

    template<typename Component, typename... Args>
    Component* addComponent(Entity e, Args&&... args){
      if(entity_component_storage.find(typeid(Component)) == entity_component_storage.end()){
        entity_component_storage[typeid(Component)] = std::make_unique<Storage<Component>>();  
      }

      Storage<Component>* storage = static_cast<Storage<Component>*>(entity_component_storage[typeid(Component)].get());
      
      std::vector<Component>& components = storage->components;
      components.emplace_back(std::forward<Args>(args)...);

      size_t indice = components.size() - 1;
      storage->add_entity(e, indice);

      if(entityLookup.find(e) == entityLookup.end()){
        entities.push_back(e);
        entityLookup[e] = entities.size() - 1;
      }

      return &components[indice];
    }

    template<typename Component>
    Component* getComponent(Entity e){
      Storage<Component>* storage = static_cast<Storage<Component>*>(entity_component_storage[typeid(Component)].get());

      size_t entityIndex = storage->map[e];

      return &storage->components[entityIndex];
    }

    template<typename... Components>
    std::vector<Entity> getEntitiesWith(){
      if constexpr(sizeof...(Components) == 0){
        DEBUGGER_LOG(WARNING, "Size of components is empty!", "ECS");
        return {};
      }

      using First = std::tuple_element_t<0, std::tuple<Components...>>;
      if constexpr(sizeof...(Components) == 1){
        if(entity_component_storage.count(typeid(Components)...) > 0){
          return static_cast<Storage<First>*>(entity_component_storage[typeid(First)].get())->entities;

        }
      }
      
      const std::vector<Entity>* smallest = &static_cast<Storage<First>*>(entity_component_storage[typeid(First)].get())->entities;

      const std::vector<const std::vector<Entity>*> vecPtrs = 
        {&static_cast<Storage<Components>*>(
          entity_component_storage.at(typeid(Components)).get())->entities...};

      for(auto& vec : vecPtrs){
        if(vec->size() < smallest->size()){
          smallest = vec;
        }
      }

      if(smallest->empty()){
        DEBUGGER_LOG(ERROR, "No valid entities with given components!", "ECS");
        return {};
      }

      std::vector<Entity> result;
      result.reserve(smallest->size());
        
      for(auto entity : *smallest){
        bool hasAllComponents = (... && 
                                 (entity_component_storage.count(typeid(Components)) > 0 &&
                                 entity_component_storage[typeid(Components)]->hasEntity(entity)));
        if(hasAllComponents) result.push_back(entity);
      }

      return result;
    }

    template<typename Component>
    void removeComponent(Entity e){
      IStorage* storage = entity_component_storage[typeid(Component)].get(); //Not Storage<T> because the type does not matter
      
      if(storage->hasEntity(e) == false) return; 

      storage->remove_entity(e);
    }

  void removeEntity(Entity e){
      auto it = entityLookup.find(e);
      if(it != entityLookup.end()){
        size_t index = it->second;
        Entity lastEntity = entities.back();

        entities[index] = lastEntity;
        entities.pop_back();

        if(lastEntity != e){
          entityLookup[lastEntity] = index;
        }
        entityLookup.erase(it);
      }

      for(auto& [type, storage] : entity_component_storage){
        if(storage->hasEntity(e)){
          storage->remove_entity(e);
        }
      }
    }

  template<typename Component>
  bool hasComponent(Entity e){
    if(entity_component_storage.count(typeid(Component)) == false){
      return false;
    }

    return entity_component_storage[typeid(Component)]->hasEntity(e);
  }

  std::vector<Entity> getAllEntities(){
    return entities;
  }

  ComponentStorage() = default;
  ~ComponentStorage() = default;

  ComponentStorage(const ComponentStorage& other){
    for(auto& [type, storage] : other.entity_component_storage){
      entity_component_storage[type] = storage->clone();
    }
  }


  uint32_t nextEntity = 1;

  private:
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> entity_component_storage;
};

#endif
