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

float64 creationDistance[] = { 150 * 150, 300 * 300, 600 * 600, 1200 * 1200, 2500 * 2500, 5000 * 5000, 10000 * 10000, 100000 * 100000 };

VoxelTerrain::VoxelTerrain()
{
    unordered_map<iaVector3I, VoxelBlock*, VectorHasher, VectorEqualFn> voxelBlocks;
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
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::Off);
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::On);

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
    /* TODO 	for (auto blockIter : _voxelBlocks)
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

        con_endl("... done"); */
}

void VoxelTerrain::setLODTrigger(uint32 lodTriggerID)
{
    _lodTrigger = lodTriggerID;
}

uint8 VoxelTerrain::getVoxelDensity(iaVector3I pos)
{
    uint8 result = 0;

    /*	iaVector3I voxelBlock(pos);
        voxelBlock /= _voxelBlockSize;

        iaVector3I voxelRelativePos(pos);

        voxelRelativePos._x = voxelRelativePos._x % static_cast<int64>(_voxelBlockSize);
        voxelRelativePos._y = voxelRelativePos._y % static_cast<int64>(_voxelBlockSize);
        voxelRelativePos._z = voxelRelativePos._z % static_cast<int64>(_voxelBlockSize);

        VoxelBlockInfo* block = nullptr;

        auto blockIter = _voxelBlocks.find(voxelBlock);
        if (blockIter != _voxelBlocks.end())
        {
            block = _voxelBlocks[voxelBlock];
            if (block->_generatedVoxels)
            {
                result = block->_voxelData->getVoxelDensity(voxelRelativePos);
            }
        }*/

    return result;
}

void VoxelTerrain::setVoxelDensity(iaVector3I voxelBlock, iaVector3I voxelRelativePos, uint8 density)
{
    /*	auto blockIter = _voxelBlocks.find(voxelBlock);
        if (blockIter != _voxelBlocks.end())
        {
            VoxelBlockInfo* block = _voxelBlocks[voxelBlock];
            if (block->_generatedVoxels)
            {
                block->_voxelData->setVoxelDensity(voxelRelativePos, density);
            }
        }*/
}

void VoxelTerrain::setVoxelDensity(iaVector3I pos, uint8 density)
{
    /*	iaVector3I voxelBlock(pos);
        voxelBlock /= _voxelBlockSize;

        iaVector3I voxelRelativePos(pos);

        voxelRelativePos._x = voxelRelativePos._x % static_cast<int64>(_voxelBlockSize);
        voxelRelativePos._y = voxelRelativePos._y % static_cast<int64>(_voxelBlockSize);
        voxelRelativePos._z = voxelRelativePos._z % static_cast<int64>(_voxelBlockSize);

        VoxelBlockInfo* block = nullptr;

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
        }*/
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
    /*	auto tileIter = _tileDataSets.find(tilepos);
        if (tileIter != _tileDataSets.end())
        {
            (*tileIter).second._destroyNodeIDs.push_back((*tileIter).second._transformNodeID);
            (*tileIter).second._modelNodeID = iNode::INVALID_NODE_ID;
            (*tileIter).second._transformNodeID = iNode::INVALID_NODE_ID;
        }*/
}

void VoxelTerrain::handleMeshTiles(iVoxelData* voxelData, const iaVector3I& blockPos, iNodeLODTrigger* lodTrigger, const iaVector3I& lodTriggerPos, uint32 lod)
{
    /*	TileInformation tileInformation;
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
        }*/
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
    static uint64 counter = 0;

    if (counter++ % 1 == 0)
    {
        handleVoxelBlocks(_lowestLOD);
    }
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
    float64 actualBlockSize = VoxelBlock::_voxelBlockSize * lodFactor;
    iaVector3I voxelBlockPosition;
    VoxelBlock* voxelBlock = nullptr;

    iNodeLODTrigger* lodTrigger = static_cast<iNodeLODTrigger*>(iNodeFactory::getInstance().getNode(_lodTrigger));
    if (lodTrigger != nullptr)
    {
        iaVector3f pos = lodTrigger->getWorldPosition();
        if (pos.length2() < 1.0)
        {
            return;
        }
        pos.set(9243, 144, 16322);

        iaVector3I center(pos._x, pos._y, pos._z);
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

        if (start._y > 3)
        {
            start._y = 3;
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
                    voxelBlockPosition.set(voxelBlockX, voxelBlockY, voxelBlockZ);

                    auto blockIter = _voxelBlocks.find(voxelBlockPosition);
                    if (blockIter == _voxelBlocks.end())
                    {
                        voxelBlock = new VoxelBlock(lod, voxelBlockPosition*actualBlockSize);
                        _voxelBlocks[voxelBlockPosition] = voxelBlock;
                    }
                    else
                    {
                        voxelBlock = _voxelBlocks[voxelBlockPosition];
                    }

                    update(voxelBlock, iaVector3d(static_cast<float64>(pos._x), static_cast<float64>(pos._y), static_cast<float64>(pos._z)));
                }
            }
        }
    }
}

