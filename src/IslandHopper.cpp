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
#include <iNodeTransformControl.h>
#include <iNodeLODTrigger.h>
#include <iNodeLODSwitch.h>
#include <iOctree.h>
#include <iPhysics.h>
#include <iPhysicsMaterialCombo.h>
#include <iNodeWater.h>
#include <iNodeParticleSystem.h>
#include <iNodeEmitter.h>
#include <iPhysicsBody.h>
using namespace Igor;

#include "Player.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "StaticEnemy.h"
#include "EntityManager.h"

//#define SIN_WAVE_TERRAIN
#define USE_WATER

const float64 mapScaleXZ = 0.003693182;
const float64 highestMontain = 3742;
const float64 dataHeightScale = highestMontain;
const float64 waterOffset = 1000;

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
	_window.setSize(1280, 600);
	_window.setPosition(100, 100);
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

	// 1
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
		iTextureResourceFactory::getInstance().loadFile("skybox_day/front.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().loadFile("skybox_day/back.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().loadFile("skybox_day/left.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().loadFile("skybox_day/right.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().loadFile("skybox_day/top.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
		iTextureResourceFactory::getInstance().loadFile("skybox_day/bottom.jpg", iResourceCacheMode::Free, iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp));
	skyBoxNode->setTextureScale(1);
	skyBoxNode->setBoxSize(1000);
	// create a sky box material
	_materialSkyBox = iMaterialResourceFactory::getInstance().createMaterial();
	iMaterialResourceFactory::getInstance().getMaterial(_materialSkyBox)->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_materialSkyBox)->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterialGroup(_materialSkyBox)->setOrder(10);
	iMaterialResourceFactory::getInstance().getMaterialGroup(_materialSkyBox)->getMaterial()->setName("SkyBox");
	// and set the sky box material
	skyBoxNode->setMaterial(_materialSkyBox);
	// insert sky box to scene
	_scene->getRoot()->insertNode(skyBoxNode);

	// set up a meterial for the particles
	_particlesMaterial = iMaterialResourceFactory::getInstance().createMaterial();
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Texture2D1, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::Texture2D2, iRenderStateValue::On);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::BlendFuncSource, iRenderStateValue::SourceAlpha);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->getRenderStateSet().setRenderState(iRenderState::BlendFuncDestination, iRenderStateValue::OneMinusSourceAlpha);
	iMaterialResourceFactory::getInstance().getMaterial(_particlesMaterial)->setOrder(500);

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
		iMaterialResourceFactory::getInstance().getMaterialGroup(materialWater)->setOrder(300 - i);
		iMaterialResourceFactory::getInstance().getMaterial(materialWater)->getRenderStateSet().setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
		iMaterialResourceFactory::getInstance().getMaterial(materialWater)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
		iMaterialResourceFactory::getInstance().getMaterialGroup(materialWater)->getMaterial()->setName("WaterPlane");

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
	matrix.translate(759846, 4655, 381272);
	Player* player = new Player(_scene, matrix);
	_playerID = player->getID();
}

