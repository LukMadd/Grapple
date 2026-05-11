#include "EngineUtility.hpp"
#include <utility>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/string_cast.hpp>
#include "Debug/Debugger.hpp"

namespace EngineUtility{
       bool rayIntersectsAABB(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& boxMin,
        const glm::vec3& boxMax,
        float& t
    ){
        const float EPSILON = 1e-6f;
        //"axis" for debugging
        auto updateSlab = [&](float rayO, float rayD, float bMin, float bMax, float &t0, float &t1, char axis) -> bool {
                if(fabs(rayD) < EPSILON){
                    if (rayO < bMin || rayO > bMax) return false;
                    return true;
                }
                float tNear = (bMin - rayO) / rayD;
                float tFar  = (bMax - rayO) / rayD;
                if (tNear > tFar) std::swap(tNear, tFar);
                t0 = std::max(t0, tNear);
                t1 = std::min(t1, tFar);
                return t0 <= t1;
            };

        float tMin = 0.0f;
        float tMax = FLT_MAX;

        if(!updateSlab(rayOrigin.x, rayDir.x, boxMin.x, boxMax.x, tMin, tMax, 'X')) return false;
        if(!updateSlab(rayOrigin.y, rayDir.y, boxMin.y, boxMax.y, tMin, tMax, 'Y')) return false;
        if(!updateSlab(rayOrigin.z, rayDir.z, boxMin.z, boxMax.z, tMin, tMax, 'Z')) return false; 

        t = tMin;

        glm::vec3 intersection = rayOrigin + rayDir * t;
        return true;
    }

    AABB getWorldAABB(TransformComponent& t, AABB& local){
      glm::vec3 halfExtents = (local.max - local.min) * 0.5f;
      glm::vec3 center = local.center();

      glm::mat3 rotScale = glm::mat3(t.rotation) * glm::mat3(glm::scale(glm::mat4(1.0f), t.scale));

      glm::vec3 worldCenter = t.position + rotScale * center;

      glm::vec3 worldHalfExtents =
          glm::abs(rotScale[0]) * halfExtents.x +
          glm::abs(rotScale[1]) * halfExtents.y +
          glm::abs(rotScale[2]) * halfExtents.z;

      AABB result;
      result.min = worldCenter - worldHalfExtents;
      result.max = worldCenter + worldHalfExtents;

      return result;
    }

    AABB getWorldAABBWithoutComponent(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale, 
                                     AABB& local){
      glm::vec3 halfExtents = (local.max - local.min) * 0.5f;
      glm::vec3 center = local.center();

      glm::mat3 rotScale = glm::mat3(rotation) * glm::mat3(glm::scale(glm::mat4(1.0f), scale));

      glm::vec3 worldCenter = position + rotScale * center;

      glm::vec3 worldHalfExtents =
          glm::abs(rotScale[0]) * halfExtents.x +
          glm::abs(rotScale[1]) * halfExtents.y +
          glm::abs(rotScale[2]) * halfExtents.z;

      AABB result;
      result.min = worldCenter - worldHalfExtents;
      result.max = worldCenter + worldHalfExtents;

      return result;
 
    }

    void initDebugSubSystems(){
        //Initialize any debug subsystems here in the future
        Debugger::get().initDebugSystem("Engine");
        Debugger::get().initDebugSystem("Scene_Manager");
        Debugger::get().initDebugSystem("Context_Manager");
        Debugger::get().initDebugSystem("ECS");
        Debugger::get().initDebugSystem("Renderer");
        Debugger::get().initDebugSystem("UI");
    }
}
