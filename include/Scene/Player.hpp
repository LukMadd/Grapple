#include "ECS/ECS.hpp"
#include "Camera/Camera.hpp"
#include "ECS/Components.hpp"

using namespace EngineCamera;

struct Player3D{
  public:
    Player3D(std::shared_ptr<Camera> camera = nullptr, ECS* ecs = nullptr);

    Entity entity;
    glm::vec3 front;
    Camera* camera; //Camera position and rotation is determined by player transform
    TransformComponent* transform; //For easy position/rotation/scale getters

    glm::vec3 getPosition();
    void move(glm::vec3 position, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial) const;

    bool isValid(){return valid;};

  private:
    bool valid = false;
};

struct PlayerSettings{
  inline static float movement_speed = 1.0f;
};