void IslandHopper::onVoxelDataGenerated(const iaVector3I& min, const iaVector3I& max)
{
	return;

/*	iaVector3I pos;
	iaVector3I diff;
	diff = max;
	diff -= min;

	srand(min._x + min._y + min._z);

	iaMatrixf enemyMatrix;
	Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));

	int count = 0;

	iaVector3I center(10000, 9400, 10000 - 200);

	for (int i = 0; i < 300; ++i)
	{
		pos.set(rand() % diff._x, rand() % diff._y, rand() % diff._z);
		pos += min;

		if (center.distance(pos) < 60)
		{
			bool addEnemy = true;

			for (int x = -2; x < 3; x++)
			{
				for (int y = -2; y < 3; y++)
				{
					for (int z = -2; z < 3; z++)
					{
						if (VoxelTerrain::getInstance().getVoxelDensity(iaVector3I(pos._x + x, pos._y + y, pos._z + z)) != 0)
						{
							addEnemy = false;
							break;
						}
					}
				}
			}

			if (addEnemy)
			{
				enemyMatrix._pos.set(pos._x, pos._y, pos._z);
				Enemy* enemy = new Enemy(_scene, enemyMatrix, _playerID);
				count++;
			}

			if (count >= 20)
			{
				break;
			}
		}
	}/**/

	/*count = 0;

	for (int i = 0; i < 800; ++i)
	{
		pos.set(rand() % diff._x, rand() % diff._y, rand() % diff._z);
		pos += min;

		if (center.distance(pos) < 60)
		{
			bool addEnemy = true;

			for (int x = -1; x < 2; x++)
			{
				for (int y = -1; y < 2; y++)
				{
					for (int z = -1; z < 2; z++)
					{
						if (VoxelTerrain::getInstance().getVoxelDensity(iaVector3I(pos._x + x, pos._y + y, pos._z + z)) != 0)
						{
							addEnemy = false;
							break;
						}
					}
				}
			}

			if (addEnemy)
			{
				iaVector3f from(pos._x, pos._y, pos._z);

				iaMatrixf matrix;

				switch (rand() % 6)
				{
				case 0:
					// nothing
					break;

				case 1:
					matrix.rotate(M_PI * 0.5, iaAxis::Z);
					break;

				case 2:
					matrix.rotate(M_PI * -0.5, iaAxis::Z);
					break;

				case 3:
					matrix.rotate(M_PI, iaAxis::Z);
					break;

				case 4:
					matrix.rotate(M_PI * 0.5, iaAxis::X);
					break;

				case 5:
					matrix.rotate(M_PI * -0.5, iaAxis::X);
					break;
				}

				iaVector3f to = from + matrix._top * -200;

				iaVector3I right(matrix._right._x, matrix._right._y, matrix._right._z);
				iaVector3I top(matrix._top._x, matrix._top._y, matrix._top._z);
				iaVector3I depth(matrix._depth._x, matrix._depth._y, matrix._depth._z);
				iaVector3I outside, inside;

				VoxelTerrain::getInstance().castRay(iaVector3I(from._x, from._y, from._z), iaVector3I(to._x, to._y, to._z), outside, inside);

				int rating = 0;

				if (outside.distance(pos) < 190)
				{
					iSphered sphere(iaVector3d(outside._x, outside._y, outside._z), 5);
					vector<uint64> result;
					EntityManager::getInstance().getEntities(sphere, result);
					if (result.empty())
					{
						if (VoxelTerrain::getInstance().getVoxelDensity(inside + right) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside - right) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside + right + depth) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside - right + depth) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside + right - depth) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside - right - depth) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside + depth) != 0) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(inside - depth) != 0) rating++;

						if (VoxelTerrain::getInstance().getVoxelDensity(outside + right) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside - right) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside + right + depth) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside - right + depth) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside + right - depth) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside - right - depth) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside + depth) < 50) rating++;
						if (VoxelTerrain::getInstance().getVoxelDensity(outside - depth) < 50) rating++;

						if (rating > 10)
						{
							enemyMatrix.identity();
							enemyMatrix = matrix;
							enemyMatrix._pos.set(outside._x, outside._y, outside._z);
							StaticEnemy* enemy = new StaticEnemy(_scene, enemyMatrix, _playerID);

							count++;
						}
					}
				}
			}

			if (count >= 100)
			{
				break;
			}
		}
	}/**/
}

void IslandHopper::init()
{
	con(" -- OpenGL 3D Test --" << endl);

	_perlinNoise.generateBase(313373);

	initViews();
	initScene();

	initPlayer();
	initVoxelData();

	initLSystems();

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
	_statisticsVisualizer.setVerbosity(iRenderStatisticsVerbosity::FPSMetricsAndTasks);

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

	_heightMap = iTextureResourceFactory::getInstance().loadFileAsPixmap("NewZealand.png");
	_minimap = iTextureResourceFactory::getInstance().loadFile("NewZealandMini.png", iResourceCacheMode::Keep, iTextureBuildMode::Normal);

	// launch resource handlers
	_taskFlushModels = iTaskManager::getInstance().addTask(new iTaskFlushModels(&_window));
	_taskFlushTextures = iTaskManager::getInstance().addTask(new iTaskFlushTextures(&_window));

	registerHandles();
}

