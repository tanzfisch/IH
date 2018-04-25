#ifndef __ISLANDHOPPER__
#define __ISLANDHOPPER__

#include <Igor.h>
#include <iWindow.h>
#include <iView.h>
#include <iTimerHandle.h>
#include <iModelResourceFactory.h>
#include <iKeyboard.h>
#include <iStatisticsVisualizer.h>
#include <iPerlinNoise.h>
#include <iLSystem.h>
#include <iVoxelTerrain.h>
using namespace Igor;

#include <iaMatrix.h>
using namespace IgorAux;

namespace Igor
{
	class iScene;
	class iNodeTransform;
	class iNodeLight;
    class iTextureFont;
    class iVoxelData;
    class iContouringCubes;
    class iMeshBuilder;
    class iMesh;
    class iTargetMaterial;
    class iNodeTransformControl;
    class iNodeLODTrigger;
    class iTexture;
	class iPhysicsBody;
}

class Enemy;

class IslandHopper
{

public:

    IslandHopper();
	virtual ~IslandHopper();

	void run();

private:

    /*! visualize statistics
    */
    iStatisticsVisualizer _statisticsVisualizer;

	iPerlinNoise _perlinNoise;

    bool _loading = true;
    bool _activeControls = false;
	bool _wireframe = false;
	bool _showMinimap = false;

    iWindow _window;
    iView _view;
    iView _viewOrtho;

    uint64 _playerID = 0;
    uint64 _bossID = 0;

    iTextureFont* _font = nullptr;

    iScene* _scene = nullptr;

    uint64 _toolSize = 10;
    uint8 _toolDensity = 0;

    iaVector2f _mouseDelta;
    iaVector3f _weaponPos;

    iNodeTransform* _lightTranslate = nullptr;
    iNodeTransform* _lightRotate = nullptr;
    iNodeLight* _lightNode = nullptr;

    iVoxelTerrain* _voxelTerrain = nullptr;

    iPixmap* _heightMap = nullptr;
    shared_ptr<iTexture> _minimap;
    
	uint64 _materialWithTextureAndBlending = 0;
	uint64 _materialSolid = 0;
	uint64 _octreeMaterial = 0;
	uint64 _materialSkyBox = 0;

    uint64 _taskFlushModels = 0; 
    uint64 _taskFlushTextures = 0;

	vector<iSpheref> _holes;

    void generateVoxelData(iVoxelBlockInfo* voxelBlockInfo);

    void onKeyPressed(iKeyCode key);
    void onKeyReleased(iKeyCode key);

    void onWindowClosed();
    void onWindowResized(int32 clientWidth, int32 clientHeight);

    void onMouseMoved(const iaVector2i& from, const iaVector2i& to, iWindow* _window);

    //void onVoxelDataGenerated(const iaVector3I& min, const iaVector3I& max);
    
    void onMouseUp(iKeyCode key);
    void onMouseDown(iKeyCode key);
    void onMouseWheel(int d);

    void handleMouse();

    void deinit();
    void init();

	void onRender();
    void onHandle();
    void onRenderOrtho();

    void registerHandles();
    void unregisterHandles();
    void initViews();
    void initScene();
    void initPlayer();

    void initVoxelData();
    void deinitVoxelData();

};

#endif
