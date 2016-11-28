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
#include <iContouringCubes.h>
using namespace Igor;

#include <iaConsole.h>
using namespace IgorAux;

#include "TaskGenerateVoxels.h"

//#define FIX_POSITION
//#define FIX_HEIGHT
//#define WIREFRAME

float64 visibleDistance[] = { 150 * 150, 300 * 300, 700 * 700, 1500 * 1500, 3000 * 3000, 6000 * 6000, 12000 * 12000, 100000 * 100000 };

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

#ifdef WIREFRAME
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::CullFace, iRenderStateValue::Off);
    iMaterialResourceFactory::getInstance().getMaterial(_terrainMaterialID)->getRenderStateSet().setRenderState(iRenderState::Wireframe, iRenderStateValue::On);
#endif
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

    return result;
}

void VoxelTerrain::setVoxelDensity(iaVector3I voxelBlock, iaVector3I voxelRelativePos, uint8 density)
{
}

void VoxelTerrain::setVoxelDensity(iaVector3I pos, uint8 density)
{
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
    handleVoxelBlocks();
}

bool VoxelTerrain::loading()
{
    _runningTaskMutex.lock();
    bool result = _runningTasks.size() ? true : false;
    _runningTaskMutex.unlock();

    return result;
}

void VoxelTerrain::handleVoxelBlocks()
{
    static uint64 counter = 0;
    float64 lodFactor = pow(2, _lowestLOD);
    float64 actualBlockSize = VoxelBlock::_voxelBlockSize * lodFactor;
    iaVector3I voxelBlockPosition;

    iNodeLODTrigger* lodTrigger = static_cast<iNodeLODTrigger*>(iNodeFactory::getInstance().getNode(_lodTrigger));
    if (lodTrigger != nullptr)
    {
        iaVector3f pos = lodTrigger->getWorldPosition();
        if (pos.length2() < 1.0)
        {
            return;
        }

#ifdef FIX_POSITION
        pos.set(9986, 310, 8977);
#endif

#ifdef FIX_HEIGHT
        pos._y = 310;
#endif

        iaVector3d centerPos(pos._x, pos._y, pos._z);

        if (counter % 100 == 0)
        {
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
                            _voxelBlocks[voxelBlockPosition] = new VoxelBlock(_lowestLOD, voxelBlockPosition*actualBlockSize, iaVector3I());
                        }
                    }
                }
            }
        }

        for (auto block : _voxelBlocks)
        {
            update(block.second, centerPos);
            updateGeometry(block.second, centerPos);
            updateVisibility(block.second, centerPos);
            // todo need to remove voxel blocks from _voxelBlocks if not in use anymore
        }
    }
}