void IslandHopper::createWaveParticleSystem()
{
	// create the particle system
	iaGradientColor4f colorGradient;
	colorGradient.setValue(0.0, iaColor4f(0, 0, 0, 0));
	colorGradient.setValue(0.5, iaColor4f(1, 1, 0, 1.0));
	colorGradient.setValue(1.0, iaColor4f(0, 0, 0, 0));

	iaGradientVector2f velocity;
	velocity.setValue(0.0, iaVector2f(0.01, 0.02));

	iaGradientVector2f visibility;
	visibility.setValue(0.0, iaVector2f(0.7, 2.0));

	iaGradientVector2f size;
	size.setValue(0.0, iaVector2f(0.3, 0.6));

	iaGradientf emission;
	emission.setValue(0.0, 5);

	iaGradientVector2f startOrientation;
	startOrientation.setValue(0.0, iaVector2f(0.0, 2.0 * M_PI));

	iaGradientVector2f startOrientationRate;
	startOrientationRate.setValue(0.0, iaVector2f(-0.05, 0.05));

	iNodeParticleSystem* particleSystem = static_cast<iNodeParticleSystem*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeParticleSystem));
	particleSystem->setMaterial(_particlesMaterial);
	particleSystem->setTextureA("particleKreuzHerzPikKaro.png");
	particleSystem->setFirstTextureTiling(2, 2);
	particleSystem->setColorGradient(colorGradient);
	particleSystem->setStartVelocityGradient(velocity);
	particleSystem->setStartOrientationGradient(startOrientation);
	particleSystem->setStartOrientationRateGradient(startOrientationRate);
	particleSystem->setStartVisibleTimeGradient(visibility);
	particleSystem->setStartSizeGradient(size);
	particleSystem->setEmissionGradient(emission);
	particleSystem->start();

	// create an mesh emitter with the shape of a cos wave
	iNodeEmitter* emitter = static_cast<iNodeEmitter*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeEmitter));
	particleSystem->setEmitter(emitter->getID());
	emitter->setType(iEmitterType::Mesh);

	iEmitterTriangle triangle;
	for (int i = 0; i < 65; i++)
	{
		float32 h1 = cos(i * 0.2) * 1.3;
		float32 h2 = cos((i + 1) * 0.2) * 1.3;

		triangle.pos[0] = iaVector3d(0 + i * 0.2, h1, 0);
		triangle.pos[1] = iaVector3d(1 + i * 0.2, h2, 0);
		triangle.pos[2] = iaVector3d(0 + i * 0.2, h1, 0.5);
		triangle.vel[0] = iaVector3d(0, 1, 0);
		triangle.vel[1] = iaVector3d(0, 1, 0);
		triangle.vel[2] = iaVector3d(0, 1, 0);
		emitter->addTriangle(triangle);

		triangle.pos[0] = iaVector3d(1 + i * 0.2, h2, 0);
		triangle.pos[1] = iaVector3d(1 + i * 0.2, h2, 0.5);
		triangle.pos[2] = iaVector3d(0 + i * 0.2, h1, 0.5);
		triangle.vel[0] = iaVector3d(0, 1, 0);
		triangle.vel[1] = iaVector3d(0, 1, 0);
		triangle.vel[2] = iaVector3d(0, 1, 0);
		emitter->addTriangle(triangle);
	}

	// position the emitter in scene using a transform node
	iNodeTransform* emitterTransform = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	emitterTransform->translate(706363.5, 1219, 553599);
	emitterTransform->insertNode(emitter);
	emitterTransform->insertNode(particleSystem);
	_scene->getRoot()->insertNode(emitterTransform);
}

void IslandHopper::initLSystems()
{
	initStyle1();
	initStyle2();
	initStyle3();

	// register plant mesh generator
	iModelResourceFactory::getInstance().registerModelDataIO("pg", &PlantMeshGenerator::createInstance);

	iaMatrixd matrix;
	matrix._pos.set(759852, 4655, 381260);
	generatePlant(matrix, plantInformationType2, 7, iTimer::getInstance().getTime());

	matrix._pos.set(759851, 4655, 381259.7);
	matrix.rotate(0.9, iaAxis::Y);
	generatePlant(matrix, plantInformationType2, 6, iTimer::getInstance().getTime());

	matrix._pos.set(759851.5, 4655.5, 381259);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType2, 5, iTimer::getInstance().getTime());

	matrix._pos.set(759852.3, 4655.5, 381259.3);
	matrix.rotate(0.3, iaAxis::Y);
	generatePlant(matrix, plantInformationType2, 4, iTimer::getInstance().getTime());

	matrix._pos.set(759845, 4652.5, 381262);
	matrix.rotate(0.3, iaAxis::Y);
	generatePlant(matrix, plantInformationType3, 6, iTimer::getInstance().getTime());

	matrix._pos.set(759845.3, 4652.25, 381263);
	matrix.rotate(0.5, iaAxis::Y);
	generatePlant(matrix, plantInformationType3, 5, iTimer::getInstance().getTime());

	matrix._pos.set(759844.5, 4652.25, 381263);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType3, 4, iTimer::getInstance().getTime());

	matrix._pos.set(759848, 4653, 381262);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 7, iTimer::getInstance().getTime());

	matrix._pos.set(759848.4, 4653, 381263);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 6, iTimer::getInstance().getTime());

	matrix._pos.set(759847.3, 4652.5, 381263.5);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 5, iTimer::getInstance().getTime());

	matrix._pos.set(759848, 4653, 381262.5);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 5, iTimer::getInstance().getTime());

	matrix._pos.set(759846, 4652.5, 381262.5);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 5, iTimer::getInstance().getTime());

	matrix._pos.set(759845.5, 4652.5, 381263);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 4, iTimer::getInstance().getTime());

	matrix._pos.set(759843, 4652.5, 381263.5);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 5, iTimer::getInstance().getTime());

	matrix._pos.set(759843.25, 4652.5, 381262.5);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 6, iTimer::getInstance().getTime());

	matrix._pos.set(759843.5, 4652.3, 381262);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType1, 4, iTimer::getInstance().getTime());

	matrix._pos.set(759841, 4652, 381263.5);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType3, 6, iTimer::getInstance().getTime());

	matrix._pos.set(759840.5, 4652, 381264);
	matrix.rotate(0.7, iaAxis::Y);
	generatePlant(matrix, plantInformationType2, 5, iTimer::getInstance().getTime());
}

