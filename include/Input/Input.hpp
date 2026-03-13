#ifndef _INPUT_HPP
#define _INPUT_HPP

#include "Camera/Camera.hpp"

#include "EngineContext.hpp"
#include "Scene/Scene.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <unordered_map>

namespace Engine{
    enum class Key{
        KEY_ARROW_UP = GLFW_KEY_UP,
        KEY_ARROW_DOWN = GLFW_KEY_DOWN,
        KEY_W = GLFW_KEY_W,
        KEY_A = GLFW_KEY_A,
        KEY_S = GLFW_KEY_S,
        KEY_D = GLFW_KEY_D,
        KEY_SPACE = GLFW_KEY_SPACE,
        KEY_SHIFT = GLFW_KEY_LEFT_SHIFT,
        KEY_ESCAPE = GLFW_KEY_ESCAPE,
        KEY_TAB = GLFW_KEY_TAB,
        KEY_F10 = GLFW_KEY_F10,
        KEY_PERIOD = GLFW_KEY_PERIOD,
        KEY_GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT
    };
}

    using namespace Engine;

namespace EngineInput{
    class Input{
        private:
            GLFWwindow* window;
            static EngineContext** context;

        public:
            void init(GLFWwindow* windowRef, EngineContext** engine_context);

            double xOffset = 0.0, yOffset = 0.0;
            double lastX = 0.0, lastY = 0.0;
            bool firstMouse = true;

            static EngineCamera::Camera *inputCamera;
            static Entity selectedEntity;
            static EngineScene::Scene *inputScene;

            static std::unordered_map<int, bool> keyStates;
            static std::unordered_map<int, bool> mouseButtonStates; 

            static void KeyCallBack(GLFWwindow* window, int key, int scanCode, int action, int mods);
            static void MouseButtonCallBack(GLFWwindow* window, int button, int action, int mods);

            bool ignoreMouseDelta = false;
            glm::vec2 getOffset();

            void resetMouse();

            void setCallBacks();
            
            static Input& get(){
                static Input instance;
                return instance;
            }

            static bool isKeyPressed(int key);
            static bool isButtonPressed(int button);
    };

}

    #endif
