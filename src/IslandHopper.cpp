#include "IslandHopper.h"

#include <iaConsole.h>
using namespace IgorAux;

#include <iMaterial.h>
#include <iMaterialGroup.h>
#include <iNodeVisitorPrintTree.h>
#include <iTaskManager.h>
#include <iNodeSkyBox.h>
#include <iNodeLight.h>
#include <iNodeCamera.h>
#include <iNodeModel.h> 
#include <iNodeTransform.h>
#include <iRenderer.h>
#include <iApplication.h>
#include <iSceneFactory.h>
#include <iScene.h>
#include <iNodeManager.h>
#include <iMouse.h>
#include <iTimer.h>
#include <iTextureFont.h>
#include <iTaskFlushModels.h>
#include <iTaskFlushTextures.h>
#include <iMaterialResourceFactory.h>
#include <iVoxelData.h>
#include <iMeshBuilder.h>
#include <iaVector3.h>
#include <iContouringCubes.h>
#include <iTextureResourceFactory.h>
#include <iPixmap.h>
#include <iTargetMaterial.h>
#include <iNodeLODTrigger.h>
#include <iNodeLODSwitch.h>
#include <iOctree.h>
#include <iPhysics.h>
#include <iPhysicsMaterialCombo.h>
#include <iNodeWater.h>
#include <iNodeParticleSystem.h>
#include <iNodeEmitter.h>
#include <iPhysicsBody.h>
#include <iNodeMesh.h>
using namespace Igor;

#include <iaConvert.h>
using namespace IgorAux;

#include "Plane.h"

// #define SIN_WAVE_TERRAIN
#define USE_WATER

const float64 waterLevel = 10000;

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

	iApplication::getInstance().registerApplicationPreDrawHandleDelegate(iApplicationPreDrawHandleDelegate(this, &IslandHopper::onHandle));
}

void IslandHopper::unregisterHandles()
{
	iApplication::getInstance().unregisterApplicationPreDrawHandleDelegate(iApplicationPreDrawHandleDelegate(this, &IslandHopper::onHandle));

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
	_viewOrtho.registerRenderDelegate(RenderDelegate(this, &IslandHopper::onRenderOrtho));
	_viewOrtho.setName("ortho");

	_window.setTitle("IslandHopper");
	_window.addView(&_view);
	_window.addView(&_viewOrtho);
#if 1
	_window.setClientSize(1280, 600);
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
	iNodeSkyBox* skyBoxNode = iNodeManager::getInstance().createNode<iNodeSkyBox>();
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
		iNodeWater* waterNode = iNodeManager::getInstance().createNode<iNodeWater>();
		waterNode->setWaterPosition(waterLevel - i * 1);

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
	iaMatrixd matrix;
	//matrix.translate(730000, 4800, 530000);
//    matrix.translate(759669, 4817, 381392);
	//matrix.translate(759844, 4661, 381278);
	matrix.translate(759846, 10100, 381272);
	_plane = new Plane(_scene, &_view, matrix);
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

	iTargetMaterial* targetMaterial = _voxelTerrain->getTargetMaterial();
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

__IGOR_INLINE__ float64 metaballFunction(const iaVector3f& metaballPos, const iaVector3f& checkPos)
{
	return 1.0 / ((checkPos._x - metaballPos._x) * (checkPos._x - metaballPos._x) + (checkPos._y - metaballPos._y) * (checkPos._y - metaballPos._y) + (checkPos._z - metaballPos._z) * (checkPos._z - metaballPos._z));
}

void IslandHopper::onVoxelDataGenerated(iVoxelBlockPropsInfo voxelBlockPropsInfo)
{
}

void IslandHopper::onGenerateVoxelData(iVoxelBlockInfo* voxelBlockInfo)
{
	uint32 lodFactor = static_cast<uint32>(pow(2, voxelBlockInfo->_lod));
	iVoxelData* voxelData = voxelBlockInfo->_voxelData;
	iaVector3I position = voxelBlockInfo->_positionInLOD;
	position *= (32 * lodFactor);
	iaVector3f& lodOffset = voxelBlockInfo->_lodOffset;
	uint64& size = voxelBlockInfo->_size;

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
				height = waterLevel + (sin(pos._x * 0.125) + sin(pos._z * 0.125)) * 10.0;
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

				height = (noise * 2000) + waterLevel + 200;

				if (height < waterLevel - 100)
				{
					height = waterLevel - 100;
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

						if (pos._y > waterLevel &&
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

	_viewOrtho.unregisterRenderDelegate(RenderDelegate(this, &IslandHopper::onRenderOrtho));

	_window.close();
	_window.removeView(&_view);
	_window.removeView(&_viewOrtho);

	if (_font != nullptr)
	{
		delete _font;
		_font = nullptr;
	}
}

void IslandHopper::onKeyPressed(iKeyCode key)
{
	if (_activeControls)
	{
		switch (key)
		{
		case iKeyCode::Space:
			_plane->startFastTravel();
			break;

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

	case iKeyCode::LAlt:
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

		case iKeyCode::Space:
			_plane->stopFastTravel();
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

		case iKeyCode::F5:
			_plane->setPosition(iaVector3d(706378, 10280, 553650));
			break;

		case iKeyCode::F6:
			_plane->setPosition(iaVector3d(27934.5, 10270, 17452.6));
			break;

		case iKeyCode::F7:
			_plane->setPosition(iaVector3d(16912.3, 10300, 31719.6));
			break;

		case iKeyCode::F8:
			_plane->setPosition(iaVector3d(10841.6, 10450, 25283.8));
			break;

			/*	case iKeyCode::F9:
				{
					iAABoxI box;
					box._center = _plane->getSphere()._center.convert<int64>();
					box._halfWidths.set(10, 10, 10);
					_voxelTerrain->modify(box, 0);
				}
				break;

				case iKeyCode::F10:
				{
					iAABoxI box;
					box._center = _plane->getSphere()._center.convert<int64>();
					box._halfWidths.set(10, 10, 10);
					_voxelTerrain->modify(box, 128);
				}
				break;*/

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

void IslandHopper::onMouseMoved(const iaVector2i& from, const iaVector2i& to, iWindow* _window)
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
		iaString altitude = "Altitude:";
		altitude += iaString::toString(_plane->getAltitude() - waterLevel);
		iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.01, altitude);
	}

	_profiler.draw(&_window, _font, iaColor4f(1.0, 1.0, 1.0, 1));

	iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
}

void IslandHopper::run()
{
	iApplication::getInstance().run();
}