void VoxelTerrain::update(VoxelBlock* voxelBlock, iaVector3d observerPosition)
{
    if (voxelBlock->_stage == Stage::Empty)
    {
        return;
    }

    float64 distance = observerPosition.distance2(voxelBlock->_blockCenterPos);

    switch (voxelBlock->_stage)
    {
    case Stage::Initial:
        if (distance < visibleDistance[voxelBlock->_lod] * 1.5)
        {
            voxelBlock->_stage = Stage::Setup;
        }
        break;

    case Stage::Setup:
    {
        if (voxelBlock->_voxelBlockInfo == nullptr)
        {
            voxelBlock->_voxelData = new iVoxelData();
            voxelBlock->_voxelData->setMode(iaRLEMode::Compressed);
            voxelBlock->_voxelData->setClearValue(0);

            voxelBlock->_voxelBlockInfo = new VoxelBlockInfo();
            voxelBlock->_voxelBlockInfo->_size.set(VoxelBlock::_voxelBlockSize + VoxelBlock::_voxelBlockOverlap,
                VoxelBlock::_voxelBlockSize + VoxelBlock::_voxelBlockOverlap,
                VoxelBlock::_voxelBlockSize + VoxelBlock::_voxelBlockOverlap);
            voxelBlock->_voxelBlockInfo->_position = voxelBlock->_position;
            voxelBlock->_voxelBlockInfo->_voxelData = voxelBlock->_voxelData;
            voxelBlock->_voxelBlockInfo->_offset = iContouringCubes::calcLODOffset(voxelBlock->_lod);

            TaskGenerateVoxels* task = new TaskGenerateVoxels(voxelBlock->_voxelBlockInfo, voxelBlock->_lod, static_cast<uint32>(distance * 0.9));
            voxelBlock->_voxelGenerationTaskID = iTaskManager::getInstance().addTask(task);
        }

        voxelBlock->_stage = Stage::GeneratingVoxel;
    }
    break;

    case Stage::GeneratingVoxel:
    {
        iTask* task = iTaskManager::getInstance().getTask(voxelBlock->_voxelGenerationTaskID);
        if (task == nullptr)
        {
            voxelBlock->_voxelGenerationTaskID = iTask::INVALID_TASK_ID;
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
                        float64 halfSize = static_cast<float64>(voxelBlock->_size >> 1);
                        voxelBlock->_children[0] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position, iaVector3I(0, 0, 0));
                        voxelBlock->_children[1] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, 0, 0), iaVector3I(1, 0, 0));
                        voxelBlock->_children[2] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, 0, halfSize), iaVector3I(1, 0, 1));
                        voxelBlock->_children[3] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(0, 0, halfSize), iaVector3I(0, 0, 1));

                        voxelBlock->_children[4] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(0, halfSize, 0), iaVector3I(0, 1, 0));
                        voxelBlock->_children[5] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, halfSize, 0), iaVector3I(1, 1, 0));
                        voxelBlock->_children[6] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(halfSize, halfSize, halfSize), iaVector3I(1, 1, 1));
                        voxelBlock->_children[7] = new VoxelBlock(voxelBlock->_lod - 1, voxelBlock->_position + iaVector3I(0, halfSize, halfSize), iaVector3I(0, 1, 1));

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

    case Stage::Ready:
    {
        if (distance >= visibleDistance[voxelBlock->_lod] * 3.0)
        {
            /*iNodeFactory::getInstance().destroyNodeAsync(voxelBlock->_transformNodeID);
            voxelBlock->_transformNodeID = iNode::INVALID_NODE_ID;
            voxelBlock->_modelNodeID = iNode::INVALID_NODE_ID;

            for (uint32 nodeID : voxelBlock->_nodesToDestroy)
            {
                iNodeFactory::getInstance().destroyNodeAsync(nodeID);
            }
            voxelBlock->_nodesToDestroy.clear();

            voxelBlock->_stage = Stage::Initial;*/

            // TODO free voxel data here
            // TODO need a save way to delete voxel data. currently there is a problem with mesh generation that might have still jobs in the queue using the voxel data that we want to delete
            // TODO maybe we copy the data before we give it to mesh generation
            /* if (voxelBlock->_voxelData != nullptr)
            {
            delete voxelBlock->_voxelData;
            voxelBlock->_voxelData = nullptr;
            }

            if (voxelBlock->_voxelBlockInfo != nullptr)
            {
            delete voxelBlock->_voxelBlockInfo;
            voxelBlock->_voxelBlockInfo = nullptr;
            }*/
        }
        else
        {
            uint32 neighborsLOD = calcLODTransition(voxelBlock, observerPosition);
            if (voxelBlock->_neighborsLOD != neighborsLOD ||
                voxelBlock->_dirty)
            {
                iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDCurrent));
                if (modelNode != nullptr &&
                    modelNode->isLoaded())
                {
                    voxelBlock->_transformNodeIDQueued = iNode::INVALID_NODE_ID;
                    voxelBlock->_stage = Stage::GeneratingMesh;
                }
            }
        }
    }
    break;
    }

    if (voxelBlock->_children[0] != nullptr)
    {
        for (int i = 0; i < 8; ++i)
        {
            update(voxelBlock->_children[i], observerPosition);
        }
    }
}

void VoxelTerrain::updateGeometry(VoxelBlock* voxelBlock, iaVector3d observerPosition)
{
    if (voxelBlock->_stage != Stage::Empty)
    {
        if (voxelBlock->_stage == Stage::GeneratingMesh)
        {
            updateMesh(voxelBlock, observerPosition);
        }

        if (voxelBlock->_children[0] != nullptr)
        {
            for (int i = 0; i < 8; ++i)
            {
                updateGeometry(voxelBlock->_children[i], observerPosition);
            }
        }
    }
}

