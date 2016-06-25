#include "VoxelTerrain.h"

#include <iNodeLODTrigger.h>
#include <iNodeFactory.h>
#include <iApplication.h>
#include <iVoxelData.h>
#include <iTaskManager.h>
#include <iModelResourceFactory.h>
#include <iNodeTransform.h>
#include <iNodeLODSwitch.h>
#include <iNodeModel.h>
#include <iNodeMesh.h>
#include <iScene.h>
#include <iMaterialResourceFactory.h>
#include <iMaterial.h>
#include <iPhysics.h>
#include <iPhysicsBody.h>
#include <iMesh.h>
#include <iStatistics.h>
using namespace Igor;

#include <iaConsole.h>
using namespace IgorAux;

#include "TaskGenerateVoxels.h"

VoxelTerrain::VoxelTerrain()
{

}

VoxelTerrain::~VoxelTerrain()
{
    deinit();
}

void VoxelTerrain::setScene(iScene* scene)
{
    con_assert(scene != nullptr, "zero pointer");
    _scene = scene;

    init();
}

void VoxelTerrain::init()
{
    iApplication::getInstance().registerApplicationHandleDelegate(iApplicationHandleDelegate(this, &VoxelTerrain::onHandle));

    iModelResourceFactory::getInstance().registerModelDataIO("vtg", &VoxelTerrainMeshGenerator::createInstance);

    // set up terrain material
    _terrainMaterialID = iMaterialResourceFactory::getInstance().createMaterial("TerrainMaterial");
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->addShaderSource("terrain.vert", iShaderObjectType::Vertex);
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->addShaderSource("terrain_directional_light.frag", iShaderObjectType::Fragment);
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->compileShader();
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::Texture2D0, iRenderStateValue::On);
    //iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::Off);
    //iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::On);

    iTaskManager::getInstance().registerTaskFinishedDelegate(iTaskFinishedDelegate(this, &VoxelTerrain::onTaskFinished));
}

void VoxelTerrain::deinit()
{
    con_endl("shutdown VoxelTerrain ...");

    iModelResourceFactory::getInstance().unregisterModelDataIO("vtg");

    iApplication::getInstance().unregisterApplicationHandleDelegate(iApplicationHandleDelegate(this, &VoxelTerrain::onHandle));

    con_endl("waiting for some tasks ...");
    while (loading())
    {
        _sleep(1000);
    }

    iTaskManager::getInstance().unregisterTaskFinishedDelegate(iTaskFinishedDelegate(this, &VoxelTerrain::onTaskFinished));

    con_endl("clear blocks ...");
    for (auto blockIter : _voxelBlocks)
    {
        if (blockIter.second != nullptr)
        {
            if (blockIter.second->_generatedVoxels &&
                blockIter.second->_voxelData != nullptr)
            {
                delete blockIter.second->_voxelData;
            }

            delete blockIter.second;
        }
    }
    _voxelBlocks.clear();

    con_endl("clear tiles ...");
    for (auto tile : _tileDataSets)
    {
        if (tile.second._transformNodeID != iNode::INVALID_NODE_ID)
        {
            iNodeFactory::getInstance().destroyNode(tile.second._transformNodeID);
        }

        for (auto id : tile.second._destroyNodeIDs)
        {
            iNodeFactory::getInstance().destroyNode(id);
        }
    }

    con_endl("... done");
}

void VoxelTerrain::setLODTrigger(uint32 lodTriggerID)
{
    _lodTrigger = lodTriggerID;
}

uint8 VoxelTerrain::getVoxelDensity(iaVector3I pos)
{
    uint8 result = 0;

    iaVector3I voxelBlock(pos);
    voxelBlock /= _voxelBlockSize;

    iaVector3I voxelRelativePos(pos);

    voxelRelativePos._x = voxelRelativePos._x % static_cast<int64>(_voxelBlockSize);
    voxelRelativePos._y = voxelRelativePos._y % static_cast<int64>(_voxelBlockSize);
    voxelRelativePos._z = voxelRelativePos._z % static_cast<int64>(_voxelBlockSize);

    VoxelBlock* block = nullptr;

    auto blockIter = _voxelBlocks.find(voxelBlock);
    if (blockIter != _voxelBlocks.end())
    {
        block = _voxelBlocks[voxelBlock];
        if (block->_generatedVoxels)
        {
            result = block->_voxelData->getVoxelDensity(voxelRelativePos);
        }
    }

    return result;
}

