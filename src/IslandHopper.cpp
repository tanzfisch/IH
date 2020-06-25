#include "IslandHopper.h"

#include <igor/resources/material/iMaterial.h>
#include <igor/resources/material/iMaterialGroup.h>
#include <igor/scene/traversal/iNodeVisitorPrintTree.h>
#include <igor/threading/iTaskManager.h>
#include <igor/scene/nodes/iNodeSkyBox.h>
#include <igor/scene/nodes/iNodeLight.h>
#include <igor/scene/nodes/iNodeCamera.h>
#include <igor/scene/nodes/iNodeModel.h>
#include <igor/scene/nodes/iNodeTransform.h>
#include <igor/scene/nodes/iNodeManager.h>
#include <igor/scene/nodes/iNodeLODTrigger.h>
#include <igor/scene/nodes/iNodeLODSwitch.h>
#include <igor/scene/nodes/iNodeWater.h>
#include <igor/scene/nodes/iNodeParticleSystem.h>
#include <igor/scene/nodes/iNodeEmitter.h>
#include <igor/scene/nodes/iNodeMesh.h>
#include <igor/graphics/iRenderer.h>
#include <igor/system/iApplication.h>
#include <igor/scene/iSceneFactory.h>
#include <igor/scene/iScene.h>
#include <igor/system/iMouse.h>
#include <igor/system/iTimer.h>
#include <igor/resources/texture/iTextureFont.h>
#include <igor/threading/tasks/iTaskFlushModels.h>
#include <igor/threading/tasks/iTaskFlushTextures.h>
#include <igor/resources/material/iMaterialResourceFactory.h>
#include <igor/terrain/data/iVoxelData.h>
#include <igor/resources/mesh/iMeshBuilder.h>
#include <igor/resources/texture/iTextureResourceFactory.h>
#include <igor/resources/texture/iPixmap.h>
#include <igor/resources/material/iTargetMaterial.h>
#include <igor/scene/octree/iOctree.h>
#include <igor/physics/iPhysics.h>
#include <igor/physics/iPhysicsMaterialCombo.h>
#include <igor/physics/iPhysicsBody.h>

using namespace igor;

#include <iaux/data/iaConvert.h>
#include <iaux/system/iaConsole.h>
#include <iaux/math/iaVector3.h>
using namespace iaux;

#include "Plane.h"

// #define SIN_WAVE_TERRAIN
#define USE_WATER

static const float64 s_waterLevel = 10000;
static const float64 s_treeLine = 10500;

IslandHopper::IslandHopper()
{
	init();
}

IslandHopper::~IslandHopper()
{
	deinit();
}

void IslandHopper::registerHandles()
{
	iKeyboard::getInstance().registerKeyDownDelegate(iKeyDownDelegate(this, &IslandHopper::onKeyPressed));
	iKeyboard::getInstance().registerKeyUpDelegate(iKeyUpDelegate(this, &IslandHopper::onKeyReleased));
	iMouse::getInstance().registerMouseKeyDownDelegate(iMouseKeyDownDelegate(this, &IslandHopper::onMouseDown));
	iMouse::getInstance().registerMouseKeyUpDelegate(iMouseKeyUpDelegate(this, &IslandHopper::onMouseUp));
	iMouse::getInstance().registerMouseWheelDelegate(iMouseWheelDelegate(this, &IslandHopper::onMouseWheel));
	iMouse::getInstance().registerMouseMoveFullDelegate(iMouseMoveFullDelegate(this, &IslandHopper::onMouseMoved));

	_window.registerWindowResizeDelegate(WindowResizeDelegate(this, &IslandHopper::onWindowResized));
	_window.registerWindowCloseDelegate(WindowCloseDelegate(this, &IslandHopper::onWindowClosed));

	iApplication::getInstance().registerApplicationPreDrawHandleDelegate(iPreDrawDelegate(this, &IslandHopper::onHandle));
}

