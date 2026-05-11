#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "Misc/BoundingBox.hpp"
#include "Misc/Mesh.hpp"
#include "Misc/Material.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

using Entity = uint32_t;
constexpr int nullEntity = std::numeric_limits<uint32_t>::max();

struct TransformComponent{
    glm::vec3 position{0.0f};
    glm::quat rotation{1,0,0,0};
    glm::vec3 scale{1.0f};

    glm::mat4 modelMatrix{1.0f};
    glm::mat4 localMatrix{1.0f};
    glm::mat4 worldMatrix{1.0f};

    TransformComponent() = default;
};

struct MeshComponent{
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

    MeshComponent() = default;
};

struct MaterialComponent{
    std::shared_ptr<Material> material;
    bool hasTexture = false;

    MaterialComponent() = default;
};

struct PhysicsComponent{
    glm::vec3 velocity{0.0f};
    bool isStatic = false;

    PhysicsComponent() = default;
};

struct BoundingBoxComponent{
    AABB localBoundingBox = AABB();

    BoundingBoxComponent() = default;
};

struct SpatialPartitioningComponent{
    std::vector<uint64_t> cells{};
    bool assignedToCell = false;

    SpatialPartitioningComponent() = default;
};

struct RenderableComponent {
    uint32_t uniformIndex = 0;

    RenderableComponent() = default;
};

struct SceneNodeComponent{
    Entity node = nullEntity;
    Entity parent = nullEntity;

    SceneNodeComponent() = default;
};

struct MetadataComponent{
    std::string name;
    std::string type;
    std::string uuid;
    bool selected = false;

    MetadataComponent() = default;
};

#endif