void VoxelTerrain::setVoxelDensity(iaVector3I voxelBlock, iaVector3I voxelRelativePos, uint8 density)
{
    auto blockIter = _voxelBlocks.find(voxelBlock);
    if (blockIter != _voxelBlocks.end())
    {
        VoxelBlock* block = _voxelBlocks[voxelBlock];
        if (block->_generatedVoxels)
        {
            block->_voxelData->setVoxelDensity(voxelRelativePos, density);
        }
    }
}

void VoxelTerrain::setVoxelDensity(iaVector3I pos, uint8 density)
{
    iaVector3I voxelBlock(pos);
    voxelBlock /= _voxelBlockSize;

    iaVector3I voxelRelativePos(pos);

    voxelRelativePos._x = voxelRelativePos._x % static_cast<int64>(_voxelBlockSize);
    voxelRelativePos._y = voxelRelativePos._y % static_cast<int64>(_voxelBlockSize);
    voxelRelativePos._z = voxelRelativePos._z % static_cast<int64>(_voxelBlockSize);

    VoxelBlock* block = nullptr;

    auto blockIter = _voxelBlocks.find(voxelBlock);
    if (blockIter != _voxelBlocks.end())
    {
        block = _voxelBlocks[voxelBlock];
        if (block->_generatedVoxels)
        {
            block->_voxelData->setVoxelDensity(voxelRelativePos, density);
        }
    }

    if (voxelRelativePos._x < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockX(voxelBlock);
        voxelBlockX._x--;
        iaVector3I voxelRelativePosX(voxelRelativePos);
        voxelRelativePosX._x += _voxelBlockSize;
        setVoxelDensity(voxelBlockX, voxelRelativePosX, density);
    }

    if (voxelRelativePos._y < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockY(voxelBlock);
        voxelBlockY._y--;
        iaVector3I voxelRelativePosY(voxelRelativePos);
        voxelRelativePosY._y += _voxelBlockSize;
        setVoxelDensity(voxelBlockY, voxelRelativePosY, density);
    }

    if (voxelRelativePos._z < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockZ(voxelBlock);
        voxelBlockZ._z--;
        iaVector3I voxelRelativePosZ(voxelRelativePos);
        voxelRelativePosZ._z += _voxelBlockSize;
        setVoxelDensity(voxelBlockZ, voxelRelativePosZ, density);
    }

    if (voxelRelativePos._x < _voxelBlockOverlap &&
        voxelRelativePos._y < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockXY(voxelBlock);
        voxelBlockXY._x--;
        voxelBlockXY._y--;
        iaVector3I voxelRelativePosXY(voxelRelativePos);
        voxelRelativePosXY._x += _voxelBlockSize;
        voxelRelativePosXY._y += _voxelBlockSize;
        setVoxelDensity(voxelBlockXY, voxelRelativePosXY, density);
    }

    if (voxelRelativePos._x < _voxelBlockOverlap &&
        voxelRelativePos._z < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockXZ(voxelBlock);
        voxelBlockXZ._x--;
        voxelBlockXZ._z--;
        iaVector3I voxelRelativePosXZ(voxelRelativePos);
        voxelRelativePosXZ._x += _voxelBlockSize;
        voxelRelativePosXZ._z += _voxelBlockSize;
        setVoxelDensity(voxelBlockXZ, voxelRelativePosXZ, density);
    }

    if (voxelRelativePos._y < _voxelBlockOverlap &&
        voxelRelativePos._z < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockYZ(voxelBlock);
        voxelBlockYZ._y--;
        voxelBlockYZ._z--;
        iaVector3I voxelRelativePosYZ(voxelRelativePos);
        voxelRelativePosYZ._y += _voxelBlockSize;
        voxelRelativePosYZ._z += _voxelBlockSize;
        setVoxelDensity(voxelBlockYZ, voxelRelativePosYZ, density);
    }

    if (voxelRelativePos._x < _voxelBlockOverlap &&
        voxelRelativePos._y < _voxelBlockOverlap &&
        voxelRelativePos._z < _voxelBlockOverlap)
    {
        iaVector3I voxelBlockXYZ(voxelBlock);
        voxelBlockXYZ._x--;
        voxelBlockXYZ._y--;
        voxelBlockXYZ._z--;
        iaVector3I voxelRelativePosXYZ(voxelRelativePos);
        voxelRelativePosXYZ._x += _voxelBlockSize;
        voxelRelativePosXYZ._y += _voxelBlockSize;
        voxelRelativePosXYZ._z += _voxelBlockSize;
        setVoxelDensity(voxelBlockXYZ, voxelRelativePosXYZ, density);
    }
}

