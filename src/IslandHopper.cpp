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
#include <iNodeFactory.h>
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
#include <iStatistics.h>
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

#include "Player.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "StaticEnemy.h"
#include "EntityManager.h"

// #define SIN_WAVE_TERRAIN
#define USE_WATER

const float64 waterOffset = 10000;

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
	_view.setClipPlanes(1.0f, 60000.f);
	_view.registerRenderDelegate(RenderDelegate(this, &IslandHopper::onRender));
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

	_lightTranslate = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	_lightTranslate->translate(100, 100, 100);
	_lightRotate = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	_lightNode = static_cast<iNodeLight*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeLight));
	_lightNode->setAmbient(iaColor4f(0.7f, 0.7f, 0.7f, 1.0f));
	_lightNode->setDiffuse(iaColor4f(1.0f, 0.9f, 0.8f, 1.0f));
	_lightNode->setSpecular(iaColor4f(1.0f, 0.9f, 0.87f, 1.0f));

	_lightRotate->insertNode(_lightTranslate);
	_lightTranslate->insertNode(_lightNode);
	_scene->getRoot()->insertNode(_lightRotate);

	// reate a sky box and add it to scene
	iNodeSkyBox* skyBoxNode = static_cast<iNodeSkyBox*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeSkyBox));
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
    material->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
    material->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
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
		iNodeWater* waterNode = static_cast<iNodeWater*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeWater));
		waterNode->setWaterPosition(waterOffset - i * 1);

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
        material->getRenderStateSet().setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
        material->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
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
	Player* player = new Player(_scene, &_view, matrix);
	_playerID = player->getID();
}

void IslandHopper::init()
{
	_perlinNoise.generateBase(313373);

	initViews();
	initScene();

	initPlayer();
	initVoxelData();

	// set up octree debug rendering
	_octreeMaterial = iMaterialResourceFactory::getInstance().createMaterial("Octree");
	iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->getRenderStateSet().setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::On);

	// setup some materials
	_font = new iTextureFont("StandardFont.png");
	_materialWithTextureAndBlending = iMaterialResourceFactory::getInstance().createMaterial("TextureAndBlending");
	iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);

	_materialSolid = iMaterialResourceFactory::getInstance().createMaterial();
	iMaterialResourceFactory::getInstance().getMaterial(_materialSolid)->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_materialSolid)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);

	// configure statistics
	_statisticsVisualizer.setVerbosity(iRenderStatisticsVerbosity::FPSOnly);

	uint64 particlesMaterial = iMaterialResourceFactory::getInstance().createMaterial();
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->setName("PMat");
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Texture2D1, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Texture2D2, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::BlendFuncSource, iRenderStateValue::SourceAlpha);
	iMaterialResourceFactory::getInstance().getMaterial(particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::BlendFuncDestination, iRenderStateValue::OneMinusSourceAlpha);

	// launch resource handlers
	_taskFlushModels = iTaskManager::getInstance().addTask(new iTaskFlushModels(&_window));
	_taskFlushTextures = iTaskManager::getInstance().addTask(new iTaskFlushTextures(&_window));

	registerHandles();

    // start physics
    iPhysics::getInstance().start();
}

