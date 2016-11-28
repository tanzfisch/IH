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
#include <iPerlinNoise.h>
#include <iNodeTransformControl.h>
#include <iNodeLODTrigger.h>
#include <iNodeLODSwitch.h>
#include <iOctree.h>
#include <iPhysics.h>
#include <iPhysicsMaterialCombo.h>
#include <iNodeWater.h>
using namespace Igor;

#include "Player.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "StaticEnemy.h"
#include "EntityManager.h"

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

    iApplication::getInstance().registerApplicationHandleDelegate(iApplicationHandleDelegate(this, &IslandHopper::onHandle));
}

void IslandHopper::unregisterHandles()
{
    iApplication::getInstance().unregisterApplicationHandleDelegate(iApplicationHandleDelegate(this, &IslandHopper::onHandle));

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
    _window.setSize(1024, 768);
#else
    _window.setSizeByDesktop();
    _window.setFullscreen();
#endif
    _window.open();

    iMouse::getInstance().showCursor(false);

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
        iTextureResourceFactory::getInstance().loadFile("skybox_day/front.jpg", iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
        iTextureResourceFactory::getInstance().loadFile("skybox_day/back.jpg", iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
        iTextureResourceFactory::getInstance().loadFile("skybox_day/left.jpg", iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
        iTextureResourceFactory::getInstance().loadFile("skybox_day/right.jpg", iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
        iTextureResourceFactory::getInstance().loadFile("skybox_day/top.jpg", iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp),
        iTextureResourceFactory::getInstance().loadFile("skybox_day/bottom.jpg", iTextureBuildMode::Mipmapped, iTextureWrapMode::Clamp));
    skyBoxNode->setTextureScale(1);
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

    // TODO just provisorical water
    // create a water plane and add it to scene

    for (int i = 0; i < 10; ++i) // todo just for the look give water a depth
    {
        iNodeWater* waterNode = static_cast<iNodeWater*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeWater));
        waterNode->setWaterPosition(263.5 - i * 2.2);

        if (i == 9)
        {
            waterNode->setAmbient(iaColor4f(1, 0.4 - static_cast<float64>(i) * 0.01, 0.95 - static_cast<float64>(i) * 0.08, 1.0));
        }
        else
        {
            waterNode->setAmbient(iaColor4f(1, 0.4 - static_cast<float64>(i) * 0.01, 0.95 - static_cast<float64>(i) * 0.08, 0.2 + static_cast<float64>(i) * 0.01));
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
}

void IslandHopper::initPlayer()
{
    iaMatrixf matrix;
    //matrix.translate(10000, 9400, 10000);
    matrix.translate(9986, 1000, 8977);
    Player* player = new Player(_scene, matrix);
    _playerID = player->getID();

    /*iaMatrixf enemyMatrix;
    enemyMatrix._pos.set(10000, 9400, 10000 - 200);
    BossEnemy* boss = new BossEnemy(_scene, enemyMatrix, _playerID);
    _bossID = boss->getID();*/
}

void IslandHopper::onVoxelDataGenerated(const iaVector3I& min, const iaVector3I& max)
{
    return;

    iaVector3I pos;
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

    count = 0;

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

    initViews();
    initScene();

    initPlayer();
    initVoxelData();

    // set up octree debug rendering
    _octreeMaterial = iMaterialResourceFactory::getInstance().createMaterial("Octree");
    iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
    iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->getRenderStateSet().setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
    iMaterialResourceFactory::getInstance().getMaterial(_octreeMaterial)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::On);

    // set up statistics
    _font = new iTextureFont("StandardFont.png");
    _materialWithTextureAndBlending = iMaterialResourceFactory::getInstance().createMaterial("TextureAndBlending");
    iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
    iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
    iMaterialResourceFactory::getInstance().getMaterial(_materialWithTextureAndBlending)->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
    iStatistics::getInstance().setVerbosity(iRenderStatisticsVerbosity::None);

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
}

void IslandHopper::deinit()
{
    unregisterHandles();

    iTaskManager::getInstance().abortTask(_taskFlushModels);
    iTaskManager::getInstance().abortTask(_taskFlushTextures);

    VoxelTerrain::getInstance().unregisterVoxelDataGeneratedDelegate(VoxelDataGeneratedDelegate(this, &IslandHopper::onVoxelDataGenerated));
    VoxelTerrain::getInstance().destroyInstance();

    iSceneFactory::getInstance().destroyScene(_scene);

    _view.unregisterRenderDelegate(RenderDelegate(this, &IslandHopper::onRender));
    _viewOrtho.unregisterRenderDelegate(RenderDelegate(this, &IslandHopper::onRenderOrtho));

    _window.close();
    _window.removeView(&_view);
    _window.removeView(&_viewOrtho);

    if (_font)
    {
        delete _font;
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
                player->startForward();
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
                player->startFastTurn();
                break;

            case iKeyCode::One:
                player->startRollLeft();
                break;

            case iKeyCode::Three:
                player->startRollRight();
                break;

            case iKeyCode::Space:
                player->dig(_toolSize, _toolDensity);
                break;

            case iKeyCode::F5:
                player->setPosition(iaVector3f(9986, 400, 8977));
                break;

            case iKeyCode::F6:
                player->setPosition(iaVector3f(27934.5, 553.604, 17452.6));
                break;

            case iKeyCode::F7:
                player->setPosition(iaVector3f(16912.3, 1000, 31719.6));
                break;

            case iKeyCode::F8:
                player->setPosition(iaVector3f(10841.6, 3000, 25283.8));
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
        iRenderStatisticsVerbosity level = iStatistics::getInstance().getVerbosity();

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

        iStatistics::getInstance().setVerbosity(level);
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

            case iKeyCode::LShift:
                player->stopFastTurn();
                break;

            case iKeyCode::One:
                player->stopRollLeft();
                break;

            case iKeyCode::Three:
                player->stopRollRight();
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
        _mouseDelta.set(x2 - x1, y2 - y1);

        if (!iKeyboard::getInstance().getKey(iKeyCode::Space))
        {
            iMouse::getInstance().setCenter(true);
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
            if (key == iKeyCode::MouseRight)
            {
                iaVector3f updown(_weaponPos._x, _weaponPos._y, _weaponPos._z);
                player->shootSecondaryWeapon(_view, updown);
            }

            if (key == iKeyCode::MouseLeft)
            {
                iaVector3f updown(_weaponPos._x, _weaponPos._y, _weaponPos._z);
                player->shootPrimaryWeapon(_view, updown);
            }
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

void IslandHopper::initVoxelData()
{
    VoxelTerrain::getInstance().setScene(_scene);
    Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
    if (player != nullptr)
    {
        VoxelTerrain::getInstance().setLODTrigger(player->getLODTriggerID());
    }
    VoxelTerrain::getInstance().registerVoxelDataGeneratedDelegate(VoxelDataGeneratedDelegate(this, &IslandHopper::onVoxelDataGenerated));
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
        if (iTaskManager::getInstance().getQueuedTaskCount() < 4)
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
    iStatistics::getInstance().drawStatistics(&_window, _font, iaColor4f(1.0, 1.0, 1.0, 1));

    iaMatrixf matrix;
    iRenderer::getInstance().setViewMatrix(matrix);
    matrix.translate(iaVector3f(0, 0, -30));
    iRenderer::getInstance().setModelMatrix(matrix);
    iMaterialResourceFactory::getInstance().setMaterial(_materialWithTextureAndBlending);
    iRenderer::getInstance().setFont(_font);

    if (_loading)
    {
        iRenderer::getInstance().setColor(iaColor4f(0, 0, 1, 1));
        iRenderer::getInstance().setFontSize(40.0f);
        iRenderer::getInstance().drawString(_window.getClientWidth() * 0.5, _window.getClientHeight() * 0.5, "generating level ...", iHorizontalAlignment::Center, iVerticalAlignment::Center);
    }
    else
    {
        /*  BossEnemy* boss = static_cast<BossEnemy*>(EntityManager::getInstance().getEntity(_bossID));
          if (boss == nullptr)
          {
              iRenderer::getInstance().setColor(iaColor4f(0, 1, 0, 1));
              iRenderer::getInstance().setFontSize(40.0f);
              iRenderer::getInstance().drawString(_window.getClientWidth() * 0.5, _window.getClientHeight() * 0.5, "you win!", iHorrizontalAlign::Center, iVerticalAlign::Center);
          }*/

        Player* player = static_cast<Player*>(EntityManager::getInstance().getEntity(_playerID));
        if (player != nullptr)
        {
            iaString healthText = iaString::ftoa(player->getHealth(), 0);
            iaString shieldText = iaString::ftoa(player->getShield(), 0);

            iRenderer::getInstance().setFontSize(15.0f);
            iRenderer::getInstance().setColor(iaColor4f(1, 0, 0, 1));
            iRenderer::getInstance().drawString(_window.getClientWidth() * 0.05, _window.getClientHeight() * 0.05, healthText);

            iRenderer::getInstance().setColor(iaColor4f(0, 0, 1, 1));
            iRenderer::getInstance().drawString(_window.getClientWidth() * 0.10, _window.getClientHeight() * 0.05, shieldText);

            iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));

            iaString position;
            position += iaString::ftoa(player->getSphere()._center._x, 0);
            position += ",";
            position += iaString::ftoa(player->getSphere()._center._y, 0);
            position += ",";
            position += iaString::ftoa(player->getSphere()._center._z, 0);
            iRenderer::getInstance().drawString(_window.getClientWidth() * 0.05, _window.getClientHeight() * 0.10, position);

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




    iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
}

void IslandHopper::run()
{
    iApplication::getInstance().run();
}


