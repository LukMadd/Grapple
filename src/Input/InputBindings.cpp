#include "Input/InputBindings.hpp"
#include "ECS/Components.hpp"
#include "ECS/EntityFunctions.hpp"
#include "Input/Action.hpp"
#include "Renderer/Window.hpp"

using namespace Engine;
using namespace EnginePartitioning;

namespace EngineInput {
std::unordered_map<int, std::function<void()>>
InputBindings::getDeveloperBindings(Camera *camera, GLFWwindow *window,
                                    ActionManager *actionManager) {
  std::unordered_map<int, std::function<void()>> devBindings;

  auto dev_exit_fullscreen = [window]() {
    if (Engine::isFullscreen) {
      Engine::toggleFullscreen(window);
    }
  };

  auto dev_enter_fullscreen = [window]() {
    if (Engine::isFullscreen == false) {
      toggleFullscreen(window);
    }
  };

  auto dev_show_cursor = [window]() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    Input::get().resetMouse();
  };

  auto dev_hide_cursor = [window]() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    Input::get().resetMouse();
  };

  auto dev_lock_camera = [actionManager]() {
    if(!was_camera_locked_action_active){
      is_camera_locked = !is_camera_locked;
    }
    is_camera_locked_action_active = true;
  };

  auto dev_move_camera_up = [camera]() {
    if (camera->getTargetTransform() != nullptr)
      return;
    camera->moveUp(camera->getVelocity());
  };

  auto dev_move_camera_down = [camera]() {
    if (camera->getTargetTransform() != nullptr)
      return;
    camera->moveDown(camera->getVelocity());
  };

  auto dev_move_camera_forward = [camera]() {
    if (camera->getTargetTransform() != nullptr)
      return;
    camera->moveForward(camera->getVelocity());
  };

  auto dev_move_camera_backward = [camera]() {
    if (camera->getTargetTransform() != nullptr)
      return;
    camera->moveBackward(camera->getVelocity());
  };

  auto dev_move_camera_left = [camera]() {
    if (camera->getTargetTransform() != nullptr)
      return;
    camera->moveLeft(camera->getVelocity());
  };

  auto dev_move_camera_right = [camera]() {
    if (camera->getTargetTransform() != nullptr)
      return;
    camera->moveRight(camera->getVelocity());
  };

  devBindings[Developer::EXIT_FULL_SCREEN] = dev_exit_fullscreen;
  devBindings[Developer::ENTER_FULL_SCREEN] = dev_enter_fullscreen;
  devBindings[Developer::SHOW_CURSOR] = dev_show_cursor;
  devBindings[Developer::HIDE_CURSOR] = dev_hide_cursor;
  devBindings[Developer::TOGGLE_CAMERA_LOCK] = dev_lock_camera;
  devBindings[Developer::CAMERA_UP] = dev_move_camera_up;
  devBindings[Developer::CAMERA_DOWN] = dev_move_camera_down;
  devBindings[Developer::CAMERA_FORWARD] = dev_move_camera_forward;
  devBindings[Developer::CAMERA_BACKWARD] = dev_move_camera_backward;
  devBindings[Developer::CAMERA_LEFT] = dev_move_camera_left;
  devBindings[Developer::CAMERA_RIGHT] = dev_move_camera_right;

  return devBindings;
}

std::unordered_map<int, std::function<void()>>
InputBindings::getPlayerBindings3D(GLFWwindow *window, ActionManager *actionManager, ECS* ecs, Spatial_Partitioner* spatial, Player3D& player){
  std::unordered_map<int, std::function<void()>> playerBindings;
  auto* playerTransform = ecs->getComponent<TransformComponent>(player.entity); 

  auto player_exit_fullscreen = [window]() {
    if(Engine::isFullscreen) {
      Engine::toggleFullscreen(window);
    }
  };

  auto player_enter_fullscreen = [window]() {
    if(Engine::isFullscreen == false) {
      toggleFullscreen(window);
    }
  };

  auto player_move_forward = [ecs, player, spatial](){
    player.move(player.transform->position + (glm::vec3(PlayerSettings::movement_speed) * player.camera->getFront()), ecs, spatial);
  };

  auto player_move_backward = [ecs, player, spatial](){
    player.move(player.transform->position - (glm::vec3(PlayerSettings::movement_speed) * player.camera->getFront()), ecs, spatial);
  };

  auto player_move_left = [ecs, player, spatial](){
    player.move(player.transform->position - (glm::vec3(PlayerSettings::movement_speed) * glm::normalize(glm::cross(player.camera->getFront(), player.camera->getUp()))), ecs, spatial); 
  };

  auto player_move_right = [ecs, player, spatial](){
    player.move(player.transform->position + (glm::vec3(PlayerSettings::movement_speed) * glm::normalize(glm::cross(player.camera->getFront(), player.camera->getUp()))), ecs, spatial); 
  };

  playerBindings[Player::EXIT_FULLSCREEN] = player_exit_fullscreen;
  playerBindings[Player::ENTER_FULLSCREEN] = player_enter_fullscreen;
  playerBindings[Player::MOVE_FORWARD] = player_move_forward;
  playerBindings[Player::MOVE_BACKWARD] = player_move_backward;
  playerBindings[Player::MOVE_LEFT] = player_move_left;
  playerBindings[Player::MOVE_RIGHT] = player_move_right;

  return playerBindings;
}

} // namespace EngineInput