void IslandHopper::initStyle1()
{
	_lSystemType1.setRule('F', "FF");

	vector<pair<float64, iaString>> weightedRule1;
	weightedRule1.push_back(pair<float64, iaString>(0.25, "F[+X]OF[-X]+X."));
	weightedRule1.push_back(pair<float64, iaString>(0.25, "FO[-X]F[+X]-X."));
	weightedRule1.push_back(pair<float64, iaString>(0.25, "F[RX]F[LX]ORX."));
	weightedRule1.push_back(pair<float64, iaString>(0.25, "F[LX]FO[RX]LX."));
	_lSystemType1.setRule('X', weightedRule1);

	vector<pair<float64, iaString>> weightedRule2;
	weightedRule2.push_back(pair<float64, iaString>(0.3, "."));
	weightedRule2.push_back(pair<float64, iaString>(0.7, "*"));
	_lSystemType1.setRule('.', weightedRule2);
	_lSystemType1.setAgeFilter('.', iLSystemAgeFunction::Greater, 2);

	vector<pair<float64, iaString>> weightedRule3;
	weightedRule3.push_back(pair<float64, iaString>(0.3, "*"));
	weightedRule3.push_back(pair<float64, iaString>(0.7, "o"));
	_lSystemType1.setRule('*', weightedRule3);
	_lSystemType1.setAgeFilter('*', iLSystemAgeFunction::Greater, 4);

	plantInformationType1._segmentLenght = 0.0125;
	plantInformationType1._segmentAngle = 0.4;
	plantInformationType1._trunkColor.set(0, 0.8, 0);
	plantInformationType1._budColor.set(0.8, 0.7, 0.0);
	plantInformationType1._flowerColor.set(1, 0, 0);
	plantInformationType1._leafColor.set(0, 0.7, 0);
	plantInformationType1._branchColor.set(0, 0.8, 0);
	plantInformationType1._lSystem = &_lSystemType1;
	plantInformationType1._axiom[0] = 'X';
}

void IslandHopper::initStyle2()
{
	_lSystemType2.setRule('F', "FF");

	vector<pair<float64, iaString>> weightedRule1;
	weightedRule1.push_back(pair<float64, iaString>(0.25, "F-[[X]+X]O+F[+FX]-X."));
	weightedRule1.push_back(pair<float64, iaString>(0.25, "F+[[X]-X]-F[-FX]O+X."));
	weightedRule1.push_back(pair<float64, iaString>(0.25, "FR[[X]LX]LFO[LFX]RX."));
	weightedRule1.push_back(pair<float64, iaString>(0.25, "FL[[X]RX]ORF[RFX]LX."));
	_lSystemType2.setRule('X', weightedRule1);

	vector<pair<float64, iaString>> weightedRule2;
	weightedRule2.push_back(pair<float64, iaString>(0.3, "."));
	weightedRule2.push_back(pair<float64, iaString>(0.7, "*"));
	_lSystemType2.setRule('.', weightedRule2);
	_lSystemType2.setAgeFilter('.', iLSystemAgeFunction::Greater, 2);

	vector<pair<float64, iaString>> weightedRule3;
	weightedRule3.push_back(pair<float64, iaString>(0.3, "*"));
	weightedRule3.push_back(pair<float64, iaString>(0.7, "o"));
	_lSystemType2.setRule('*', weightedRule3);
	_lSystemType2.setAgeFilter('*', iLSystemAgeFunction::Greater, 4);

	plantInformationType2._segmentLenght = 0.025;
	plantInformationType2._segmentAngle = 0.3;
	plantInformationType2._trunkColor.set(0, 0.7, 0);
	plantInformationType2._budColor.set(0.5, 0.5, 0.9);
	plantInformationType2._flowerColor.set(1, 0, 1);
	plantInformationType2._leafColor.set(0, 0.6, 0);
	plantInformationType2._branchColor.set(0, 0.8, 0);
	plantInformationType2._lSystem = &_lSystemType2;
	plantInformationType2._axiom[0] = 'X';
}