void IslandHopper::unregisterHandles()
{
	iApplication::getInstance().unregisterApplicationPreDrawHandleDelegate(iPreDrawDelegate(this, &IslandHopper::onHandle));

	_window.unregisterWindowResizeDelegate(WindowResizeDelegate(this, &IslandHopper::onWindowResized));
	_window.unregisterWindowCloseDelegate(WindowCloseDelegate(this, &IslandHopper::onWindowClosed));

	iMouse::getInstance().unregisterMouseMoveFullDelegate(iMouseMoveFullDelegate(this, &IslandHopper::onMouseMoved));
	iMouse::getInstance().unregisterMouseKeyDownDelegate(iMouseKeyDownDelegate(this, &IslandHopper::onMouseDown));
	iMouse::getInstance().unregisterMouseKeyUpDelegate(iMouseKeyUpDelegate(this, &IslandHopper::onMouseUp));
	iMouse::getInstance().unregisterMouseWheelDelegate(iMouseWheelDelegate(this, &IslandHopper::onMouseWheel));

	iKeyboard::getInstance().unregisterKeyUpDelegate(iKeyUpDelegate(this, &IslandHopper::onKeyReleased));
	iKeyboard::getInstance().unregisterKeyDownDelegate(iKeyDownDelegate(this, &IslandHopper::onKeyPressed));
}

void IslandHopper::initViews()
{
	_view.setClearColor(iaColor4f(0.0f, 0.0f, 0.0f, 1.0f));
	_view.setPerspective(60);
	_view.setClipPlanes(1.0f, 80000.f);
	_view.setName("3d");

	_viewOrtho.setClearColor(false);
	_viewOrtho.setClearDepth(false);
	_viewOrtho.setClipPlanes(0.1, 100);
	_viewOrtho.registerRenderDelegate(iDrawDelegate(this, &IslandHopper::onRenderOrtho));
	_viewOrtho.setName("ortho");

	_window.setTitle("IslandHopper");
	_window.addView(&_view);
	_window.addView(&_viewOrtho);
#if 1
	_window.setClientSize(1280, 600);
	_window.setCentered();
#else
	_window.setSizeByDesktop();
	_window.setFullscreen();
#endif
	_window.open();

	iMouse::getInstance().showCursor(true);

	_viewOrtho.setOrthogonal(0, _window.getClientWidth(), _window.getClientHeight(), 0);
}

