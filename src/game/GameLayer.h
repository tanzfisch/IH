#ifndef __GAMELAYER_H__
#define __GAMELAYER_H__

#include "Plane.h"

class HUDLayer;

class GameLayer : public iLayer
{

public:
    /*! init members
    */
    GameLayer(iWindow *window, int32 zIndex);

    /*! does nothing
    */
    virtual ~GameLayer() = default;

    /*! sets the hud layer so it's content can be controled by the game layer

    \param hudLayer the hud layer
    */
    void setHud(HUDLayer *hudLayer);

private:
    /*! the main view
    */
    iView _view;

    /*! main scene
    */
    iScene *_scene = nullptr;

    /*! handle to hud layer
    */
    HUDLayer *_hudLayer = nullptr;

    /*! noise function to generate terrain
    */
    iPerlinNoise _perlinNoise;

    /*! flag to control wether or not controls are active
    */
    bool _activeControls = true;

    /*! light position
    */
    iNodeTransform *_lightTranslate = nullptr;

    /*! light direction
    */
    iNodeTransform *_lightRotate = nullptr;

    /*! light
    */
    iNodeLight *_lightNode = nullptr;

    /*! voxel terrain
    */
    iVoxelTerrain *_voxelTerrain = nullptr;

    Plane *_plane = nullptr;

    /*! instancing material for vegetation etc
    */
    iMaterialID _materialWithInstancing = iMaterial::INVALID_MATERIAL_ID;

    /*! skybox material
    */
    iMaterialID _materialSkyBox = iMaterial::INVALID_MATERIAL_ID;

    /*! flush models task
    */
    iTaskID _taskFlushModels = iTask::INVALID_TASK_ID;

    /*!  flush textures task
    */
    iTaskID _taskFlushTextures = iTask::INVALID_TASK_ID;

    void playerCrashed();
    void playerLanded();

    void onVoxelDataGenerated(iVoxelBlockPropsInfo voxelBlockPropsInfo);
    void onGenerateVoxelData(iVoxelBlockInfo *voxelBlockInfo);

    void onRenderOrtho();

    void registerHandles();
    void unregisterHandles();
    void initViews();
    void initScene();
    void initPlayer();

    void initVoxelData();
    void deinitVoxelData();

    /*! called when model was loaded
    */
    void onModelReady(iNodeID nodeID);

    /*! init
	*/
    void onInit() override;

    /*! deinit
	*/
    void onDeinit() override;

    /*! called on application pre draw event
    */
    void onPreDraw() override;

    /*! called on any other event
    */
    void onEvent(iEvent &event) override;

    /*! called when key was pressed

    \param event the event to handle
    */
    bool onKeyDown(iEventKeyDown &event);

    /*! called when key was released

    \param event the event to handle
    */
    bool onKeyUp(iEventKeyUp &event);

    /*! handles mouse key down event

    \param event the mouse key down event
    \returns true if consumed
    */
    bool onMouseKeyDownEvent(iEventMouseKeyDown &event);

    /*! handles mouse key up event

    \param event the mouse key up event
    \returns true if consumed
    */
    bool onMouseKeyUpEvent(iEventMouseKeyUp &event);

    /*! handles mouse move event

    \param event the mouse move event
    \returns true if consumed
    */
    bool onMouseMoveEvent(iEventMouseMove &event);

    /*! handles mouse wheel event

    \param event the mouse wheel event
    \returns true if consumed
    */
    bool onMouseWheelEvent(iEventMouseWheel &event);
};

#endif // __GAMELAYER_H__