bool VoxelTerrain::update(VoxelBlock* voxelBlock, iaVector3d observerPosition)
{
    bool visible = true;

    if (voxelBlock->_stage == Stage::Empty)
    {
        return visible;
    }

    float64 halfSize = static_cast<float64>(voxelBlock->_size >> 1);

    iaVector3d blockCenterPos(static_cast<float64>(voxelBlock->_position._x) + halfSize,
        static_cast<float64>(voxelBlock->_position._y) + halfSize,
        static_cast<float64>(voxelBlock->_position._z) + halfSize);

    float64 distance = observerPosition.distance2(blockCenterPos);

    switch (voxelBlock->_stage)
    {
    case Stage::Initial:
    {
        visible = false;

        if (distance < creationDistance[voxelBlock->_lod])
        {
            if (voxelBlock->_voxelBlockInfo == nullptr)
            {
                voxelBlock->_voxelData = new iVoxelData();
                voxelBlock->_voxelData->setMode(iaRLEMode::Compressed);
                voxelBlock->_voxelData->setClearValue(0);

                voxelBlock->_voxelBlockInfo = new VoxelBlockInfo();
                voxelBlock->_voxelBlockInfo->_size.set(voxelBlock->_voxelBlockSize + voxelBlock->_voxelBlockOverlap, voxelBlock->_voxelBlockSize + voxelBlock->_voxelBlockOverlap, voxelBlock->_voxelBlockSize + voxelBlock->_voxelBlockOverlap);
                voxelBlock->_voxelBlockInfo->_position = voxelBlock->_position;
                voxelBlock->_voxelBlockInfo->_offset = voxelBlock->_offset;
                voxelBlock->_voxelBlockInfo->_voxelData = voxelBlock->_voxelData;

                TaskGenerateVoxels* task = new TaskGenerateVoxels(voxelBlock->_voxelBlockInfo, voxelBlock->_lod, static_cast<uint32>(distance * 0.9));
                voxelBlock->_taskID = iTaskManager::getInstance().addTask(task);
            }

            voxelBlock->_stage = Stage::GeneratingVoxel;
        }
    }
    break;

    case Stage::GeneratingVoxel:
    {
        visible = false;

        iTask* task = iTaskManager::getInstance().getTask(voxelBlock->_taskID);
        if (task == nullptr)
        {
            voxelBlock->_taskID = iTask::INVALID_TASK_ID;
            if (!voxelBlock->_voxelBlockInfo->_transition)
            {
                delete voxelBlock->_voxelData;
                voxelBlock->_voxelData = nullptr;
                voxelBlock->_voxelBlockInfo->_voxelData = nullptr;

                voxelBlock->_stage = Stage::Empty;
            }
            else
            {
                if (voxelBlock->_lod > 0)
                {
                    if (voxelBlock->_children[0] == nullptr)
                    {
                        voxelBlock->_children[0] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position);
                        voxelBlock->_children[1] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, 0, 0));
                        voxelBlock->_children[2] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, 0, halfSize));
                        voxelBlock->_children[3] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(0, 0, halfSize));

                        voxelBlock->_children[4] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(0, halfSize, 0));
                        voxelBlock->_children[5] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, halfSize, 0));
                        voxelBlock->_children[6] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, halfSize, halfSize));
                        voxelBlock->_children[7] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(0, halfSize, halfSize));

                        voxelBlock->_children[0]->_parent = voxelBlock;
                        voxelBlock->_children[1]->_parent = voxelBlock;
                        voxelBlock->_children[2]->_parent = voxelBlock;
                        voxelBlock->_children[3]->_parent = voxelBlock;
                        voxelBlock->_children[4]->_parent = voxelBlock;
                        voxelBlock->_children[5]->_parent = voxelBlock;
                        voxelBlock->_children[6]->_parent = voxelBlock;
                        voxelBlock->_children[7]->_parent = voxelBlock;
                    }
                }

                voxelBlock->_stage = Stage::GeneratingMesh;
            }
        }
    }
    break;

    case Stage::GeneratingMesh:
    {
        visible = false;

        if (distance < creationDistance[voxelBlock->_lod])
        {
            updateMesh(voxelBlock, observerPosition);
        }
    }
    break;

    case Stage::Ready:
    {
        visible = false;
        bool meshVisible = true;
        bool destroy = false;

        if (voxelBlock->_lod > 0)
        {
            bool childrenVisible = true;
            for (int i = 0; i < 8; ++i)
            {
                if (!update(voxelBlock->_children[i], observerPosition))
                {
                    childrenVisible = false;
                }
            }

            if (childrenVisible)
            {
                visible = true;
                meshVisible = false;
            }
            else
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (voxelBlock->_children[i]->_stage == Stage::Ready)
                    {
                        iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(voxelBlock->_children[i]->_transformNodeID));
                        if (transformNode != nullptr)
                        {
                            transformNode->setActive(false);
                        }
                    }
                }
            }
        }

        if (distance >= creationDistance[voxelBlock->_lod])
        {
            meshVisible = false;

            if (distance >= creationDistance[voxelBlock->_lod] * 3.0)
            {
                destroy = true;
            }
        }

        uint32 neighborsLOD = calcLODTransition(voxelBlock, observerPosition);
        if (voxelBlock->_neighborsLOD != neighborsLOD)
        {
            destroy = true;
        }

        iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(voxelBlock->_transformNodeID));
        if (transformNode != nullptr)
        {
            transformNode->setActive(meshVisible);
        }

        if (destroy)
        {
            iNodeFactory::getInstance().destroyNode(voxelBlock->_transformNodeID);
            voxelBlock->_transformNodeID = iNode::INVALID_NODE_ID;
            voxelBlock->_modelNodeID = iNode::INVALID_NODE_ID;
            voxelBlock->_stage = Stage::Initial;

            if (voxelBlock->_voxelData != nullptr)
            {
                delete voxelBlock->_voxelData;
                voxelBlock->_voxelData = nullptr;
            }

            if (voxelBlock->_voxelBlockInfo != nullptr)
            {
                delete voxelBlock->_voxelBlockInfo;
                voxelBlock->_voxelBlockInfo = nullptr;
            }

            visible = false;
        }
        else if (meshVisible)
        {
            iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeID));
            if (modelNode != nullptr)
            {
                iNode* group = static_cast<iNodeMesh*>(modelNode->getChild("group"));
                if (group != nullptr)
                {
                    iNodeMesh* meshNode = static_cast<iNodeMesh*>(group->getChild("mesh"));
                    if (meshNode != nullptr)
                    {
                        meshNode->setVisible(true);
                    }
                }
            }

            visible = true;
        }
    }
    break;
    }

    return visible;
}