void IslandHopper::initStyle3()
{
	_lSystemType3.setRule('F', "FF");

	vector<pair<float64, iaString>> weightedRule1;
	weightedRule1.push_back(pair<float64, iaString>(0.5, "F[+X]O[-X]FX."));
	weightedRule1.push_back(pair<float64, iaString>(0.5, "F[RX]O[LX]FX."));
	_lSystemType3.setRule('X', weightedRule1);

	vector<pair<float64, iaString>> weightedRule2;
	weightedRule2.push_back(pair<float64, iaString>(0.3, "."));
	weightedRule2.push_back(pair<float64, iaString>(0.7, "*"));
	_lSystemType3.setRule('.', weightedRule2);
	_lSystemType3.setAgeFilter('.', iLSystemAgeFunction::Greater, 2);

	vector<pair<float64, iaString>> weightedRule3;
	weightedRule3.push_back(pair<float64, iaString>(0.3, "*"));
	weightedRule3.push_back(pair<float64, iaString>(0.7, "o"));
	_lSystemType3.setRule('*', weightedRule3);
	_lSystemType3.setAgeFilter('*', iLSystemAgeFunction::Greater, 4);

	plantInformationType3._segmentLenght = 0.012;
	plantInformationType3._segmentAngle = 0.5;
	plantInformationType3._trunkColor.set(0, 0.8, 0);
	plantInformationType3._budColor.set(0.8, 0.8, 0.5);
	plantInformationType3._flowerColor.set(1, 1, 1);
	plantInformationType3._leafColor.set(0, 0.7, 0);
	plantInformationType3._branchColor.set(0, 0.8, 0);
	plantInformationType3._lSystem = &_lSystemType3;
	plantInformationType3._axiom[0] = 'X';
}

void IslandHopper::generatePlant(const iaMatrixd& matrix, PlantInformation& plantInformation, uint32 iterations, uint64 seed)
{
	Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
	if (player != nullptr)
	{
		static uint64 plantType = 0;

		plantInformation._iterations = iterations;
		plantInformation._materialID = iMaterialResourceFactory::getInstance().getDefaultMaterialID();
		plantInformation._seed = seed;

		iModelDataInputParameter* inputParam = new iModelDataInputParameter();
		inputParam->_identifier = "pg";
		inputParam->_joinVertexes = true;
		inputParam->_needsRenderContext = false;
		inputParam->_modelSourceType = iModelSourceType::Generated;
		inputParam->_loadPriority = 0;
		inputParam->_parameters.setData(reinterpret_cast<const char*>(&plantInformation), sizeof(PlantInformation));

		iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
		modelNode->setModel(iaString("plant_") + iaString::itoa(plantType++), iResourceCacheMode::Free, inputParam);

		iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
		transformNode->setMatrix(matrix);

		iNodeLODSwitch* lodSwitch = static_cast<iNodeLODSwitch*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeLODSwitch));
		lodSwitch->insertNode(modelNode);
		lodSwitch->setThresholds(modelNode, 0, 10 * iterations);
		lodSwitch->addTrigger(player->getLODTriggerID());

		transformNode->insertNode(lodSwitch);

		_scene->getRoot()->insertNode(transformNode);
	}
}

