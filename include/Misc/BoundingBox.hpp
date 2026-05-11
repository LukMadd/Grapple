#ifndef BOUNDING_BOX_HPP
#define BOUNDING_BOX_HPP

#include <glm/glm.hpp>

struct AABB{
    glm::vec3 min;
    glm::vec3 max;

    bool isInitialized = false;

    glm::vec3 center() const {
      return (min + max) * 0.5f;
    }

    AABB()
    : min(FLT_MAX, FLT_MAX, FLT_MAX),
        max(-FLT_MAX, -FLT_MAX, -FLT_MAX),
        isInitialized(false) {}
};

struct OBB{
    glm::vec3 center;
    glm::vec3 halfSize;
    glm::mat3 axes;
};

inline OBB AABB_to_OBB(const glm::vec3& min, const glm::vec3& max) {
    OBB obb;
    obb.center = (min + max) * 0.5f;
    obb.halfSize = (max - min) * 0.5f;
    obb.axes = glm::mat3(1.0f);
    return obb;
}

#endif