#define NEIGHBOR_XPOSITIVE 0x20
#define NEIGHBOR_XNEGATIVE 0x10
#define NEIGHBOR_YPOSITIVE 0x08
#define NEIGHBOR_YNEGATIVE 0x04
#define NEIGHBOR_ZPOSITIVE 0x02
#define NEIGHBOR_ZNEGATIVE 0x01

uint32 VoxelTerrain::calcLODTransition(VoxelBlock* voxelBlock, iaVector3d observerPosition)
{
    uint32 result = 0;

    if (voxelBlock->_lod >= _lowestLOD)
    {
        return result;
    }

    iaVector3d halfSize(static_cast<float64>(voxelBlock->_size >> 1), static_cast<float64>(voxelBlock->_size >> 1), static_cast<float64>(voxelBlock->_size >> 1));

    vector<iaVector3d> centers;
    centers.push_back(halfSize);
    centers.push_back(halfSize + iaVector3d(voxelBlock->_size, 0, 0));
    centers.push_back(halfSize + iaVector3d(voxelBlock->_size, 0, voxelBlock->_size));
    centers.push_back(halfSize + iaVector3d(0, 0, voxelBlock->_size));
    centers.push_back(halfSize + iaVector3d(0, voxelBlock->_size, 0));
    centers.push_back(halfSize + iaVector3d(voxelBlock->_size, voxelBlock->_size, 0));
    centers.push_back(halfSize + iaVector3d(voxelBlock->_size, voxelBlock->_size, voxelBlock->_size));
    centers.push_back(halfSize + iaVector3d(0, voxelBlock->_size, voxelBlock->_size));

    uint64 parentSize = voxelBlock->_size << 1;
    iaVector3I parentPosInt = voxelBlock->_position;
    parentPosInt /= parentSize;
    parentPosInt *= parentSize;

    iaVector3d parentPos(static_cast<float64>(parentPosInt._x), static_cast<float64>(parentPosInt._y), static_cast<float64>(parentPosInt._z));

    iaVector3I relativeToParent = voxelBlock->_position;
    relativeToParent._x = relativeToParent._x % parentSize;
    relativeToParent._y = relativeToParent._y % parentSize;
    relativeToParent._z = relativeToParent._z % parentSize;

    if (relativeToParent._x > 0)
    {
        iaVector3d nextParentPos = parentPos;
        nextParentPos._x += parentSize;

        iaVector3d childPos;

        for (int i = 0; i < 8; ++i)
        {
            childPos = nextParentPos;
            childPos += centers[i];

            if (observerPosition.distance2(childPos) >= creationDistance[voxelBlock->_lod])
            {
                result |= NEIGHBOR_XPOSITIVE;
                break;
            }
        }
    }
     else
     {
         iaVector3d nextParentPos = parentPos;
         nextParentPos._x -= parentSize;

         iaVector3d childPos;

         for (int i = 0; i < 8; ++i)
         {
             childPos = nextParentPos;
             childPos += centers[i];

             if (observerPosition.distance2(childPos) >= creationDistance[voxelBlock->_lod])
             {
                 result |= NEIGHBOR_XNEGATIVE;
                 break;
             }
         }
     }

      if (relativeToParent._y > 0)
      {
          iaVector3d nextParentPos = parentPos;
          nextParentPos._y += parentSize;

          iaVector3d childPos;

          for (int i = 0; i < 8; ++i)
          {
              childPos = nextParentPos;
              childPos += centers[i];

              if (observerPosition.distance2(childPos) >= creationDistance[voxelBlock->_lod])
              {
                  result |= NEIGHBOR_YPOSITIVE;
                  break;
              }
          }
      }
      else
      {
          iaVector3d nextParentPos = parentPos;
          nextParentPos._y -= parentSize;

          iaVector3d childPos;

          for (int i = 0; i < 8; ++i)
          {
              childPos = nextParentPos;
              childPos += centers[i];

              if (observerPosition.distance2(childPos) >= creationDistance[voxelBlock->_lod])
              {
                  result |= NEIGHBOR_YNEGATIVE;
                  break;
              }
          }
      }

      if (relativeToParent._z > 0)
      {
          iaVector3d nextParentPos = parentPos;
          nextParentPos._z += parentSize;

          iaVector3d childPos;

          for (int i = 0; i < 8; ++i)
          {
              childPos = nextParentPos;
              childPos += centers[i];

              if (observerPosition.distance2(childPos) >= creationDistance[voxelBlock->_lod])
              {
                  result |= NEIGHBOR_ZPOSITIVE;
                  break;
              }
          }
      }
      else
      {
          iaVector3d nextParentPos = parentPos;
          nextParentPos._z -= parentSize;

          iaVector3d childPos;

          for (int i = 0; i < 8; ++i)
          {
              childPos = nextParentPos;
              childPos += centers[i];

              if (observerPosition.distance2(childPos) >= creationDistance[voxelBlock->_lod])
              {
                  result |= NEIGHBOR_ZNEGATIVE;
                  break;
              }
          }
      }


    return result;
}

