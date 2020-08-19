#include "GameLayer.h"

#include "../ui/HUDLayer.h"

static const int64 s_waterLevel = 10000;
static const int64 s_treeLine = 10200;

GameLayer::GameLayer(iWindow *window, int32 zIndex)
    : iLayer(window, "Game", zIndex)
{
}

void GameLayer::initViews()
{
    getWindow()->setVSync(false);

    _view.setName("Game");
    _view.setClearColor(false);
    _view.setClearDepth(true);
    _view.setPerspective(80);
    _view.setClipPlanes(1.0f, 80000.f);
    getWindow()->addView(&_view);

    iMouse::getInstance().showCursor(true);
}

void GameLayer::initScene()
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
    _materialSkyBox = iMaterialResourceFactory::getInstance().createMaterial("SkyBox");
    auto material = iMaterialResourceFactory::getInstance().getMaterial(_materialSkyBox);
    material->setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
    material->setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
    material->setOrder(iMaterial::RENDER_ORDER_MIN);
    // and set the sky box material
    skyBoxNode->setMaterial(_materialSkyBox);
    // insert sky box to scene
    _scene->getRoot()->insertNode(skyBoxNode);

    iNodeWater *waterNode = iNodeManager::getInstance().createNode<iNodeWater>();
    waterNode->setWaterPosition(s_waterLevel);

    // create a water material
    uint64 materialWater = iMaterialResourceFactory::getInstance().createMaterial("Water");
    material = iMaterialResourceFactory::getInstance().getMaterial(materialWater);
    // TODO why is this a problem? material->setRenderState(iRenderState::DepthMask, iRenderStateValue::Off);
    material->setRenderState(iRenderState::Blend, iRenderStateValue::Off);

    // and set the water material
    waterNode->setMaterial(materialWater);
    _scene->getRoot()->insertNode(waterNode);
}

void GameLayer::setHud(HUDLayer *hudLayer)
{
    _hudLayer = hudLayer;
}

void GameLayer::initPlayer()
{
    if (_plane == nullptr)
    {
        _plane = new Plane(_scene, &_view);
        _plane->_planeCrashedEvent.append(PlaneCrashedDelegate(this, &GameLayer::playerCrashed));
        _plane->_planeLandedEvent.append(PlaneLandedDelegate(this, &GameLayer::playerLanded));
    }

    iaMatrixd matrix;
    matrix.translate(740846, 10100, 381272);
    _plane->setMatrix(matrix);
}

void GameLayer::onInit()
{
    _perlinNoise.generateBase(313373);

    initViews();
    initScene();

    initPlayer();
    initVoxelData();

    // instancing material
    _materialWithInstancing = iMaterialResourceFactory::getInstance().createMaterial("Instancing");
    auto material = iMaterialResourceFactory::getInstance().getMaterial(_materialWithInstancing);
    material->setRenderState(iRenderState::Instanced, iRenderStateValue::On);
    //material->setRenderState(iRenderState::InstancedFunc, iRenderStateValue::PositionOrientation);
    //material->setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
    material->addShaderSource("igor/default_ipo.vert", iShaderObjectType::Vertex);
    material->addShaderSource("igor/default_ipo_directional_light.frag", iShaderObjectType::Fragment);
    material->compileShader();

    // launch resource handlers
    _taskFlushModels = iTaskManager::getInstance().addTask(new iTaskFlushModels(getWindow()));
    _taskFlushTextures = iTaskManager::getInstance().addTask(new iTaskFlushTextures(getWindow()));

    // start physics
    iPhysics::getInstance().start();
}

void GameLayer::initVoxelData()
{
    // it's a flat landscape so we can limit the discovery in the vertical axis
    iaVector3I maxDiscovery(1000000, 10, 1000000);

    _voxelTerrain = new iVoxelTerrain(iVoxelTerrainGenerateDelegate(this, &GameLayer::onGenerateVoxelData),
                                      iVoxelTerrainPlacePropsDelegate(this, &GameLayer::onVoxelDataGenerated), 11, 6, maxDiscovery);

    iTargetMaterial *targetMaterial = _voxelTerrain->getTargetMaterial();
    targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("green.png"), 0);
    targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("brown.png"), 1);
    targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("gray.png"), 2);
    targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("detail.png"), 3);
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