void VoxelTerrain::castRay(const iaVector3I& from, const iaVector3I& to, iaVector3I& outside, iaVector3I& inside)
{
    iaVector3I u(from);
    iaVector3I delta(to);
    delta -= from;
    iaVector3I step;

    (delta._x > 0) ? step._x = 1 : step._x = -1;
    (delta._y > 0) ? step._y = 1 : step._y = -1;
    (delta._z > 0) ? step._z = 1 : step._z = -1;

    delta._x = abs(delta._x);
    delta._y = abs(delta._y);
    delta._z = abs(delta._z);

    int64 dist = (delta._x > delta._y) ? delta._x : delta._y;
    dist = (dist > delta._z) ? dist : delta._z;

    iaVector3I err(delta._x, delta._y, delta._z);

    outside = u;

    for (int i = 0; i < dist; i++)
    {
        if (getVoxelDensity(iaVector3I(u._x, u._y, u._z)) != 0)
        {
            inside = u;
            return;
        }

        outside = u;

        err += delta;

        if (err._x > dist)
        {
            err._x -= dist;
            u._x += step._x;
        }

        if (err._y > dist)
        {
            err._y -= dist;
            u._y += step._y;
        }

        if (err._z > dist)
        {
            err._z -= dist;
            u._z += step._z;
        }
    }
}

#define array3D(x, y, z, f) ((x) + (f) * ((y) + (f) * (z)))

void VoxelTerrain::refreshTile(iaVector3I tilepos)
{
    auto tileIter = _tileDataSets.find(tilepos);
    if (tileIter != _tileDataSets.end())
    {
        (*tileIter).second._destroyNodeIDs.push_back((*tileIter).second._transformNodeID);
        (*tileIter).second._modelNodeID = iNode::INVALID_NODE_ID;
        (*tileIter).second._transformNodeID = iNode::INVALID_NODE_ID;
    }
}