void IslandHopper::initScene()
{
	_scene = iSceneFactory::getInstance().createScene();
	_view.setScene(_scene);

	_lightTranslate = iNodeManager::getInstance().createNode<iNodeTransform>();
	_lightTranslate->translate(100, 100, 100);
	_lightRotate = iNodeManager::getInstance().createNode<iNodeTransform>();
	_lightNode = iNodeManager::getInstance().createNode<iNodeLight>();
	_lightNode->setAmbient(iaColor4f(0.7f, 0.7f, 0.7f, 1.0f));
	_lightNode->setDiffuse(iaColor4f(1.0f, 0.9f, 0.8f, 1.0f));
	_lightNode->setSpecular(iaColor4f(1.0f, 0.9f, 0.87f, 1.0f));

	_lightRotate->insertNode(_lightTranslate);
	_lightTranslate->insertNode(_lightNode);
	_scene->getRoot()->insertNode(_lightRotate);

	// reate a sky box and add it to scene
	iNodeSkyBox *skyBoxNode = iNodeManager::getInstance().createNode<iNodeSkyBox>();
	skyBoxNode->setTextures(
		iTextureResourceFactory::getInstance().requestFile("skybox_day/front.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().requestFile("skybox_day/back.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().requestFile("skybox_day/left.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().requestFile("skybox_day/right.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().requestFile("skybox_day/top.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().requestFile("skybox_day/bottom.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp));
	skyBoxNode->setTextureScale(1);
	// create a sky box material
	_materialSkyBox = iMaterialResourceFactory::getInstance().createMaterial();
	auto material = iMaterialResourceFactory::getInstance().getMaterial(_materialSkyBox);
	material->setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
	material->setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
	material->setOrder(iMaterial::RENDER_ORDER_MIN);
	material->setName("SkyBox");
	// and set the sky box material
	skyBoxNode->setMaterial(_materialSkyBox);
	// insert sky box to scene
	_scene->getRoot()->insertNode(skyBoxNode);

	// TODO just provisorical water
	// create a water plane and add it to scene

#ifdef USE_WATER
	for (int i = 0; i < 10; ++i) // todo just for the look give water a depth
	{
		iNodeWater *waterNode = iNodeManager::getInstance().createNode<iNodeWater>();
		waterNode->setWaterPosition(s_waterLevel - i * 1);

		if (i == 9)
		{
			waterNode->setAmbient(iaColor4f(0, 0.4 - static_cast<float64>(i) * 0.01, 0.95 - static_cast<float64>(i) * 0.08, 1.0));
		}
		else
		{
			waterNode->setAmbient(iaColor4f(0, 0.4 - static_cast<float64>(i) * 0.01, 0.95 - static_cast<float64>(i) * 0.08, 0.2 + static_cast<float64>(i) * 0.01));
		}

		// create a water material
		uint64 materialWater = iMaterialResourceFactory::getInstance().createMaterial();
		material = iMaterialResourceFactory::getInstance().getMaterial(materialWater);
		material->setOrder(iMaterial::RENDER_ORDER_MAX - i);
		material->setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
		material->setRenderState(iRenderState::Blend, iRenderStateValue::On);
		material->setName("WaterPlane");

		// and set the water material
		waterNode->setMaterial(materialWater);
		// insert sky box to scene
		_scene->getRoot()->insertNode(waterNode);
	}
#endif
}

void IslandHopper::initPlayer()
{
	if (_plane == nullptr)
	{
		_plane = new Plane(_scene, &_view);
		_plane->_planeCrashedEvent.append(PlaneCrashedDelegate(this, &IslandHopper::playerCrashed));
		_plane->_planeLandedEvent.append(PlaneLandedDelegate(this, &IslandHopper::playerLanded));
	}

	iaMatrixd matrix;
	matrix.translate(759846, 10100, 381272);
	_plane->setMatrix(matrix);
}

void IslandHopper::init()
{
	_perlinNoise.generateBase(313373);

	initViews();
	initScene();

	initPlayer();
	initVoxelData();

	// load a font
	_font = new iTextureFont("StandardFont.png");

	// configure profiler
	_profiler.setVerbosity(iProfilerVerbosity::FPSOnly);

	// set up octree debug material
	_octreeMaterial = iMaterialResourceFactory::getInstance().createMaterial("Octree");
	iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->setRenderState(iRenderState::Blend, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->setRenderState(iRenderState::Wireframe, iRenderStateValue::On);

	// setup some materials
	_materialWithTextureAndBlending = iMaterialResourceFactory::getInstance().createMaterial("TextureAndBlending");
	iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->setRenderState(iRenderState::Blend, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);

	// launch resource handlers
	_taskFlushModels = iTaskManager::getInstance().addTask(new iTaskFlushModels(&_window));
	_taskFlushTextures = iTaskManager::getInstance().addTask(new iTaskFlushTextures(&_window));

	registerHandles();

	// start physics
	iPhysics::getInstance().start();
}

void IslandHopper::initVoxelData()
{
	// it's a flat landscape so we can limit the discovery in the vertical axis
	iaVector3I maxDiscovery(1000000, 3, 1000000);

	_voxelTerrain = new iVoxelTerrain(iVoxelTerrainGenerateDelegate(this, &IslandHopper::onGenerateVoxelData),
									  iVoxelTerrainPlacePropsDelegate(this, &IslandHopper::onVoxelDataGenerated), 11, 4, &maxDiscovery);

	iTargetMaterial *targetMaterial = _voxelTerrain->getTargetMaterial();
	targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("grass.png"), 0);
	targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("dirt.png"), 1);
	targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("rock.png"), 2);
	targetMaterial->setAmbient(iaColor3f(0.3f, 0.3f, 0.3f));
	targetMaterial->setDiffuse(iaColor3f(0.8f, 0.8f, 0.8f));
	targetMaterial->setSpecular(iaColor3f(1.0f, 1.0f, 1.0f));
	targetMaterial->setEmissive(iaColor3f(0.0f, 0.0f, 0.0f));
	targetMaterial->setShininess(1000.0f);

	uint64 materialID = iMaterialResourceFactory::getInstance().createMaterial("TerrainMaterial");
	auto material = iMaterialResourceFactory::getInstance().getMaterial(materialID);
	material->addShaderSource("terrain.vert", iShaderObjectType::Vertex);
	material->addShaderSource("terrain_directional_light.frag", iShaderObjectType::Fragment);
	material->compileShader();

	_voxelTerrain->setMaterialID(materialID);
	_voxelTerrain->setScene(_scene);

	_voxelTerrain->setLODTrigger(_plane->getLODTriggerID());
}

__IGOR_INLINE__ float64 metaballFunction(const iaVector3f &metaballPos, const iaVector3f &checkPos)
{
	return 1.0 / ((checkPos._x - metaballPos._x) * (checkPos._x - metaballPos._x) + (checkPos._y - metaballPos._y) * (checkPos._y - metaballPos._y) + (checkPos._z - metaballPos._z) * (checkPos._z - metaballPos._z));
}

