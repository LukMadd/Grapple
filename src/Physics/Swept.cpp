#include "Physics/Swept.hpp"
#include <algorithm>

SweepResult sweepAABB(AABB& moving, glm::vec3& velocity, AABB& staticBox){
glm::vec3 invEntry;
    glm::vec3 invExit;

    glm::vec3 entry;
    glm::vec3 exit;

    const float INF = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; i++)
    {
        // NO MOVEMENT ON AXIS
        if (velocity[i] == 0.0f)
        {
            // separated on stationary axis -> impossible collision
            if (moving.max[i] <= staticBox.min[i] ||
                moving.min[i] >= staticBox.max[i])
            {
                return {1.0f, glm::vec3(0.0f), false};
            }

            entry[i] = -INF;
            exit[i] = INF;
            continue;
        }

        // MOVING POSITIVE
        if (velocity[i] > 0.0f)
        {
            invEntry[i] = staticBox.min[i] - moving.max[i];
            invExit[i]  = staticBox.max[i] - moving.min[i];
        }
        // MOVING NEGATIVE
        else
        {
            invEntry[i] = staticBox.max[i] - moving.min[i];
            invExit[i]  = staticBox.min[i] - moving.max[i];
        }

        entry[i] = invEntry[i] / velocity[i];
        exit[i]  = invExit[i]  / velocity[i];
    }

    float entryTime = std::max({ entry.x, entry.y, entry.z });
    float exitTime  = std::min({ exit.x, exit.y, exit.z });

    SweepResult result;
    result.hit =
        entryTime <= exitTime &&
        entryTime >= 0.0f &&
        entryTime <= 1.0f;

    result.t = result.hit ? entryTime : 1.0f;
    result.normal = glm::vec3(0.0f);

    if (!result.hit)
        return result;

    if (entry.x >= entry.y && entry.x >= entry.z)
    {
        result.normal.x = (velocity.x > 0.0f) ? -1.0f : 1.0f;
    }
    else if (entry.y >= entry.z)
    {
        result.normal.y = (velocity.y > 0.0f) ? -1.0f : 1.0f;
    }
    else
    {
        result.normal.z = (velocity.z > 0.0f) ? -1.0f : 1.0f;
    }

    return result;
  }

bool checkCollision(const AABB& a, const AABB& b, float eps){
    return (a.min.x <= b.max.x + eps && a.max.x >= b.min.x - eps) &&
           (a.min.y <= b.max.y + eps && a.max.y >= b.min.y - eps) &&
           (a.min.z <= b.max.z + eps && a.max.z >= b.min.z - eps);
}