void VoxelTerrain::handleMeshTiles(iVoxelData* voxelData, const iaVector3I& blockPos, iNodeLODTrigger* lodTrigger, const iaVector3I& lodTriggerPos, uint32 lod)
{
    TileInformation tileInformation;
    tileInformation._materialID = _terrainMaterialID;
    tileInformation._voxelData = voxelData;
    tileInformation._lodTriggerID = lodTrigger->getID();
	tileInformation._lod = lod;

    iaVector3I tilePos = blockPos;
    iaVector3I tileIndexPos = tilePos;
    tileIndexPos /= _voxelBlockSize;

    float32 distance = lodTriggerPos.distance2(tilePos);

    auto tileIter = _tileDataSets.find(tileIndexPos);
    if (tileIter == _tileDataSets.end())
    {
        TileData tileData;
        tileData._transformNodeID = iNode::INVALID_NODE_ID;
        tileData._modelNodeID = iNode::INVALID_NODE_ID;
        _tileDataSets[tileIndexPos] = tileData;

        tileIter = _tileDataSets.find(tileIndexPos);
    }

    if (distance < _tileCreationDistance &&
        (*tileIter).second._transformNodeID == iNode::INVALID_NODE_ID)
    {
        tileInformation._width = _voxelBlockSize + _tileOverlap;
        tileInformation._depth = _voxelBlockSize + _tileOverlap;
        tileInformation._height = _voxelBlockSize + _tileOverlap;

        iModelDataInputParameter* inputParam = new iModelDataInputParameter(); // will be deleted by iModel
        inputParam->_identifier = "vtg";
        inputParam->_joinVertexes = true;
        inputParam->_needsRenderContext = false;
        inputParam->_modelSourceType = iModelSourceType::Generated;
        inputParam->_loadPriority = static_cast<uint32>(distance);
        inputParam->_parameters.setData(reinterpret_cast<const char*>(&tileInformation), sizeof(TileInformation));

        iaString tileName = iaString::itoa(tilePos._x);
        tileName += ":";
        tileName += iaString::itoa(tilePos._y);
        tileName += ":";
        tileName += iaString::itoa(tilePos._z);
        tileName += ":";
        tileName += iaString::itoa((*tileIter).second._mutationCounter++);

        iNodeTransform* transform = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
        transform->translate(tilePos._x, tilePos._y, tilePos._z);

        iNodeLODSwitch* lodSwitch = static_cast<iNodeLODSwitch*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeLODSwitch));
        lodSwitch->addTrigger(lodTrigger);

        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
        modelNode->setModel(tileName, inputParam);

        transform->insertNode(lodSwitch);
        lodSwitch->insertNode(modelNode);

        lodSwitch->setThresholds(modelNode, 0.0f, _tileVisualDistance);
        _scene->getRoot()->insertNode(transform);

        (*tileIter).second._transformNodeID = transform->getID();
        (*tileIter).second._modelNodeID = modelNode->getID();
    }

    if (!(*tileIter).second._destroyNodeIDs.empty())
    {
        bool destroy = false;
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode((*tileIter).second._modelNodeID));
        if (modelNode != nullptr)
        {
            if (modelNode->isLoaded())
            {
                destroy = true;
            }
        }
        else
        {
            destroy = true;
        }

        if (destroy)
        {
            for (auto id : (*tileIter).second._destroyNodeIDs)
            {
                iNode* node = iNodeFactory::getInstance().getNode(id);
                if (node != nullptr)
                {
                    iNodeFactory::getInstance().destroyNode(node);
                }
            }

            (*tileIter).second._destroyNodeIDs.clear();
        }
    }

    if (distance > _tileDestructionDistance)
    {
        if ((*tileIter).second._transformNodeID != iNode::INVALID_NODE_ID)
        {
            iNodeFactory::getInstance().destroyNode((*tileIter).second._transformNodeID);
        }

        if (!(*tileIter).second._destroyNodeIDs.empty())
        {
            for (auto id : (*tileIter).second._destroyNodeIDs)
            {
                iNodeFactory::getInstance().destroyNode(id);
            }

            (*tileIter).second._destroyNodeIDs.clear();
        }

        _tileDataSets.erase(tileIter);
    }
}

void VoxelTerrain::registerVoxelDataGeneratedDelegate(VoxelDataGeneratedDelegate voxelDataGeneratedDelegate)
{
    _dataGeneratedEvent.append(voxelDataGeneratedDelegate);
}

void VoxelTerrain::unregisterVoxelDataGeneratedDelegate(VoxelDataGeneratedDelegate voxelDataGeneratedDelegate)
{
    _dataGeneratedEvent.remove(voxelDataGeneratedDelegate);
}

void VoxelTerrain::onHandle()
{
    handleVoxelBlocks(0);
}

void VoxelTerrain::onTaskFinished(uint64 taskID)
{
    _runningTaskMutex.lock();
    auto task = find(_runningTasks.begin(), _runningTasks.end(), taskID);
    if (task != _runningTasks.end())
    {
        _runningTasks.erase(task);
    }
    _runningTaskMutex.unlock();
}

bool VoxelTerrain::loading()
{
    _runningTaskMutex.lock();
    bool result = _runningTasks.size() ? true : false;
    _runningTaskMutex.unlock();

    return result;
}