void IslandHopper::createSmokingBox(const iaVector3d& pos)
{
	iaGradientVector2f startOrientation;
	startOrientation.setValue(0.0, iaVector2f(0.0, 2.0 * M_PI));

	iaGradientVector2f startOrientationRate;
	startOrientationRate.setValue(0.0, iaVector2f(-0.01, 0.01));

#if 0
	iaGradientColor4f smokeGradient;
	smokeGradient.setValue(0.0f, iaColor4f(1, 0, 1, 0.0));
	smokeGradient.setValue(0.2f, iaColor4f(0, 0, 1, 1.0));
	smokeGradient.setValue(0.4f, iaColor4f(0, 1, 1, 1.0));
	smokeGradient.setValue(0.6f, iaColor4f(0, 1, 0, 1.0));
	smokeGradient.setValue(0.8f, iaColor4f(1, 1, 0, 1.0));
	smokeGradient.setValue(1.0f, iaColor4f(1, 0, 0, 0.0));

#else
	iaGradientColor4f smokeGradient;
	smokeGradient.setValue(0.0, iaColor4f(0, 0, 0, 0));
	smokeGradient.setValue(0.1, iaColor4f(0, 0, 0, 0.8));
	smokeGradient.setValue(0.7, iaColor4f(0, 0, 0, 0.5));
	smokeGradient.setValue(1.0, iaColor4f(0, 0, 0, 0));
#endif

	iaGradientVector2f smokeSize;
	smokeSize.setValue(0.0, iaVector2f(0.4, 0.7));

	iaGradientVector2f smokeVisibility;
	smokeVisibility.setValue(0.0, iaVector2f(4.0, 5.0));

	iaGradientf smokeEmission;
	smokeEmission.setValue(0.0, 1);

	iaGradientVector2f smokeLift;
	smokeLift.setValue(0.0, iaVector2f(0.0005, 0.001));

	iaGradientf sizeScale;
	sizeScale.setValue(0.0, 1.0);
	sizeScale.setValue(5.0, 15.0);

	iNodeParticleSystem* smokeParticleSystem = static_cast<iNodeParticleSystem*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeParticleSystem));
	smokeParticleSystem->setMaterial(_particlesMaterial);
	smokeParticleSystem->setTextureA("particleSmoke.png");
	smokeParticleSystem->setTextureB("octave1.png");
	smokeParticleSystem->setTextureC("octave2.png");
	smokeParticleSystem->setVelocityOriented();
	smokeParticleSystem->setVortexTorque(0.2, 0.4);
	smokeParticleSystem->setVorticityConfinement(0.05);
	smokeParticleSystem->setVortexRange(20.0, 40.0);
	smokeParticleSystem->setVortexToParticleRate(0.05);
	smokeParticleSystem->setStartOrientationGradient(startOrientation);
	smokeParticleSystem->setStartOrientationRateGradient(startOrientationRate);
	smokeParticleSystem->setStartSizeGradient(smokeSize);
	smokeParticleSystem->setSizeScaleGradient(sizeScale);
	smokeParticleSystem->setColorGradient(smokeGradient);
	smokeParticleSystem->setStartVisibleTimeGradient(smokeVisibility);
	smokeParticleSystem->setEmissionGradient(smokeEmission);
	smokeParticleSystem->setStartLiftGradient(smokeLift);
	smokeParticleSystem->setPeriodTime(5.0);
	smokeParticleSystem->start();

	// fire 
	iaGradientColor4f colors;
	colors.setValue(0.0, iaColor4f(1.0, 0.9, 0.9, 0.0));
	colors.setValue(0.2, iaColor4f(1.0, 0.9, 0.9, 1.0));
	colors.setValue(0.4, iaColor4f(1.0, 0.9, 0.9, 1.0));
	colors.setValue(1.0, iaColor4f(0.0, 0.0, 0.0, 0.0));

	iaGradientVector2f visibility;
	visibility.setValue(0.0, iaVector2f(0.2, 0.5));

	iaGradientVector2f fireLift;
	fireLift.setValue(0.0, iaVector2f(0.008, 0.015));

	iaGradientf emission;
	emission.setValue(0.0, 4);

	iaGradientVector2f startSize;
	startSize.setValue(0.0, iaVector2f(0.3, 0.7));

	iNodeParticleSystem* particleSystem = static_cast<iNodeParticleSystem*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeParticleSystem));
	particleSystem->setMaterial(_particlesMaterial);
	particleSystem->setTextureA("particleFire.png");
	particleSystem->setFirstTextureTiling(4, 4);
	particleSystem->setColorGradient(colors);
	particleSystem->setEmissionGradient(emission);
	particleSystem->setStartVisibleTimeGradient(visibility);
	particleSystem->setStartSizeGradient(startSize);
	particleSystem->setPeriodTime(2.0);
	particleSystem->setVortexTorque(0.2, 0.5);
	particleSystem->setVorticityConfinement(0.05);
	particleSystem->setVortexRange(1.0, 2.0);
	particleSystem->setVortexToParticleRate(5);
	particleSystem->setStartLiftGradient(fireLift);
	particleSystem->start();

	iNodeEmitter* emitter = static_cast<iNodeEmitter*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeEmitter));
	particleSystem->setEmitter(emitter->getID());
	emitter->setType(iEmitterType::Cube);
	emitter->setSize(0.5);

	iNodeEmitter* smokeEmitter = static_cast<iNodeEmitter*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeEmitter));
	smokeParticleSystem->setEmitter(smokeEmitter->getID());
	smokeEmitter->setType(iEmitterType::Point);

	iPhysicsCollision* boxCollision = iPhysics::getInstance().createBox(1, 1, 1, iaMatrixd());
	iPhysicsBody* boxBody = iPhysics::getInstance().createBody(boxCollision);
	boxBody->setMass(10);
	boxBody->registerForceAndTorqueDelegate(iApplyForceAndTorqueDelegate(this, &IslandHopper::onApplyForceAndTorqueBox));

	iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	transformNode->translate(pos);

	iNodeModel* crate = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
	crate->setModel("crate.ompf");
	transformNode->insertNode(crate);

	iPhysics::getInstance().bindTransformNode(boxBody, transformNode);
	_scene->getRoot()->insertNode(transformNode);
	transformNode->insertNode(emitter);
	transformNode->insertNode(smokeEmitter);

	iNodeTransform* workaroundTransformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	workaroundTransformNode->translate(pos);
	workaroundTransformNode->insertNode(particleSystem);
	workaroundTransformNode->insertNode(smokeParticleSystem);
	_scene->getRoot()->insertNode(workaroundTransformNode);
}