bool VoxelTerrain::updateVisibility(VoxelBlock* voxelBlock, iaVector3d observerPosition)
{
    bool childrenVisible = false;
    bool meshVisible = false;

    if (voxelBlock->_stage == Stage::Empty)
    {
        return true;
    }

    float64 distance = observerPosition.distance2(voxelBlock->_blockCenterPos);
    if (distance < visibleDistance[voxelBlock->_lod])
    {
        meshVisible = true;
    }

    if (voxelBlock->_lod > 0)
    {
        if (voxelBlock->_children[0] != nullptr)
        {
            childrenVisible = true;
            for (int i = 0; i < 8; ++i)
            {
                if (!updateVisibility(voxelBlock->_children[i], observerPosition))
                {
                    childrenVisible = false;
                }
            }

            if (!childrenVisible)
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (voxelBlock->_children[i]->_modelNodeIDCurrent != iNode::INVALID_NODE_ID)
                    {
                        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_children[i]->_modelNodeIDCurrent));
                        if (modelNode != nullptr &&
                            modelNode->isLoaded())
                        {
                            iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(voxelBlock->_children[i]->_transformNodeIDCurrent));
                            if (transformNode != nullptr)
                            {
                                transformNode->setActive(false);
                            }
                        }
                    }
                }
            }
            else
            {
                meshVisible = false;
            }
        }
    }

    if (voxelBlock->_modelNodeIDCurrent != iNode::INVALID_NODE_ID)
    {
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDCurrent));
        if (modelNode != nullptr &&
            modelNode->isLoaded())
        {
            iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(voxelBlock->_transformNodeIDCurrent));
            if (transformNode != nullptr)
            {
                transformNode->setActive(meshVisible);
            }
        }
        else
        {
            meshVisible = false;
        }
    }

    return (childrenVisible || meshVisible);
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

            if (observerPosition.distance2(childPos) >= visibleDistance[voxelBlock->_lod] * 1.1)
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

            if (observerPosition.distance2(childPos) >= visibleDistance[voxelBlock->_lod] * 1.1)
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

            if (observerPosition.distance2(childPos) >= visibleDistance[voxelBlock->_lod] * 1.1)
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

            if (observerPosition.distance2(childPos) >= visibleDistance[voxelBlock->_lod] * 1.1)
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

            if (observerPosition.distance2(childPos) >= visibleDistance[voxelBlock->_lod] * 1.1)
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

            if (observerPosition.distance2(childPos) >= visibleDistance[voxelBlock->_lod] * 1.1)
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
    if (voxelBlock->_transformNodeIDQueued == iNode::INVALID_NODE_ID)
    {
        if (voxelBlock->_voxelData != nullptr)
        {
            TileInformation tileInformation;
            tileInformation._materialID = _terrainMaterialID;
            tileInformation._voxelData = voxelBlock->_voxelData;

            if (voxelBlock->_parent != nullptr)
            {
                tileInformation._voxelOffsetToNextLOD.set(voxelBlock->_parentAdress._x * 16, voxelBlock->_parentAdress._y * 16, voxelBlock->_parentAdress._z * 16);
                tileInformation._voxelDataNextLOD = voxelBlock->_parent->_voxelData;
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

            iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
            modelNode->setModel(tileName, inputParam);

            transform->insertNode(modelNode);

            _scene->getRoot()->insertNode(transform);

            voxelBlock->_transformNodeIDQueued = transform->getID();
            voxelBlock->_modelNodeIDQueued = modelNode->getID();
        }
    }
    else if (voxelBlock->_modelNodeIDQueued != iNode::INVALID_NODE_ID)
    {
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDQueued));
        if (modelNode != nullptr &&
            modelNode->isLoaded())
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

            if (voxelBlock->_transformNodeIDCurrent != iNode::INVALID_NODE_ID)
            {
                iNodeFactory::getInstance().destroyNode(voxelBlock->_transformNodeIDCurrent);
            }

            voxelBlock->_transformNodeIDCurrent = voxelBlock->_transformNodeIDQueued;
            voxelBlock->_modelNodeIDCurrent = voxelBlock->_modelNodeIDQueued;
            voxelBlock->_modelNodeIDQueued = iNode::INVALID_NODE_ID;

            voxelBlock->_stage = Stage::Ready;
        }
    }
}
