#include "Input/Input.hpp"
#include "ECS/Components.hpp"
#include "imgui_impl_glfw.h"
#include "EngineUtility.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

using namespace Engine;
using namespace EngineUtility;

namespace EngineInput{
    EngineCamera::Camera* Input::inputCamera = nullptr;
    Entity Input::selectedEntity = nullEntity;
    EngineScene::Scene* Input::inputScene = nullptr;
    EngineContext** Input::context = nullptr;

    std::unordered_map<int, bool> Input::keyStates;
    std::unordered_map<int, bool> Input::mouseButtonStates;

    void Input::init(GLFWwindow* windowRef, EngineContext** engine_context){
        window = windowRef;
        context = engine_context;
    }

    void Input::KeyCallBack(GLFWwindow *window, int key, int scanCode, int action, int mods){
        keyStates[key] = (action != GLFW_RELEASE);
    }

    void Input::MouseButtonCallBack(GLFWwindow *window, int button, int action, int mods){
        mouseButtonStates[button] = (action != GLFW_RELEASE);
      
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int windowFBWidth, windowFBHeight;
        glfwGetFramebufferSize(window, &windowFBWidth, &windowFBHeight);
       
        float ndcX = (2.0f * mouseX) / windowFBWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY) / windowFBHeight;

        glm::vec4 rayStartNDC(ndcX, ndcY, 0.0f, 1.0f);
        glm::vec4 rayEndNDC  (ndcX, ndcY, 1.0f, 1.0f);

        std::array<glm::vec4, 2> worldRays;
        
        worldRays = inputCamera->transformRay(rayStartNDC, rayEndNDC);

        glm::vec3 rayOrigin = glm::vec3(worldRays[0]);
        glm::vec3 farPoint = glm::vec3(worldRays[1]);
        glm::vec3 rayDir = glm::normalize(farPoint - rayOrigin);

        float closestDistance = FLT_MAX;
        const float MAX_DISTANCE = 100.0f;

        for(auto& entity : (*context)->ecs.view<RenderableComponent, BoundingBoxComponent, MetadataComponent>()){
            auto* boundingBox = (*context)->ecs.getComponent<BoundingBoxComponent>(entity);
            auto* metadata = (*context)->ecs.getComponent<MetadataComponent>(entity);
            auto* selected_metadata = (*context)->ecs.getComponent<MetadataComponent>(selectedEntity);
           
            float t;
            glm::vec3 worldMin = boundingBox->worldBoundingBox.min;
            glm::vec3 worldMax = boundingBox->worldBoundingBox.max;
            if(rayIntersectsAABB(rayOrigin, rayDir, worldMin, worldMax, t)){
                if(t < closestDistance && t > 0.0f && t < MAX_DISTANCE){
                    closestDistance = t;

                    if(selectedEntity != entity){
                        if(selectedEntity != nullEntity){
                            selected_metadata->selected = false;
                        }
                        selectedEntity = entity;
                        metadata->selected = true;
                    }
                }
            }
        }
    }

    void Input::setCallBacks(){
        glfwSetWindowUserPointer(window, this);

        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods){
            ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
            Input::KeyCallBack(window, key, scancode, action, mods);
        });

        glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int c){
            ImGui_ImplGlfw_CharCallback(window, c);
        });

        glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods){
            ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            Input::MouseButtonCallBack(window, button, action, mods);
        });

        glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset){
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        });

         
        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos){
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

        auto input = static_cast<Input*>(glfwGetWindowUserPointer(window));

        if(input->firstMouse || input->ignoreMouseDelta){
            input->lastX = xpos;
            input->lastY = ypos;
            input->firstMouse = false;
            input->ignoreMouseDelta = false;
            input->xOffset = 0.0;
            input->yOffset = 0.0;
            return;
        }

        input->xOffset = xpos - input->lastX;
        input->yOffset = input->lastY - ypos;

        input->lastX = xpos;
        input->lastY = ypos;
        });
    }

    glm::vec2 Input::getOffset(){
        glm::vec2 offset{xOffset, yOffset};
        xOffset = 0.0;
        yOffset = 0.0;
        return offset;
    }

    void Input::resetMouse(){
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        lastX = mouseX;
        lastY = mouseY;
        xOffset = 0.0;
        yOffset = 0.0;
        ignoreMouseDelta = true;
    }

    bool Input::isKeyPressed(int key){
        return keyStates[key];
    }

    bool Input::isButtonPressed(int button){
        return mouseButtonStates[button];
    }
}
