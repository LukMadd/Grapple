#include "UI/EngineUI.hpp"
#include "Camera/CameraManager.hpp"

#include "ECS/Components.hpp"
#include "Renderer/RendererGlobals.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneManager.hpp"
#include "ECS/EntityFunctions.hpp"
#include "imgui.h"

namespace EngineUI{
    void EngineUI::drawMainLayout(EngineScene::SceneManager *sceneManager, 
                                  EngineCamera::CameraManager *cameraManager, ECS* ecs){
            if(ImGui::BeginMenuBar()){
            if(ImGui::BeginMenu("Scene")){
                ImGui::Checkbox("View Scenes", &m_showScenesWindow);
                ImGui::Checkbox("Scene Hierarchy", &m_showSceneHierarchyWindow);
                ImGui::Checkbox("Object Inspector", &m_showObjectInspector);
                ImGui::Checkbox("Camera Inspector", &m_showCameraInspector);
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Engine")){
                ImGui::Checkbox("Resources", &m_showResourceWindow);
                ImGui::Checkbox("Render Stats", &m_showRenderStats);
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Actions")){
                if(ImGui::Button("Add Scene")){
                    sceneManager->addDefaultScene();
                }
                if(ImGui::Button("Add Object")){
                    sceneManager->getCurrentScene()->addDefaultEntity(ecs);
                }
                if(ImGui::Button("Reset Camera")){
                    cameraManager->getCurrentCamera()->resetCamera();
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Debug")){
                ImGui::Checkbox("Draw Bounding Boxes", &m_drawBoundingBoxes);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void EngineUI::drawSceneHierarchy(EngineScene::Scene *scene, ECS* ecs){
        if(m_showSceneHierarchyWindow){
            ImGui::Begin("Scene Hierarchy");
            const char* entity_preview_text;

            if(selectedEntity != nullEntity){
                auto* selected_metadata = ecs->getComponent<MetadataComponent>(selectedEntity);
                entity_preview_text = selected_metadata->name.c_str();
            } else{
                entity_preview_text = "Select an object";
            }

            if(ImGui::BeginCombo("Entities", entity_preview_text)){
                for(auto& entity : ecs->view<MetadataComponent>()){
                    ImGui::PushID(entity);
                    bool is_entity_selected = (selectedEntity == entity);
                    auto entity_metadata = ecs->getComponent<MetadataComponent>(entity);
                    if(ImGui::Selectable(entity_metadata->name.c_str(), is_entity_selected)){
                        MetadataComponent* selected_entity_metadata;
                        if(selectedEntity != nullEntity){
                          selected_entity_metadata = ecs->getComponent<MetadataComponent>(selectedEntity);
                          selected_entity_metadata->selected = false;
                        }
                        selectedEntity = entity;
                        
                        selected_entity_metadata = ecs->getComponent<MetadataComponent>(entity);
                        
                        selected_entity_metadata->selected = true;
                        m_showObjectInspector = true;
                    }
                    if(is_entity_selected){
                        ImGui::SetItemDefaultFocus();
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            

            uint32_t cameraIndex = 0;
            const char* camera_preview_text = selectedCamera ? selectedCamera->name.c_str() : "Select an camera";
            if(ImGui::BeginCombo("Cameras", camera_preview_text)){
                for(auto &camera : scene->cameraManager.getCameras()){
                    ImGui::PushID(camera.get());
                    bool is_camera_selected = (selectedCamera == camera.get());
                    if(ImGui::Selectable(camera->name.c_str(), is_camera_selected)){
                        if(selectedCamera) selectedCamera->selected = false;
                        selectedCamera = camera.get();
                        selectedCamera->selected = true;

                        scene->cameraManager.changeCamera(cameraIndex);

                        m_showCameraInspector = true;
                    }
                    if(is_camera_selected){
                        ImGui::SetItemDefaultFocus();
                    }
                    ImGui::PopID();
                    cameraIndex++;
                }
                ImGui::EndCombo();
            }
            
            ImGui::End();
        }
    }

    void EngineUI::drawEntityInspector(EngineScene::Scene *scene, ECS* ecs, EnginePartitioning::Spatial_Partitioner* spatial){
        if(m_showObjectInspector && selectedEntity || previousEntity != selectedEntity){
            previousEntity = selectedEntity;
            m_showObjectInspector = true;

            auto* metadata = ecs->getComponent<MetadataComponent>(selectedEntity);
            auto* transform = ecs->getComponent<TransformComponent>(selectedEntity);

            ImGui::Begin("Entity Inspector");

            ImGui::Text("ID: %u", selectedEntity);

            if(metadata){
                ImGui::Text("UUID: %s", metadata->uuid.c_str());
            } else{
                ImGui::Text("UUID: %s", "N/A");
            }

            if(metadata){
                ImGui::Text("Name: %s", metadata->name.c_str());
            } else{
                ImGui::Text("Name: %s", "N/A");
            }
            if(transform){
                if(ImGui::TreeNode("Position")){
                    float positionX = transform->position.x;
                    float positionY = transform->position.y;
                    float positionZ = transform->position.z;
                    ImGui::InputFloat("X: ", &positionX);
                    ImGui::InputFloat("Y: ", &positionY);
                    ImGui::InputFloat("Z: ", &positionZ);
                    if(glm::vec3(positionX, positionY, positionZ) != transform->position){
                      EntityFunctions::move(glm::vec3(positionX, positionY, positionZ), selectedEntity, ecs, spatial);
                    }
                    ImGui::TreePop();
                }

                if(ImGui::TreeNode("Rotation")){
                    float rotationX = transform->rotation.x;
                    float rotationY = transform->rotation.y;
                    float rotationZ = transform->rotation.z;
                    ImGui::InputFloat("X: ", &rotationX);
                    ImGui::InputFloat("Y: ", &rotationY);
                    ImGui::InputFloat("Z: ", &rotationZ);
                    glm::vec3 currentRotation = glm::vec3(transform->rotation.x, 
                                                        transform->rotation.y,
                                                        transform->rotation.z);
                    if(glm::vec3(rotationX, rotationY, rotationZ) != currentRotation){
                      EntityFunctions::rotate(glm::vec3(rotationX, rotationY, rotationZ), selectedEntity, ecs, spatial); 
                    }
                    ImGui::TreePop();
                }

                if(ImGui::TreeNode("Scale")){
                    float scaleX = transform->scale.x;
                    float scaleY = transform->scale.y;
                    float scaleZ = transform->scale.z;
                    ImGui::InputFloat("X: ", &scaleX);
                    ImGui::InputFloat("Y: ", &scaleY);
                    ImGui::InputFloat("Z: ", &scaleZ);
                    if(glm::vec3(scaleX, scaleY, scaleZ) != transform->scale){
                      EntityFunctions::scale(glm::vec3(scaleX, scaleY, scaleZ), selectedEntity, ecs, spatial);
                    }
                    ImGui::TreePop();
                }
            }

            if(ImGui::Button("Set As Target")){
                auto* transform = ecs->getComponent<TransformComponent>(selectedEntity);
                selectedCamera->setTarget(selectedEntity, transform);
                selectedCamera->setTargetOffset(glm::vec3(0.0f, 7.5f, -30.0f));
            }

            if(ImGui::Button("Delete")){
                scene->removeEntity(selectedEntity, ecs);
                selectedEntity = nullEntity;
                previousEntity = nullEntity;
                m_showObjectInspector = false;
            }

            ImGui::End();
        }
    }

    void EngineUI::drawCameraInspector(){
        if(m_showCameraInspector && selectedCamera){
            ImGui::Begin("Camera Inspector");

            ImGui::Text("Name: %s", selectedCamera->name.c_str());
            if(ImGui::TreeNode("Position")){
                ImGui::InputFloat("X: ", &selectedCamera->position.x);
                ImGui::InputFloat("Y: ", &selectedCamera->position.y);
                ImGui::InputFloat("Z: ", &selectedCamera->position.z);
                ImGui::TreePop();
            }

            if(ImGui::TreeNode("Rotation")){
                ImGui::InputFloat("Yaw: ", &selectedCamera->getYaw());
                ImGui::InputFloat("Pitch: ", &selectedCamera->getPitch());
                ImGui::TreePop();
            }

            if(ImGui::TreeNode("Attributes")){
                ImGui::InputFloat("FOV: ", &selectedCamera->getFov());
                ImGui::InputFloat("Speed: ", &selectedCamera->getSpeed());
                ImGui::InputFloat("Sensitivity: ", &selectedCamera->getSensitivity());
                ImGui::TreePop();
            }

            if(ImGui::TreeNode("Target")){
                if(selectedCamera->getTarget() != nullEntity){
                    glm::vec3& offset = selectedCamera->getTargetOffset();
                    if(ImGui::TreeNode("Offset")){
                        ImGui::InputFloat("X", &offset.x);
                        ImGui::InputFloat("Y", &offset.y);
                        ImGui::InputFloat("Z", &offset.z);

                        ImGui::TreePop();
                    }
                    if(ImGui::Button("Reset Target")){
                        selectedCamera->resetTarget();
                    }
                } else{
                    ImGui::Text("No selected target");
                }

                ImGui::TreePop();
            }

            ImGui::End();
        }
    }

    void EngineUI::drawSceneInspector(EngineScene::SceneManager *sceneManager, ECS* ecs){
        if(m_showSceneInspector && selectedScene){
            ImGui::Begin("Scene Inspector");

            ImGui::Text("Id: %d", selectedScene->index);
            ImGui::Text("Name: %s", selectedScene->name.c_str());
            ImGui::Text("Entity Count: %d", (int)ecs->getTotalEntities());

            if(ImGui::Button("Delete")){
                sceneManager->deleteScene(selectedScene);
                if(selectedScene){
                    selectedScene = nullptr;
                }
            }

            ImGui::End();
        }
    }

    void EngineUI::drawRecourses(EngineResource::ResourceManager *recourseManager){
        if(m_showResourceWindow){
            ImGui::Begin("Recourses");

            //Can optimize this by finding textures and models by pushing into the vectors if the cache type name is the same 
            //as the desired(recourse.second->type()->name() ==) though it will require refactoring if there is a change of stored type 
            std::vector<std::string> recourseTextures;
            std::vector<std::string> recourseModels;

            //Get the textures & models from recource manager and put them in the vectors above
            for(auto &recourse : recourseManager->getCache()){                           
                if(recourse.second.type() == typeid(std::shared_ptr<Texture>)){
                    recourseTextures.push_back(recourse.first);
                } else if(recourse.second.type() == typeid(std::shared_ptr<Mesh>)){
                    recourseModels.push_back(recourse.first);
                }
            }

            //Put the textures & models in a tree node for the UI
            if(ImGui::TreeNode("Textures")){
                for(int i = 0; i < recourseTextures.size(); i++){
                    ImGui::Text("%s", recourseTextures[i].c_str());
                }
                ImGui::TreePop();
            }

            if(ImGui::TreeNode("Models")){
                for(int i = 0; i < recourseModels.size(); i++){
                    ImGui::Text("%s", recourseModels[i].c_str());
                }
                ImGui::TreePop();
            }

            ImGui::End();
        }
    }

    void EngineUI::drawRenderStats(EngineScene::Scene *scene, float fps, ECS* ecs){
        if(m_showRenderStats){
            int triangleCount = 0;

            ecs->foreach<MeshComponent>([&triangleCount](MeshComponent* m){
                triangleCount += m->mesh->indexCount /3;
            });

            ImGui::Begin("Render Stats");
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.2fms", 1000.f / fps);
            ImGui::Text("Draw Calls: %d", drawCallCountLastFrame);
            ImGui::Text("Triangle Count: %d", triangleCount);
            ImGui::Checkbox("Wireframe Mode", &wireFrameModeEnabled);
            ImGui::End();
        }
    }

    void EngineUI::drawScenes(EngineScene::SceneManager *sceneManager){
        if(m_showScenesWindow){
            ImGui::Begin("Scenes");
            ImGui::Checkbox("Scene Inspector", &m_showSceneInspector);
            for(auto& scene : sceneManager->getScenes()){
                ImGui::PushID(scene);
                bool is_scene_selected = (selectedScene == scene);
                if(ImGui::Selectable(scene->name.c_str(), is_scene_selected)){
                    if(selectedScene) selectedScene->selected = false;
                    selectedScene = scene;
                    selectedScene->selected = true;

                    m_showSceneInspector = true;

                    sceneManager->changeScenes(selectedScene->getIndex());
                }
                ImGui::PopID();
            }        
            ImGui::End();
        }
    }

}