void IslandHopper::onApplyForceAndTorqueBox(iPhysicsBody* body, float32 timestep)
{
	float64 Ixx;
	float64 Iyy;
	float64 Izz;
	float64 mass;
	iaVector3d force;

	iPhysics::getInstance().getMassMatrix(static_cast<void*>(body->getNewtonBody()), mass, Ixx, Iyy, Izz);
	force.set(0.0f, -mass * static_cast<float32>(__IGOR_GRAVITY__), 0.0f);

	body->setForce(force);
}

__IGOR_INLINE__ float64 metaballFunction(const iaVector3f& metaballPos, const iaVector3f& checkPos)
{
	return 1.0 / ((checkPos._x - metaballPos._x) * (checkPos._x - metaballPos._x) + (checkPos._y - metaballPos._y) * (checkPos._y - metaballPos._y) + (checkPos._z - metaballPos._z) * (checkPos._z - metaballPos._z));
}

void IslandHopper::generateVoxelData(VoxelBlockInfo* voxelBlockInfo)
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

	if (voxelData != nullptr)
	{
		voxelData->initData(size, size, size);

		for (int64 x = 0; x < voxelData->getWidth(); ++x)
		{
			for (int64 z = 0; z < voxelData->getDepth(); ++z)
			{
				iaVector3f pos(x * lodFactor + position._x + lodOffset._x, 0, z * lodFactor + position._z + lodOffset._z);

				iaColor4f color;
				_heightMap->getPixelBiLinear(fmod(pos._x * mapScaleXZ, _heightMap->getWidth()), fmod(pos._z * mapScaleXZ, _heightMap->getHeight()), color);
				float64 height = color._r * dataHeightScale;

				height -= 100;

				if (height < 0)
				{
					height *= 2;
				}

				height += 100;

				if (height < 0)
				{
					height *= 10;
				}

				if (height >= -50)
				{
					float64 noise = _perlinNoise.getValue(iaVector3d(pos._x * 0.0025, 0, pos._z * 0.0025), 8, 0.5) - 0.5;
					height += noise * 300;
				}

				height += waterOffset;

#ifdef SIN_WAVE_TERRAIN
				height = 2300 + (sin(pos._x * 0.125) + sin(pos._z * 0.125)) * 5.0;
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

				const float64 toMeta = 0.0175;

				for (int64 y = 0; y < voxelData->getHeight(); ++y)
				{
					pos._y = y * lodFactor + position._y + lodOffset._y;

					if (pos._x >= 706000 &&
						pos._x <= 707000 &&
						pos._y >= 1000 &&
						pos._y <= 2000 &&
						pos._z >= 553000 &&
						pos._z <= 554000)
					{
						float64 distance = 0;
						for (auto hole : _holes)
						{
							distance += metaballFunction(hole._center, pos) * hole._radius;
						}

						if (distance >= toMeta)
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

void IslandHopper::deinit()
{
	iModelResourceFactory::getInstance().unregisterModelDataIO("pg");

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

			case iKeyCode::F12:
			{
				_wireframe = !_wireframe;

				uint64 terrainMaterial = _voxelTerrain->getMaterial();

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

			case iKeyCode::F11:
				_showMinimap = !_showMinimap;
				break;

			case iKeyCode::B:
			{
				Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
				if (player != nullptr)
				{
					createSmokingBox(player->getSphere()._center);
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

void IslandHopper::onMouseMoved(int32 x1, int32 y1, int32 x2, int32 y2, iWindow* _window)
{
	if (_activeControls)
	{
		if (iMouse::getInstance().getRightButton())
		{
			_mouseDelta.set(x2 - x1, y2 - y1);
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
	Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
	if (player != nullptr)
	{
		if (iKeyboard::getInstance().getKey(iKeyCode::LShift) && key == iKeyCode::MouseLeft)
		{
			iaVector2i pos = iMouse::getInstance().getPos();

			pos._x -= 10;
			pos._y -= 10;

			float64 miniMapSize = 300;

			if (pos._x <= miniMapSize && pos._y <= miniMapSize)
			{
				float64 height = player->getSphere()._center._y;
				player->setPosition(iaVector3d(static_cast<float64>(pos._x) / mapScaleXZ / miniMapSize * 4096, height, static_cast<float64>(pos._y) / mapScaleXZ / miniMapSize * 4096));
			}
		}
	}
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

void IslandHopper::initVoxelData()
{
	_holes.push_back(iSpheref(iaVector3f(706378, 1245, 553640), 0.6));
	_holes.push_back(iSpheref(iaVector3f(706378, 1235, 553635), 0.5));
	_holes.push_back(iSpheref(iaVector3f(706380, 1230, 553630), 0.4));
	_holes.push_back(iSpheref(iaVector3f(706390, 1220, 553620), 0.6));
	_holes.push_back(iSpheref(iaVector3f(706380, 1230, 553610), 0.5));
	_holes.push_back(iSpheref(iaVector3f(706370, 1220, 553600), 0.7));

	_voxelTerrain = new VoxelTerrain(GenerateVoxelsDelegate(this, &IslandHopper::generateVoxelData));

	_voxelTerrain->setScene(_scene);
	Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
	if (player != nullptr)
	{
		_voxelTerrain->setLODTrigger(player->getLODTriggerID());
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
		if (iTimer::getInstance().getTimerTime() > 5000 &&
			iTaskManager::getInstance().getQueuedRegularTaskCount() < 4)
		{
			_loading = false;
			_activeControls = true;
			_mouseDelta.set(0, 0);

			createWaveParticleSystem();
			createSmokingBox(iaVector3d(759845, 4720, 381248));
			createSmokingBox(iaVector3d(759829, 4750, 381244));

			createSmokingBox(iaVector3d(759811, 4700, 381189));
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
	iMaterialResourceFactory::getInstance().setMaterial(_materialWithTextureAndBlending);
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

			if (_showMinimap)
			{
				iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
				iRenderer::getInstance().drawTexture(0, 0, size, size, _minimap);

				iMaterialResourceFactory::getInstance().setMaterial(_materialSolid);
				iRenderer::getInstance().setColor(iaColor4f(0, 0, 0, 1));
				iRenderer::getInstance().drawLine(0, 0, size, 0);
				iRenderer::getInstance().drawLine(0, 0, 0, size);
				iRenderer::getInstance().drawLine(size, 0, size, size);
				iRenderer::getInstance().drawLine(0, size, size, size);

				iRenderer::getInstance().setColor(iaColor4f(1, 0, 0, 1));
				float64 playerPosZ = player->getSphere()._center._z * mapScaleXZ * size / 4096;
				float64 playerPosX = player->getSphere()._center._x * mapScaleXZ * size / 4096;
				iRenderer::getInstance().drawLine(0, playerPosZ, size, playerPosZ);
				iRenderer::getInstance().drawLine(playerPosX, 0, playerPosX, size);

				iRenderer::getInstance().setModelMatrix(matrix);
			}

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


