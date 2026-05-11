#ifndef _INPUT_BINDINGS_HPP
#define _INPUT_BINDINGS_HPP

#include "Camera/Camera.hpp"
#include "Scene/Player.hpp"
#include "ECS/ECS.hpp"
#include <GLFW/glfw3.h>

using namespace EngineCamera;

namespace EngineInput {
namespace InputBindings {
  std::unordered_map<int, std::function<void()>> getDeveloperBindings(Camera *camera, GLFWwindow *window, ActionManager *actionManager);
  std::unordered_map<int, std::function<void()>> getPlayerBindings3D(GLFWwindow *window, ActionManager *actionManager, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial, Player3D& player);
}; // namespace InputBindings

inline bool is_camera_locked = false;
inline bool is_camera_locked_action_active = false;
inline bool was_camera_locked_action_active = false;
} // namespace EngineInput

#endif