void IslandHopper::initVoxelData()
{
    _voxelTerrain = new iVoxelTerrain(iVoxelTerrainGenerateDelegate(this, &IslandHopper::onGenerateVoxelData), 
        iVoxelTerrainPlacePropsDelegate(this, &IslandHopper::onVoxelDataGenerated));

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

    Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
    if (player != nullptr)
    {
        _voxelTerrain->setLODTrigger(player->getLODTriggerID());
    }
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
#ifndef SIN_WAVE_TERRAIN
                // TODO
                float64 height = 10000 + (sin(pos._x * 0.0025) + sin(pos._z * 0.0025)) * 200.0;
#else 
                float64 height = 10000 + (sin(pos._x * 0.125) + sin(pos._z * 0.125)) * 20.0;
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

						if (pos._y > waterOffset &&
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

	_view.unregisterRenderDelegate(RenderDelegate(this, &IslandHopper::onRender));
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
		Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));

		if (player != nullptr)
		{
			switch (key)
			{
			case iKeyCode::A:
				player->startLeft();
				break;

			case iKeyCode::D:
				player->startRight();
				break;

			case iKeyCode::W:
			case iKeyCode::Z:
				player->startForward();
				break;

			case iKeyCode::H:
				player->stopForward();
				break;

			case iKeyCode::S:
				player->startBackward();
				break;

			case iKeyCode::Q:
				player->startUp();
				break;

			case iKeyCode::E:
				player->startDown();
				break;

			case iKeyCode::LShift:
				//player->startFastTurn();
				break;

			case iKeyCode::One:
				player->startRollLeft();
				break;

			case iKeyCode::Three:
				player->startRollRight();
				break;

			case iKeyCode::Space:
				//player->dig(_toolSize, _toolDensity);
				player->startFastTravel();
				break;
			}
		}
	}

	switch (key)
	{
	case iKeyCode::ESC:
		iApplication::getInstance().stop();
		break;

	case iKeyCode::F3:
	{
		iRenderStatisticsVerbosity level = _statisticsVisualizer.getVerbosity();

		if (level == iRenderStatisticsVerbosity::All)
		{
			level = iRenderStatisticsVerbosity::None;
		}
		else
		{
			int value = static_cast<int>(level);
			value++;
			level = static_cast<iRenderStatisticsVerbosity>(value);
		}

		_statisticsVisualizer.setVerbosity(level);
	}
	break;
	}
}

void IslandHopper::onKeyReleased(iKeyCode key)
{
	if (_activeControls)
	{
		Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
		if (player != nullptr)
		{
            switch (key)
            {
            case iKeyCode::A:
                player->stopLeft();
                break;

            case iKeyCode::D:
                player->stopRight();
                break;

            case iKeyCode::W:
                player->stopForward();
                break;

            case iKeyCode::S:
                player->stopBackward();
                break;

            case iKeyCode::Q:
                player->stopUp();
                break;

            case iKeyCode::E:
                player->stopDown();
                break;

            case iKeyCode::Space:
                player->stopFastTravel();
                break;

            case iKeyCode::LShift:
                //player->stopFastTurn();
                break;

            case iKeyCode::One:
                player->stopRollLeft();
                break;

            case iKeyCode::Three:
                player->stopRollRight();
                break;

            case iKeyCode::F5:
                player->setPosition(iaVector3d(706378, 1280, 553650));
                break;

            case iKeyCode::F6:
                player->setPosition(iaVector3d(27934.5, 2700, 17452.6));
                break;

            case iKeyCode::F7:
                player->setPosition(iaVector3d(16912.3, 3000, 31719.6));
                break;

            case iKeyCode::F8:
                player->setPosition(iaVector3d(10841.6, 4500, 25283.8));
                break;

            case iKeyCode::F9:
                {
                    Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
                    if (player != nullptr)
                    {
                        iAABoxI box;
                        iaConvert::convert(player->getSphere()._center, box._center);
                        box._halfWidths.set(10, 10, 10);
                        _voxelTerrain->modify(box, 0);
                    }
                }
                break;

            case iKeyCode::F10:
            {
                Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
                if (player != nullptr)
                {
                    iAABoxI box;
                    iaConvert::convert(player->getSphere()._center, box._center);
                    box._halfWidths.set(10, 10, 10);
                    _voxelTerrain->modify(box, 128);
                }
            }
            break;

			case iKeyCode::F12:
			    {
				    _wireframe = !_wireframe;

				    uint64 terrainMaterial = _voxelTerrain->getMaterialID();

				    if (_wireframe)
				    {
					    iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::Off);
					    iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::On);
				    }
				    else
				    {
					    iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::On);
					    iMaterialResourceFactory::getInstance().getMaterial(terrainMaterial)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::Off);
				    }
			    }
			    break;
			}
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
		if (iMouse::getInstance().getRightButton())
		{
			_mouseDelta.set(to._x - from._x, to._y - from._y);
			iMouse::getInstance().setCenter(true);
		}
		else
		{
			_mouseDelta.set(0, 0);
		}
	}
}