void GameLayer::onVoxelDataGenerated(iVoxelBlockPropsInfo voxelBlockPropsInfo)
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

    const iaVector3I &min = voxelBlockPropsInfo._min;
    const iaVector3I &max = voxelBlockPropsInfo._max;
    iaRandomNumberGeneratoru rand(min._x + min._y + min._z);

    for (int64 x = min._x; x < max._x; x += 4)
    {
        for (int64 z = min._z; z < max._z; z += 4)
        {
            float64 offsetx = (rand.getNext() % 300 / 100.0) - 1.5;
            float64 offsetz = (rand.getNext() % 300 / 100.0) - 1.5;

            iaVector3d from(std::min(std::max((float64)min._x, x + offsetx), (float64)max._x), std::min(max._y, s_treeLine), std::min(std::max((float64)min._z, z + offsetz), (float64)max._z));
            iaVector3d to = from;
            to._y = std::max(min._y, s_waterLevel);

            iaVector3I outside, inside;

            if (_voxelTerrain->castRay(iaVector3I(from._x, from._y, from._z), iaVector3I(to._x, to._y, to._z), outside, inside))
            {
                iaVector3d pos(outside._x, outside._y, outside._z);

                float32 noise = _perlinNoise.getValue(iaVector3d(x * 0.03, min._y, z * 0.03), 3);

                if (noise > 0.6)
                {
                    iNodeTransform *transformTree = iNodeManager::getInstance().createNode<iNodeTransform>();
                    transformTree->translate(pos);
                    transformTree->rotate(rand.getNextFloat() * M_PI_2, iaAxis::Y);
                    iNodeModel *tree = iNodeManager::getInstance().createNode<iNodeModel>();

                    float32 range = (pos._y - s_waterLevel) / (s_treeLine - s_waterLevel);

                    if (rand.getNextFloat() * range > 0.1)
                    {
                        tree->setModel("tree.ompf");
                    }
                    else
                    {
                        tree->setModel("palm.ompf");
                    }

                    tree->registerModelReadyDelegate(iModelReadyDelegate(this, &GameLayer::onModelReady));
                    iNodeLODSwitch *lodSwitch = iNodeManager::getInstance().createNode<iNodeLODSwitch>();
                    lodSwitch->insertNode(tree);
                    lodSwitch->addTrigger(_plane->getLODTriggerID());
                    lodSwitch->setThresholds(tree, 0, 400);
                    transformTree->insertNode(lodSwitch);
                    _scene->getRoot()->insertNodeAsync(transformTree);
                }
            }
        }
    }
}

void GameLayer::onModelReady(iNodeID nodeID)
{
    iNodeModelPtr modelNode = dynamic_cast<iNodeModelPtr>(iNodeManager::getInstance().getNode(nodeID));
    if (modelNode)
    {
        modelNode->setMaterial(_materialWithInstancing);
    }
}

void GameLayer::onGenerateVoxelData(iVoxelBlockInfo *voxelBlockInfo)
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

                float64 contour = _perlinNoise.getValue(iaVector3d(pos._x * 0.0001, 0, pos._z * 0.0001), 3, 0.6);
                contour -= 0.7;

                if (contour > 0.0)
                {
                    contour *= 3;
                }

                float64 noise = _perlinNoise.getValue(iaVector3d(pos._x * 0.001, 0, pos._z * 0.001), 6, 0.45) * 0.15;
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

                float64 height = (noise * 5000) + s_waterLevel + 200;

                if (height < s_waterLevel - 100)
                {
                    height = s_waterLevel - 100;
                }

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