void VoxelTerrain::updateMesh(VoxelBlock* voxelBlock, iaVector3d observerPosition)
{
    if (voxelBlock->_transformNodeID == iNode::INVALID_NODE_ID)
    {
        if (voxelBlock->_voxelData != nullptr)
        {
            TileInformation tileInformation;
            tileInformation._materialID = _terrainMaterialID;
            tileInformation._voxelData = voxelBlock->_voxelData;

            if (voxelBlock->_parent != nullptr)
            {
                tileInformation._voxelDataNextLOD = voxelBlock->_parent->_voxelData;
                tileInformation._offsetToNextLOD = voxelBlock->_offset - voxelBlock->_parent->_offset;
            }

            tileInformation._lod = voxelBlock->_lod;
            tileInformation._neighborsLOD = calcLODTransition(voxelBlock, observerPosition);
            voxelBlock->_neighborsLOD = tileInformation._neighborsLOD;

            iModelDataInputParameter* inputParam = new iModelDataInputParameter(); // will be deleted by iModel
            inputParam->_identifier = "vtg";
            inputParam->_joinVertexes = true;
            inputParam->_needsRenderContext = false;
            inputParam->_modelSourceType = iModelSourceType::Generated;
            inputParam->_loadPriority = 0;
            inputParam->_parameters.setData(reinterpret_cast<const char*>(&tileInformation), sizeof(TileInformation));

            iaString tileName = iaString::itoa(voxelBlock->_position._x);
            tileName += ":";
            tileName += iaString::itoa(voxelBlock->_position._y);
            tileName += ":";
            tileName += iaString::itoa(voxelBlock->_position._z);
            tileName += ":";
            tileName += iaString::itoa(voxelBlock->_lod);
            tileName += ":";
            tileName += iaString::itoa(voxelBlock->_mutationCounter++);

            iNodeTransform* transform = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));

            transform->translate(voxelBlock->_position._x, voxelBlock->_position._y, voxelBlock->_position._z);
            transform->translate(voxelBlock->_offset);

            iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
            modelNode->setModel(tileName, inputParam);

            transform->insertNode(modelNode);

            _scene->getRoot()->insertNode(transform);

            voxelBlock->_transformNodeID = transform->getID();
            voxelBlock->_modelNodeID = modelNode->getID();
        }
    }
    else
    {
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeID));
        if (modelNode != nullptr &&
            modelNode->isLoaded())
        {
            voxelBlock->_stage = Stage::Ready;
        }
    }
}
