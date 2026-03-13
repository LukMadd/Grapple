#ifndef _CAMERA_MANAGER_HPP
#define _CAMERA_MANAGER_HPP

#include "Camera.hpp"
#include <memory>
#include <vector>

namespace EngineCamera{
    class CameraManager{
        public:
            bool hasCameraChanged = false;

            std::shared_ptr<Camera> getCurrentCamera();

            std::vector<std::shared_ptr<Camera>> getCameras() {return cameras;}

            void pushCamera(std::shared_ptr<Camera> camera);

            void changeCamera(uint32_t cameraIndex);

            void checkIfCamerasEmpty();

            void operator=(const CameraManager* other){
              this->cameras = other->cameras;
            }

            int currentID = 0;

        private:
            std::vector<std::shared_ptr<Camera>> cameras;

            uint32_t currentCameraIndex = 0;
    };
}

#endif