void GameLayer::onDeinit()
{
    iTaskManager::getInstance().abortTask(_taskFlushModels);
    iTaskManager::getInstance().abortTask(_taskFlushTextures);

    iMaterialResourceFactory::getInstance().destroyMaterial(_materialSkyBox);
    _materialSkyBox = iMaterial::INVALID_MATERIAL_ID;
    iMaterialResourceFactory::getInstance().destroyMaterial(_materialWithInstancing);
    _materialWithInstancing = iMaterial::INVALID_MATERIAL_ID;

    deinitVoxelData();

    iSceneFactory::getInstance().destroyScene(_scene);

    getWindow()->removeView(&_view);

    if (_plane != nullptr)
    {
        delete _plane;
        _plane = nullptr;
    }
}

bool GameLayer::onKeyDown(iEventKeyDown &event)
{
    if (_activeControls)
    {
        switch (event.getKey())
        {
        case iKeyCode::A:
            _plane->startRollLeft();
            return true;

        case iKeyCode::D:
            _plane->startRollRight();
            return true;

        case iKeyCode::S:
            _plane->startRollUp();
            return true;

        case iKeyCode::W:
            _plane->startRollDown();
            return true;
        }
    }

    switch (event.getKey())
    {
    case iKeyCode::ESC:
        iApplication::getInstance().stop();
        return true;

    case iKeyCode::Alt:
        _activeControls = !_activeControls;
        iMouse::getInstance().showCursor(!_activeControls);
        return true;
    }

    return false;
}

bool GameLayer::onKeyUp(iEventKeyUp &event)
{
    if (_activeControls)
    {
        switch (event.getKey())
        {
        case iKeyCode::OEMPlus:
            _plane->setThrustLevel(_plane->getThrustLevel() + 0.1);
            return true;

        case iKeyCode::OEM2:
            if (_plane->getThrustLevel() >= 0.0)
            {
                _plane->setThrustLevel(_plane->getThrustLevel() - 0.1);
            }
            return true;

        case iKeyCode::A:
            _plane->stopRollLeft();
            return true;

        case iKeyCode::D:
            _plane->stopRollRight();
            return true;

        case iKeyCode::S:
            _plane->stopRollUp();
            return true;

        case iKeyCode::W:
            _plane->stopRollDown();
            return true;
        }
    }

    return false;
}

bool GameLayer::onMouseWheelEvent(iEventMouseWheel &event)
{
    return false;
}

void GameLayer::playerCrashed()
{
    con_endl("crash");
    initPlayer();
}

void GameLayer::playerLanded()
{
    con_endl("land");
}

bool GameLayer::onMouseMoveEvent(iEventMouseMove &event)
{
    return false;
}

bool GameLayer::onMouseKeyDownEvent(iEventMouseKeyDown &event)
{
    return false;
}

bool GameLayer::onMouseKeyUpEvent(iEventMouseKeyUp &event)
{
    return false;
}

void GameLayer::deinitVoxelData()
{
    if (_voxelTerrain != nullptr)
    {
        delete _voxelTerrain;
        _voxelTerrain = nullptr;
    }
}

void GameLayer::onPreDraw()
{
    _hudLayer->setPosition(_plane->getPosition());
}

void GameLayer::onEvent(iEvent &event)
{
    event.dispatch<iEventKeyDown>(IGOR_BIND_EVENT_FUNCTION(GameLayer::onKeyDown));
    event.dispatch<iEventKeyUp>(IGOR_BIND_EVENT_FUNCTION(GameLayer::onKeyUp));
    event.dispatch<iEventMouseKeyUp>(IGOR_BIND_EVENT_FUNCTION(GameLayer::onMouseKeyUpEvent));
    event.dispatch<iEventMouseKeyDown>(IGOR_BIND_EVENT_FUNCTION(GameLayer::onMouseKeyDownEvent));
    event.dispatch<iEventMouseMove>(IGOR_BIND_EVENT_FUNCTION(GameLayer::onMouseMoveEvent));
    event.dispatch<iEventMouseWheel>(IGOR_BIND_EVENT_FUNCTION(GameLayer::onMouseWheelEvent));
}
