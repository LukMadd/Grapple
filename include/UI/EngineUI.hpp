#ifndef _ENGINE_UI_HPP
#define _ENGINE_UI_HPP

#include "Camera/Camera.hpp"
#include "Core/ResourceManager.hpp"
#include "ECS/Components.hpp"
#include "ECS/EntityFunctions.hpp"
#include "Scene/Scene.hpp"

#include "Scene/SceneManager.hpp"

namespace EngineUI{
    class EngineUI{
        public:
            void drawMainLayout(EngineScene::SceneManager *sceneManager, 
                                EngineCamera::CameraManager *cameraManager, ECS* ecs);

            void drawSceneHierarchy(EngineScene::Scene *scene, ECS* ecs);

            void drawEntityInspector(EngineScene::Scene *scene, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial);

            void drawCameraInspector();

            void drawSceneInspector(EngineScene::SceneManager *sceneManager, ECS* ecs);

            void drawRecourses(EngineResource::ResourceManager *recourseManager);

            void drawRenderStats(EngineScene::Scene *scene, float fps, ECS* ecs);

            void drawScenes(EngineScene::SceneManager *sceneManager);
        
        private:
            bool m_showSceneHierarchyWindow = false;
            bool m_showObjectInspector = false;
            bool m_showCameraInspector = false;
            bool m_showSceneInspector = false;
            bool m_showResourceWindow = false;
            bool m_showRenderStats = false;
            bool m_showScenesWindow = false;

            bool m_drawBoundingBoxes = false;

            Entity selectedEntity = nullEntity;
            EngineCamera::Camera *selectedCamera = nullptr;
            EngineScene::Scene *selectedScene = nullptr;

            Entity previousEntity = nullEntity;

        public:
            Entity getSelectedEntity(){return selectedEntity;}
            EngineCamera::Camera*& getSelectedCamera(){return selectedCamera;}
            EngineScene::Scene* getSelectedScene(){return selectedScene;}

            bool getValueofDrawBoundingBoxes(){return m_drawBoundingBoxes;}

            void setSelectedCamera(EngineCamera::Camera *camera){selectedCamera = camera;}
    };
}

#endif
