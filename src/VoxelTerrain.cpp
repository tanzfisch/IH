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

const float64 VoxelTerrain::_visibleDistance[] = { 150 * 150, 300 * 300, 700 * 700, 1500 * 1500, 3000 * 3000, 6000 * 6000, 12000 * 12000, 100000 * 100000 };

VoxelTerrain::VoxelTerrain()
{
    unordered_map<iaVector3I, VoxelBlock*, VectorHasher, VectorEqualFn> voxelBlocks;

    for (int i = 0; i < _lowestLOD + 1; ++i)
    {
        _voxelBlocks.push_back(voxelBlocks);
    }

    _discoverBlocksSection = iStatistics::getInstance().registerSection("VT:discover", 3);
    _updateBlocksSection = iStatistics::getInstance().registerSection("VT:update", 3);
}

VoxelTerrain::~VoxelTerrain()
{
    deinit();
}

void VoxelTerrain::setScene(iScene* scene)
{
    con_assert(scene != nullptr, "zero pointer");
    con_assert(_rootNode == nullptr, "already initialized");

    if (scene != nullptr &&
        _rootNode == nullptr)
    {
        init(scene);
    }
}

void VoxelTerrain::init(iScene* scene)
{
    _rootNode = iNodeFactory::getInstance().createNode(iNodeType::iNode);
    scene->getRoot()->insertNode(_rootNode);

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

    // TODO cleanup
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

void VoxelTerrain::onHandle()
{
    handleVoxelBlocks();
}

void VoxelTerrain::handleVoxelBlocks()
{
    iNodeLODTrigger* lodTrigger = static_cast<iNodeLODTrigger*>(iNodeFactory::getInstance().getNode(_lodTrigger));
    if (lodTrigger != nullptr)
    {
        iaVector3f pos = lodTrigger->getWorldPosition();
        if (pos.length2() < 1.0f)
        {
            return;
        }

#ifdef FIX_POSITION
        pos.set(9986, 310, 8977);
#endif

#ifdef FIX_HEIGHT
        pos._y = 310;
#endif

        iaVector3I observerPosition(pos._x, pos._y, pos._z);

        iStatistics::getInstance().beginSection(_discoverBlocksSection);
        discoverBlocks(observerPosition);
        iStatistics::getInstance().endSection(_discoverBlocksSection);
 
        iStatistics::getInstance().beginSection(_updateBlocksSection);
        updateBlocks(observerPosition);
        iStatistics::getInstance().endSection(_updateBlocksSection);
    }
}

void VoxelTerrain::updateBlocks(const iaVector3I& observerPosition)
{
    auto& voxelBlocks = _voxelBlocks[_lowestLOD];
    
    for (auto block : voxelBlocks)
    {
        update(block.second, observerPosition);
        updateGeometry(block.second, observerPosition);
        updateVisibility(block.second, observerPosition);
    }

    // todo need to remove voxel blocks from _voxelBlocks if not in use anymore
}

void VoxelTerrain::discoverBlocks(const iaVector3I& observerPosition)
{
    float64 lodFactor = pow(2, _lowestLOD);
    float64 actualBlockSize = VoxelBlock::_voxelBlockSize * lodFactor;

    if (_dirtyDiscovery || observerPosition.distance2(_lastDiscoveryPosition) > actualBlockSize * actualBlockSize)
    {
        _dirtyDiscovery = false;
        _lastDiscoveryPosition = observerPosition;

        iaVector3I center = observerPosition;
        center /= actualBlockSize;

        iaVector3I start(center);
        start._x -= _voxelBlockDiscoveryDistance;
        start._y -= _voxelBlockDiscoveryDistance;
        start._z -= _voxelBlockDiscoveryDistance;

        iaVector3I stop(center);
        stop._x += _voxelBlockDiscoveryDistance;
        stop._y += _voxelBlockDiscoveryDistance;
        stop._z += _voxelBlockDiscoveryDistance;

        if (start._x < 0)
        {
            start._x = 0;
        }

        if (start._y < 0)
        {
            start._y = 0;
        }

        if (start._y > 3) // TODO workaround maybe we need to be able to configure that 
        {
            start._y = 3;
        }

        if (start._z < 0)
        {
            start._z = 0;
        }

        auto& voxelBlocks = _voxelBlocks[_lowestLOD];
        iaVector3I voxelBlockPosition;

        for (int64 voxelBlockX = start._x; voxelBlockX < stop._x; ++voxelBlockX)
        {
            for (int64 voxelBlockY = start._y; voxelBlockY < stop._y; ++voxelBlockY)
            {
                for (int64 voxelBlockZ = start._z; voxelBlockZ < stop._z; ++voxelBlockZ)
                {
                    voxelBlockPosition.set(voxelBlockX, voxelBlockY, voxelBlockZ);

                    auto blockIter = voxelBlocks.find(voxelBlockPosition);
                    if (blockIter == voxelBlocks.end())
                    {
                        voxelBlocks[voxelBlockPosition] = new VoxelBlock(_lowestLOD, voxelBlockPosition*actualBlockSize, iaVector3I());
                    }
                }
            }
        }
    }
}

void VoxelTerrain::detachNeighbours(VoxelBlock* voxelBlock)
{
    if (voxelBlock->_neighbors[0] != nullptr)
    {
        voxelBlock->_neighbors[0]->_neighbors[1] = nullptr;
        voxelBlock->_neighbors[0] = nullptr;
    }

    if (voxelBlock->_neighbors[1] != nullptr)
    {
        voxelBlock->_neighbors[1]->_neighbors[0] = nullptr;
        voxelBlock->_neighbors[1] = nullptr;
    }

    if (voxelBlock->_neighbors[2] != nullptr)
    {
        voxelBlock->_neighbors[2]->_neighbors[3] = nullptr;
        voxelBlock->_neighbors[2] = nullptr;
    }

    if (voxelBlock->_neighbors[3] != nullptr)
    {
        voxelBlock->_neighbors[3]->_neighbors[2] = nullptr;
        voxelBlock->_neighbors[3] = nullptr;
    }

    if (voxelBlock->_neighbors[4] != nullptr)
    {
        voxelBlock->_neighbors[4]->_neighbors[5] = nullptr;
        voxelBlock->_neighbors[4] = nullptr;
    }

    if (voxelBlock->_neighbors[5] != nullptr)
    {
        voxelBlock->_neighbors[5]->_neighbors[4] = nullptr;
        voxelBlock->_neighbors[5] = nullptr;
    }
}

void VoxelTerrain::attachNeighbours(VoxelBlock* voxelBlock)
{
    auto voxelBlocks = _voxelBlocks[voxelBlock->_lod];

    iaVector3I neighbourPos;

    neighbourPos = voxelBlock->_position;
    neighbourPos._x += voxelBlock->_size;
    auto neighbour = voxelBlocks.find(neighbourPos);
    if (neighbour != voxelBlocks.end())
    {
        voxelBlock->_neighbors[0] = (*neighbour).second;
        (*neighbour).second->_neighbors[1] = voxelBlock;
    }

    neighbourPos = voxelBlock->_position;
    neighbourPos._x -= voxelBlock->_size;
    neighbour = voxelBlocks.find(neighbourPos);
    if (neighbour != voxelBlocks.end())
    {
        voxelBlock->_neighbors[1] = (*neighbour).second;
        (*neighbour).second->_neighbors[0] = voxelBlock;
    }

    neighbourPos = voxelBlock->_position;
    neighbourPos._y += voxelBlock->_size;
    neighbour = voxelBlocks.find(neighbourPos);
    if (neighbour != voxelBlocks.end())
    {
        voxelBlock->_neighbors[2] = (*neighbour).second;
        (*neighbour).second->_neighbors[3] = voxelBlock;
    }

    neighbourPos = voxelBlock->_position;
    neighbourPos._y -= voxelBlock->_size;
    neighbour = voxelBlocks.find(neighbourPos);
    if (neighbour != voxelBlocks.end())
    {
        voxelBlock->_neighbors[3] = (*neighbour).second;
        (*neighbour).second->_neighbors[2] = voxelBlock;
    }

    neighbourPos = voxelBlock->_position;
    neighbourPos._z += voxelBlock->_size;
    neighbour = voxelBlocks.find(neighbourPos);
    if (neighbour != voxelBlocks.end())
    {
        voxelBlock->_neighbors[4] = (*neighbour).second;
        (*neighbour).second->_neighbors[5] = voxelBlock;
    }

    neighbourPos = voxelBlock->_position;
    neighbourPos._z -= voxelBlock->_size;
    neighbour = voxelBlocks.find(neighbourPos);
    if (neighbour != voxelBlocks.end())
    {
        voxelBlock->_neighbors[5] = (*neighbour).second;
        (*neighbour).second->_neighbors[4] = voxelBlock;
    }
}

void VoxelTerrain::update(VoxelBlock* voxelBlock, iaVector3I observerPosition)
{
    if (voxelBlock->_state == Stage::Empty)
    {
        return;
    }

    float64 distanceSquare = observerPosition.distance2(voxelBlock->_blockCenterPos);

    bool inVisibleRange = false;
    bool childrenInVisibleRange = false;

    if (voxelBlock->_lod == _lowestLOD &&
        distanceSquare < _visibleDistance[voxelBlock->_lod])
    {
        inVisibleRange = true;
    }

    if (voxelBlock->_children[0] != nullptr)
    {
        if (distanceSquare < _visibleDistance[voxelBlock->_lod - 1])
        {
            childrenInVisibleRange = true;
        }

        for (int i = 0; i < 8; ++i)
        {
            voxelBlock->_children[i]->setInVisibilityRange(childrenInVisibleRange);
        }
    }

    if (voxelBlock->_lod == _lowestLOD)
    {
        voxelBlock->setInVisibilityRange(inVisibleRange);
    }

    switch (voxelBlock->_state)
    {
    case Stage::Initial:
        if (distanceSquare < _visibleDistance[voxelBlock->_lod] * 2.0)
        {
            voxelBlock->_state = Stage::Setup;
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
            voxelBlock->_voxelBlockInfo->_voxelData = voxelBlock->_voxelData; // TODO make a copy here
            voxelBlock->_voxelBlockInfo->_lodOffset = iContouringCubes::calcLODOffset(voxelBlock->_lod);
            voxelBlock->_voxelBlockInfo->_lod = voxelBlock->_lod;

            TaskGenerateVoxels* task = new TaskGenerateVoxels(voxelBlock->_voxelBlockInfo, static_cast<uint32>(distanceSquare * 0.9));
            voxelBlock->_voxelGenerationTaskID = iTaskManager::getInstance().addTask(task);
        }

        voxelBlock->_state = Stage::GeneratingVoxel;
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

                voxelBlock->_state = Stage::Empty;
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

                        for (int i = 0; i < 8; ++i)
                        {
                            _voxelBlocks[voxelBlock->_lod - 1][voxelBlock->_children[i]->_position] = voxelBlock->_children[i];
                            voxelBlock->_children[i]->_parent = voxelBlock;
                        }

                        for (int i = 0; i < 8; ++i)
                        {
                            attachNeighbours(voxelBlock->_children[i]);
                        }
                    }
                }

                voxelBlock->_state = Stage::GeneratingMesh;
            }
        }
    }
    break;

    case Stage::Ready:
    {
        if (distanceSquare >= _visibleDistance[voxelBlock->_lod] * 4.0)
        {
            cleanUpVoxelBlock(voxelBlock);
        }
        else if (voxelBlock->getInVisibilityRange())
        {
            if (voxelBlock->_dirtyNeighbors)
            {
                uint32 neighborsLOD = calcLODTransition(voxelBlock);
                if (voxelBlock->_neighborsLOD != neighborsLOD)
                {
                    voxelBlock->_neighborsLOD = neighborsLOD;
                    voxelBlock->_dirty = true;
                }

                voxelBlock->_dirtyNeighbors = false;
            }

            if (voxelBlock->_dirty)
            {
                iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDCurrent));
                if (modelNode != nullptr &&
                    modelNode->isLoaded())
                {
                    voxelBlock->_transformNodeIDQueued = iNode::INVALID_NODE_ID;
                    voxelBlock->_state = Stage::GeneratingMesh;

                    voxelBlock->_dirty = false;
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

void VoxelTerrain::cleanUpVoxelBlock(VoxelBlock* voxelBlock)
{
    if (voxelBlock->_modelNodeIDQueued == iNode::INVALID_NODE_ID)
    {
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDCurrent));
        if (modelNode != nullptr &&
            modelNode->isLoaded())
        {
            iNodeFactory::getInstance().destroyNode(voxelBlock->_transformNodeIDCurrent);
            voxelBlock->_transformNodeIDCurrent = iNode::INVALID_NODE_ID;
            voxelBlock->_modelNodeIDCurrent = iNode::INVALID_NODE_ID;
            voxelBlock->_transformNodeIDQueued = iNode::INVALID_NODE_ID;
            voxelBlock->_modelNodeIDQueued = iNode::INVALID_NODE_ID;
            voxelBlock->_mutationCounter = 0;
            voxelBlock->_state = Stage::Initial;
            voxelBlock->_inVisibleRange = false;
            voxelBlock->_dirtyNeighbors = true;

            //detachNeighbours(voxelBlock); // no need to detach if not deleted

            /*! can't delete that yet. access ist not thread safe
            */
            /*if (voxelBlock->_voxelData != nullptr)
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
    }

    if (voxelBlock->_children[0] != nullptr)
    {
        for (int i = 0; i < 8; ++i)
        {
            cleanUpVoxelBlock(voxelBlock->_children[i]);
        }
    }
}

void VoxelTerrain::updateGeometry(VoxelBlock* voxelBlock, iaVector3I observerPosition)
{
    if (voxelBlock->_state != Stage::Empty)
    {
        if (voxelBlock->_state == Stage::GeneratingMesh)
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

bool VoxelTerrain::updateVisibility(VoxelBlock* voxelBlock, iaVector3I observerPosition)
{
    if (voxelBlock->_state == Stage::Empty)
    {
        return true;
    }

    bool childrenVisible = false;
    bool meshVisible = voxelBlock->getInVisibilityRange();

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

#define HIGHER_NEIGHBOR_LOD_XPOSITIVE 0x20
#define HIGHER_NEIGHBOR_LOD_XNEGATIVE 0x10
#define HIGHER_NEIGHBOR_LOD_YPOSITIVE 0x08
#define HIGHER_NEIGHBOR_LOD_YNEGATIVE 0x04
#define HIGHER_NEIGHBOR_LOD_ZPOSITIVE 0x02
#define HIGHER_NEIGHBOR_LOD_ZNEGATIVE 0x01

uint32 VoxelTerrain::calcLODTransition(VoxelBlock* voxelBlock)
{
    uint32 result = 0;

    if (voxelBlock->_lod >= _lowestLOD)
    {
        return result;
    }

    if (voxelBlock->_neighbors[0] != nullptr)
    {
        if (!voxelBlock->_neighbors[0]->getInVisibilityRange())
        {
            result |= HIGHER_NEIGHBOR_LOD_XPOSITIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_XPOSITIVE;
    }

    if (voxelBlock->_neighbors[1] != nullptr)
    {
        if (!voxelBlock->_neighbors[1]->getInVisibilityRange())
        {
            result |= HIGHER_NEIGHBOR_LOD_XNEGATIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_XNEGATIVE;
    }

    if (voxelBlock->_neighbors[2] != nullptr)
    {
        if (!voxelBlock->_neighbors[2]->getInVisibilityRange())
        {
            result |= HIGHER_NEIGHBOR_LOD_YPOSITIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_YPOSITIVE;
    }

    if (voxelBlock->_neighbors[3] != nullptr)
    {
        if (!voxelBlock->_neighbors[3]->getInVisibilityRange())
        {
            result |= HIGHER_NEIGHBOR_LOD_YNEGATIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_YNEGATIVE;
    }

    if (voxelBlock->_neighbors[4] != nullptr)
    {
        if (!voxelBlock->_neighbors[4]->getInVisibilityRange())
        {
            result |= HIGHER_NEIGHBOR_LOD_ZPOSITIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_ZPOSITIVE;
    }

    if (voxelBlock->_neighbors[5] != nullptr)
    {
        if (!voxelBlock->_neighbors[5]->getInVisibilityRange())
        {
            result |= HIGHER_NEIGHBOR_LOD_ZNEGATIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_ZNEGATIVE;
    }

    return result;
}

void VoxelTerrain::updateMesh(VoxelBlock* voxelBlock, iaVector3I observerPosition)
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
            tileInformation._neighborsLOD = voxelBlock->_neighborsLOD;
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
            _rootNode->insertNodeAsync(transform);

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

            voxelBlock->_state = Stage::Ready;
        }
    }
}
