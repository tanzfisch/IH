#ifndef __ISLANDHOPPER__
#define __ISLANDHOPPER__

#include <igor/igor.h>
#include <igor/os/iWindow.h>
#include <igor/graphics/iView.h>
#include <igor/os/iTimerHandle.h>
#include <igor/resources/model/iModelResourceFactory.h>
#include <igor/os/iKeyboard.h>
#include <igor/resources/profiler/iProfilerVisualizer.h>
#include <igor/graphics/generation/iPerlinNoise.h>
#include <igor/graphics/generation/iLSystem.h>
#include <igor/graphics/terrain/iVoxelTerrain.h>
using namespace Igor;

#include <iaux/math/iaMatrix.h>
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

class Plane;

class IslandHopper
{

public:

    IslandHopper();
	virtual ~IslandHopper();

	void run();

private:

	Plane* _plane = nullptr;

    /*! visualize statistics
    */
	iProfilerVisualizer _profiler;

	iPerlinNoise _perlinNoise;

    bool _loading = true;
    bool _activeControls = false;
	bool _wireframe = false;

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

	uint64 _materialWithTextureAndBlending = 0;
	uint64 _octreeMaterial = 0;
	uint64 _materialSkyBox = 0;

    uint64 _taskFlushModels = 0; 
    uint64 _taskFlushTextures = 0;

    void onVoxelDataGenerated(iVoxelBlockPropsInfo voxelBlockPropsInfo);
    void onGenerateVoxelData(iVoxelBlockInfo* voxelBlockInfo);

    void onKeyPressed(iKeyCode key);
    void onKeyReleased(iKeyCode key);

    void onWindowClosed();
    void onWindowResized(int32 clientWidth, int32 clientHeight);

    void onMouseMoved(const iaVector2i& from, const iaVector2i& to, iWindow* _window);
        
    void onMouseUp(iKeyCode key);
    void onMouseDown(iKeyCode key);
    void onMouseWheel(int d);

    void deinit();
    void init();

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