void IslandHopper::onMouseDown(iKeyCode key)
{
	if (_activeControls)
	{
		Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
		if (player != nullptr)
		{
			/* if (key == iKeyCode::MouseMiddle)
			 {
				 iaVector3d updown(_weaponPos._x, _weaponPos._y, _weaponPos._z);
				 player->shootSecondaryWeapon(_view, updown);
			 }

			 if (key == iKeyCode::MouseLeft)
			 {
				 iaVector3d updown(_weaponPos._x, _weaponPos._y, _weaponPos._z);
				 player->shootPrimaryWeapon(_view, updown);
			 }*/
		}
	}
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

void IslandHopper::handleMouse()
{
	if (_activeControls)
	{
		Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
		if (player != nullptr)
		{
			_weaponPos.set(_window.getClientWidth() * 0.5, _window.getClientHeight() * 0.5, 0);

			float32 headingDelta = _mouseDelta._x * 0.002;
			float32 pitchDelta = _mouseDelta._y * 0.002;
			player->rotate(-headingDelta, -pitchDelta);
		}
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
	else
	{
		/*    BossEnemy* boss = static_cast<BossEnemy*>(EntityManager::getInstance().getEntity(_bossID));
			if (boss == nullptr)
			{
				vector<uint64> ids;
				EntityManager::getInstance().getEntities(ids);

				for (auto id : ids)
				{
					if (_playerID != id)
					{
						Entity* entity = EntityManager::getInstance().getEntity(id);
						if (entity != nullptr &&
							entity->getType() == EntityType::Vehicle)
						{
							EntityManager::getInstance().getEntity(id)->kill();
						}
					}
				}
			}*/

		EntityManager::getInstance().handle();
	}

	handleMouse();
}

void IslandHopper::onRender()
{
	// nothing to do
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
		Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
		if (player != nullptr)
		{
			iaString healthText = iaString::ftoa(player->getHealth(), 0);
			iaString shieldText = iaString::ftoa(player->getShield(), 0);

			iRenderer::getInstance().setFontSize(15.0f);
			iRenderer::getInstance().setColor(iaColor4f(1, 0, 0, 1));
			iRenderer::getInstance().drawString(_window.getClientWidth() * 0.30, _window.getClientHeight() * 0.05, healthText);

			iRenderer::getInstance().setColor(iaColor4f(0, 0, 1, 1));
			iRenderer::getInstance().drawString(_window.getClientWidth() * 0.35, _window.getClientHeight() * 0.05, shieldText);

			iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));

			iaString position;
			position += iaString::ftoa(player->getSphere()._center._x, 0);
			position += ",";
			position += iaString::ftoa(player->getSphere()._center._y, 0);
			position += ",";
			position += iaString::ftoa(player->getSphere()._center._z, 0);
			iRenderer::getInstance().drawString(_window.getClientWidth() * 0.30, _window.getClientHeight() * 0.10, position);

			const float64 size = 300;
			iaMatrixd mapMatrix;
			mapMatrix.translate(10, 10, -30);
			iRenderer::getInstance().setModelMatrix(mapMatrix);

			player->drawReticle(_window);
		}
		else
		{
			iRenderer::getInstance().setColor(iaColor4f(1, 0, 0, 1));
			iRenderer::getInstance().setFontSize(40.0f);
			iRenderer::getInstance().drawString(_window.getClientWidth() * 0.5, _window.getClientHeight() * 0.5, "you are dead :-P", iHorizontalAlignment::Center, iVerticalAlignment::Center);
			_activeControls = false;
		}
	}

	_statisticsVisualizer.drawStatistics(&_window, _font, iaColor4f(1.0, 1.0, 1.0, 1));

	iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
}

void IslandHopper::run()
{
	iApplication::getInstance().run();
}


