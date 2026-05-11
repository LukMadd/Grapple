#include "ECS/Components.hpp"
#include <glm/glm.hpp>

namespace EngineUtility{
    bool rayIntersectsAABB(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& boxMin,
        const glm::vec3& boxMax,
        float& t
    );

    AABB getWorldAABB(TransformComponent& t, AABB& local);
    AABB getWorldAABBWithoutComponent(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale, 
                                     AABB& local);


    void initDebugSubSystems();
}