void IslandHopper::onVoxelDataGenerated(iVoxelBlockPropsInfo voxelBlockPropsInfo)
{
	// skip this block
	if (voxelBlockPropsInfo._max._y < s_waterLevel ||
		voxelBlockPropsInfo._min._y > s_treeLine)
	{
		return;
	}

	iaVector3I diff;
	diff = voxelBlockPropsInfo._max;
	diff -= voxelBlockPropsInfo._min;

	iaVector3d offset = voxelBlockPropsInfo._min.convert<float64>();
	iaRandomNumberGeneratoru rand(offset._x + offset._y + offset._z);

	for (int i = 0; i < 10; ++i)
	{
		iaVector3d from(rand.getNext() % diff._x + offset._x, s_treeLine * 2, rand.getNext() % diff._z + offset._z);
		iaVector3d to = from;
		to._y = s_waterLevel;

		iaVector3I outside, inside;

		if (_voxelTerrain->castRay(iaVector3I(from._x, from._y, from._z), iaVector3I(to._x, to._y, to._z), outside, inside))
		{
			iaVector3d pos(outside._x, outside._y, outside._z);

			iNodeTransform *transformTree = iNodeManager::getInstance().createNode<iNodeTransform>();
			transformTree->translate(pos);
			iNodeModel *tree = iNodeManager::getInstance().createNode<iNodeModel>();
			tree->setModel("tree.ompf");
			iNodeLODSwitch *lodSwitch = iNodeManager::getInstance().createNode<iNodeLODSwitch>();
			lodSwitch->insertNode(tree);
			lodSwitch->addTrigger(_plane->getLODTriggerID());
			lodSwitch->setThresholds(tree, 0, 100);
			transformTree->insertNode(lodSwitch);
			_scene->getRoot()->insertNode(transformTree);
		}
	}
}

void IslandHopper::onGenerateVoxelData(iVoxelBlockInfo *voxelBlockInfo)
{
	uint32 lodFactor = static_cast<uint32>(pow(2, voxelBlockInfo->_lod));
	iVoxelData *voxelData = voxelBlockInfo->_voxelData;
	iaVector3I position = voxelBlockInfo->_positionInLOD;
	position *= (32 * lodFactor);
	iaVector3f &lodOffset = voxelBlockInfo->_lodOffset;
	uint64 &size = voxelBlockInfo->_size;

	const float64 from = 0.35;
	const float64 to = 0.36;
	float64 factor = 1.0 / (to - from);

	if (voxelData != nullptr) // TODO remove
	{
		voxelData->initData(size, size, size);

		for (int64 x = 0; x < voxelData->getWidth(); ++x)
		{
			for (int64 z = 0; z < voxelData->getDepth(); ++z)
			{
				iaVector3f pos(x * lodFactor + position._x + lodOffset._x, 0, z * lodFactor + position._z + lodOffset._z);
				float64 height;

#ifdef SIN_WAVE_TERRAIN
				height = s_waterLevel + (sin(pos._x * 0.125) + sin(pos._z * 0.125)) * 10.0;
#else
				float64 contour = _perlinNoise.getValue(iaVector3d(pos._x * 0.0001, 0, pos._z * 0.0001), 3, 0.6);
				contour -= 0.7;

				if (contour > 0.0)
				{
					contour *= 3;
				}

				float64 noise = _perlinNoise.getValue(iaVector3d(pos._x * 0.001, 0, pos._z * 0.001), 7, 0.55) * 0.15;
				noise += contour;

				if (noise < 0.0)
				{
					noise *= 0.5;
				}
				else
				{
					noise *= 2.0;
				}

				noise += 0.0025;

				if (noise < 0.0)
				{
					noise *= 2.0;
				}

				noise += 0.0025;

				height = (noise * 2000) + s_waterLevel + 200;

				if (height < s_waterLevel - 100)
				{
					height = s_waterLevel - 100;
				}
#endif

				float64 transdiff = height - static_cast<float64>(position._y) - lodOffset._y;
				if (transdiff > 0 && transdiff <= voxelData->getHeight() * lodFactor)
				{
					voxelBlockInfo->_transition = true;
				}

				float64 heightDiff = (transdiff / static_cast<float64>(lodFactor));
				if (heightDiff > 0)
				{
					if (heightDiff > size)
					{
						heightDiff = size;
					}

					int64 diffi = static_cast<uint64>(heightDiff);
					if (diffi > 0)
					{
						voxelData->setVoxelPole(iaVector3I(x, 0, z), diffi, 128);
					}

					if (diffi < voxelData->getHeight())
					{
						heightDiff -= static_cast<float64>(diffi);
						voxelData->setVoxelDensity(iaVector3I(x, diffi, z), static_cast<uint8>((heightDiff * 254.0) + 1.0));
					}
				}

				float64 cavelikeliness = _perlinNoise.getValue(iaVector3d(pos._x * 0.0001 + 12345, 0, pos._z * 0.0001 + 12345), 3, 0.6);
				cavelikeliness -= 0.5;
				if (cavelikeliness > 0.0)
				{
					cavelikeliness *= 1.0 / 0.5;
					cavelikeliness *= 5;
				}

				if (cavelikeliness > 0)
				{
					for (int64 y = 0; y < voxelData->getHeight(); ++y)
					{
						pos._y = y * lodFactor + position._y + lodOffset._y;

						if (pos._y > s_waterLevel &&
							pos._y > height - 300 &&
							pos._y < height + 10)
						{
							float64 onoff = _perlinNoise.getValue(iaVector3d(pos._x * 0.01, pos._y * 0.01, pos._z * 0.01), 5, 0.5);

							float64 diff = (pos._y - (height - 50)) / 50.0;
							onoff += (1.0 - diff) * 0.1;

							if (onoff <= from)
							{
								if (onoff >= to)
								{
									float64 gradient = 1.0 - ((onoff - from) * factor);
									voxelData->setVoxelDensity(iaVector3I(x, y, z), static_cast<uint8>((gradient * 254.0) + 1.0));
									voxelBlockInfo->_transition = true;
								}
								else
								{
									voxelData->setVoxelDensity(iaVector3I(x, y, z), 0);
									voxelBlockInfo->_transition = true;
								}
							}
						}
					}
				}
			}
		}
	}
}