void VoxelTerrain::handleVoxelBlocks(uint32 lod)
{
	float64 lodFactor = pow(2, lod);
	float64 actualBlockSize = _voxelBlockSize * lodFactor;
    iaVector3I currentVoxelBlock;
    VoxelBlock* block = nullptr;

    iNodeLODTrigger* lodTrigger = static_cast<iNodeLODTrigger*>(iNodeFactory::getInstance().getNode(_lodTrigger));
    if (lodTrigger != nullptr)
    {
        iaVector3f pos = lodTrigger->getWorldPosition();
        iaVector3I lodTriggerPos(pos._x, pos._y, pos._z);

        iaVector3I center(lodTriggerPos._x, lodTriggerPos._y, lodTriggerPos._z);
        center /= actualBlockSize;

        iaVector3I start(center);
        start._x -= _voxelBlockScanDistance;
        start._y -= _voxelBlockScanDistance;
        start._z -= _voxelBlockScanDistance;

        iaVector3I stop(center);
        stop._x += _voxelBlockScanDistance;
        stop._y += _voxelBlockScanDistance;
        stop._z += _voxelBlockScanDistance;

        if (start._x < 0)
        {
            start._x = 0;
        }

        if (start._y < 0)
        {
            start._y = 0;
        }

        if (stop._y > 3)
        {
            stop._y = 3;
        }

        if (start._z < 0)
        {
            start._z = 0;
        }

        for (int64 voxelBlockX = start._x; voxelBlockX < stop._x; ++voxelBlockX)
        {
            for (int64 voxelBlockY = start._y; voxelBlockY < stop._y; ++voxelBlockY)
            {
                for (int64 voxelBlockZ = start._z; voxelBlockZ < stop._z; ++voxelBlockZ)
                {
                    currentVoxelBlock.set(voxelBlockX, voxelBlockY, voxelBlockZ);

                    auto blockIter = _voxelBlocks.find(currentVoxelBlock);
                    if (blockIter == _voxelBlocks.end())
                    {
                        block = new VoxelBlock();
                        _voxelBlocks[currentVoxelBlock] = block;
                    }
                    else
                    {
                        block = _voxelBlocks[currentVoxelBlock];
                    }

                    iaVector3I blockPos(voxelBlockX, voxelBlockY, voxelBlockZ);
                    blockPos *= actualBlockSize;

                    if (block->_voxelData == nullptr)
                    {
                        iaVector3I blockCenterPos = blockPos;
                        blockCenterPos._x += actualBlockSize / 2;
                        blockCenterPos._y += actualBlockSize / 2;
                        blockCenterPos._z += actualBlockSize / 2;

                        float32 distance = lodTriggerPos.distance2(blockCenterPos);
                        if (distance < _voxelBlockCreationDistance)
                        {
                            block->_voxelData = new iVoxelData();
                            block->_voxelData->setMode(iaRLEMode::Uncompressed);
                            block->_voxelData->setClearValue(0);
                            block->_position = blockPos;
                            block->_size.set(_voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap);

                            TaskGenerateVoxels* task = new TaskGenerateVoxels(block, lod, static_cast<uint32>(distance * 0.9));
                            _runningTaskMutex.lock();
                            _runningTasks.push_back(task->getID());
                            _runningTaskMutex.unlock();
                            iTaskManager::getInstance().addTask(task);
                        }
                    }
                    else if (block->_generatedVoxels)
                    {
                        if (block->_changedVoxels)
                        {
                            handleMeshTiles(block->_voxelData, blockPos, lodTrigger, lodTriggerPos, lod);

                            if (lod == 0 && 
								!block->_generatedEnemies)
                            {
                                iaVector3I blockMax = blockPos;
                                blockMax._x += _voxelBlockSize;
                                blockMax._y += _voxelBlockSize;
                                blockMax._z += _voxelBlockSize;
                                _dataGeneratedEvent(blockPos, blockMax);
                                block->_generatedEnemies = true;
                            }
                        }
                    }
                }
            }
        }
    }
}
