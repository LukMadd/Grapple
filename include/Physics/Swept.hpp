#ifndef SWEPT_HPP
#define SWEPT_HPP

#include "Misc/BoundingBox.hpp"

struct SweepResult{
  float t; //At what time it hits
  glm::vec3 normal; //Direction of hit
  bool hit;
};

SweepResult sweepAABB(AABB& moving, glm::vec3& velocity, AABB& staticBox);

bool checkCollision(const AABB& a, const AABB& b, float eps = 0.00001f);

#endif