void IslandHopper::deinit()
{
	unregisterHandles();

	iTaskManager::getInstance().abortTask(_taskFlushModels);
	iTaskManager::getInstance().abortTask(_taskFlushTextures);

	deinitVoxelData();

	iSceneFactory::getInstance().destroyScene(_scene);

	_viewOrtho.unregisterRenderDelegate(iDrawDelegate(this, &IslandHopper::onRenderOrtho));

	_window.close();
	_window.removeView(&_view);
	_window.removeView(&_viewOrtho);

	if (_font != nullptr)
	{
		delete _font;
		_font = nullptr;
	}

	if (_plane != nullptr)
	{
		delete _plane;
		_plane = nullptr;
	}
}

void IslandHopper::onKeyPressed(iKeyCode key)
{
	if (_activeControls)
	{
		switch (key)
		{
		case iKeyCode::A:
			_plane->startRollLeft();
			break;

		case iKeyCode::D:
			_plane->startRollRight();
			break;

		case iKeyCode::S:
			_plane->startRollUp();
			break;

		case iKeyCode::W:
			_plane->startRollDown();
			break;
		}
	}

	switch (key)
	{
	case iKeyCode::ESC:
		iApplication::getInstance().stop();
		break;

	case iKeyCode::F3:
	{
		iProfilerVerbosity level = _profiler.getVerbosity();

		if (level == iProfilerVerbosity::All)
		{
			level = iProfilerVerbosity::None;
		}
		else
		{
			int value = static_cast<int>(level);
			value++;
			level = static_cast<iProfilerVerbosity>(value);
		}

		_profiler.setVerbosity(level);
	}
	break;

	case iKeyCode::Alt:
		_activeControls = !_activeControls;
		iMouse::getInstance().showCursor(!_activeControls);
		break;
	}
}

void IslandHopper::onKeyReleased(iKeyCode key)
{
	if (_activeControls)
	{
		switch (key)
		{

		case iKeyCode::OEMPlus:
			_plane->setThrustLevel(_plane->getThrustLevel() + 0.1);
			break;

		case iKeyCode::OEM2:
			if (_plane->getThrustLevel() >= 0.3)
			{
				_plane->setThrustLevel(_plane->getThrustLevel() - 0.1);
			}
			break;

		case iKeyCode::A:
			_plane->stopRollLeft();
			break;

		case iKeyCode::D:
			_plane->stopRollRight();
			break;

		case iKeyCode::S:
			_plane->stopRollUp();
			break;

		case iKeyCode::W:
			_plane->stopRollDown();
			break;

		case iKeyCode::F12:
		{
			_wireframe = !_wireframe;

			uint64 terrainMaterial = _voxelTerrain->getMaterialID();

			if (_wireframe)
			{
				iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->setRenderState(iRenderState::CullFace, iRenderStateValue::Off);
				iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->setRenderState(iRenderState::Wireframe, iRenderStateValue::On);
			}
			else
			{
				iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->setRenderState(iRenderState::CullFace, iRenderStateValue::On);
				iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->setRenderState(iRenderState::Wireframe, iRenderStateValue::Off);
			}
		}
		break;
		}
	}
}

void IslandHopper::onMouseWheel(int d)
{
	if (_activeControls)
	{
		if (d > 0)
		{
			_toolSize += 1;
			if (_toolSize > 20)
			{
				_toolSize = 20;
			}
		}
		else
		{
			_toolSize -= 5;

			if (_toolSize < 5)
			{
				_toolSize = 5;
			}
		}
	}
}

void IslandHopper::playerCrashed()
{
	con_endl("crash");
	initPlayer();
}

void IslandHopper::playerLanded()
{
	con_endl("land");
}

void IslandHopper::onMouseMoved(const iaVector2i &from, const iaVector2i &to, iWindow *_window)
{
	if (_activeControls)
	{
		_mouseDelta.set(to._x - from._x, to._y - from._y);
		iMouse::getInstance().setCenter();
	}
}

void IslandHopper::onMouseDown(iKeyCode key)
{
}

void IslandHopper::onMouseUp(iKeyCode key)
{
}

void IslandHopper::onWindowClosed()
{
	iApplication::getInstance().stop();
}

void IslandHopper::onWindowResized(int32 clientWidth, int32 clientHeight)
{
	_viewOrtho.setOrthogonal(0, static_cast<float32>(clientWidth), static_cast<float32>(clientHeight), 0);
}

void IslandHopper::deinitVoxelData()
{
	if (_voxelTerrain != nullptr)
	{
		delete _voxelTerrain;
		_voxelTerrain = nullptr;
	}
}

void IslandHopper::onHandle()
{
	if (_loading)
	{
		if (iTimer::getInstance().getApplicationTime() > 5000 &&
			iTaskManager::getInstance().getQueuedRegularTaskCount() < 4)
		{
			_loading = false;
			_activeControls = true;
			_mouseDelta.set(0, 0);
		}
	}
}

void IslandHopper::onRenderOrtho()
{
	iaMatrixd matrix;
	iRenderer::getInstance().setViewMatrix(matrix);
	matrix.translate(0, 0, -30);
	iRenderer::getInstance().setModelMatrix(matrix);
	iRenderer::getInstance().setMaterial(_materialWithTextureAndBlending);
	iRenderer::getInstance().setFont(_font);

	if (_loading)
	{
		iRenderer::getInstance().setColor(iaColor4f(0, 0, 0, 1));
		iRenderer::getInstance().drawRectangle(0, 0, _window.getClientWidth(), _window.getClientHeight());

		iRenderer::getInstance().setColor(iaColor4f(0, 0, 1, 1));
		iRenderer::getInstance().setFontSize(40.0f);
		iRenderer::getInstance().drawString(_window.getClientWidth() * 0.5, _window.getClientHeight() * 0.5, "generating level ...", iHorizontalAlignment::Center, iVerticalAlignment::Center);
	}
	else
	{
		iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
		iRenderer::getInstance().setFontSize(15.0f);
		iaString altitude = "Altitude: ";
		altitude += iaString::toString(_plane->getAltitude() - s_waterLevel);
		iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.01, altitude);

		iaString thrustLevel = "Thrust: ";
		thrustLevel += iaString::toString(_plane->getThrustLevel() * 100.0);
		iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.04, thrustLevel);

		iaString velocity = "Velocity: ";
		velocity += iaString::toString(_plane->getVelocity());
		iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.07, velocity);
	}

	_profiler.draw(&_window, _font, iaColor4f(1.0, 1.0, 1.0, 1));

	iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
}

void IslandHopper::run()
{
	iApplication::getInstance().run();
}
