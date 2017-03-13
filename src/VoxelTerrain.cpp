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
#include <iPerlinNoise.h>
using namespace Igor;

#include <iaConvert.h>
#include <iaConsole.h>
using namespace IgorAux;

#include "TaskGenerateVoxels.h"
#include "TaskVoxelTerrain.h"

//#define DISABLE_SECTIONS
// #define FIX_POSITION
// #define FIX_HEIGHT
// #define WIREFRAME

iaVector3I childOffsetPosition[8] = 
{ 
    iaVector3I(0, 0, 0),
    iaVector3I(1, 0, 0),
    iaVector3I(1, 0, 1),
    iaVector3I(0, 0, 1),
    iaVector3I(0, 1, 0),
    iaVector3I(1, 1, 0),
    iaVector3I(1, 1, 1),
    iaVector3I(0, 1, 1) 
};

VoxelTerrain::VoxelTerrain(GenerateVoxelsDelegate generateVoxelsDelegate)
{
    _generateVoxelsDelegate = generateVoxelsDelegate;

    unordered_map<iaVector3I, VoxelBlock*, VectorHasher, VectorEqualFn> voxelBlocks;

    for (int i = 0; i < _lowestLOD + 1; ++i)
    {
        _voxelBlocks.push_back(voxelBlocks);
    }

    _totalSection = iStatistics::getInstance().registerSection("VT:all", 3);
    _discoverBlocksSection = iStatistics::getInstance().registerSection("VT:discover", 3);
    _updateBlocksSection = iStatistics::getInstance().registerSection("VT:blocks", 3);
    _deleteBlocksSection = iStatistics::getInstance().registerSection("VT:delete", 3);
    _applyActionsSection = iStatistics::getInstance().registerSection("VT:applyActions", 3);
    _updateVisBlocksSection = iStatistics::getInstance().registerSection("VT:vis", 3);

    iRenderer::getInstance().setWorldGridResolution(1000.0);
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

void VoxelTerrain::setNodeActiveAsync(iNode* node, bool active)
{
    iNodeFactory::iAction action;
    action._action = active ? iNodeFactory::iActionType::Activate : iNodeFactory::iActionType::Deactivate;
    action._nodeA = node->getID();

    _actionQueue.push_back(action);
}

void VoxelTerrain::insertNodeAsync(iNode* src, iNode* dst)
{
    iNodeFactory::iAction action;
    action._action = iNodeFactory::iActionType::Insert;
    action._nodeA = src->getID();
    action._nodeB = dst->getID();

    _actionQueue.push_back(action);
}

void VoxelTerrain::removeNodeAsync(iNode* src, iNode* dst)
{
    iNodeFactory::iAction action;
    action._action = iNodeFactory::iActionType::Remove;
    action._nodeA = src->getID();
    action._nodeB = dst->getID();

    _actionQueue.push_back(action);
}

void VoxelTerrain::destroyNodeAsync(uint32 nodeID)
{
    con_assert(nodeID != iNode::INVALID_NODE_ID, "invalid node id");

    iNodeFactory::iAction action;
    action._action = iNodeFactory::iActionType::Destroy;
    action._nodeA = nodeID;

    _actionQueue.push_back(action);
}

void VoxelTerrain::init(iScene* scene)
{
    _rootNode = iNodeFactory::getInstance().createNode(iNodeType::iNode);
    scene->getRoot()->insertNode(_rootNode);

    iModelResourceFactory::getInstance().registerModelDataIO("vtg", &VoxelTerrainMeshGenerator::createInstance);
    iTaskManager::getInstance().addTask(new TaskVoxelTerrain(this));

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

    // TODO cleanup
}

void VoxelTerrain::setLODTrigger(uint32 lodTriggerID)
{
    _lodTrigger = lodTriggerID;
}

void VoxelTerrain::handleVoxelBlocks()
{
#ifndef DISABLE_SECTIONS
    iStatistics::getInstance().beginSection(_totalSection);
#endif

#ifndef DISABLE_SECTIONS
    iStatistics::getInstance().beginSection(_deleteBlocksSection);
#endif
    deleteBlocks();
#ifndef DISABLE_SECTIONS
    iStatistics::getInstance().endSection(_deleteBlocksSection);
#endif

    iNodeLODTrigger* lodTrigger = static_cast<iNodeLODTrigger*>(iNodeFactory::getInstance().getNode(_lodTrigger));
    if (lodTrigger != nullptr)
    {
        iaVector3d pos = lodTrigger->getWorldPosition();
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

#ifndef DISABLE_SECTIONS
        iStatistics::getInstance().beginSection(_discoverBlocksSection);
#endif
        discoverBlocks(observerPosition);
#ifndef DISABLE_SECTIONS
        iStatistics::getInstance().endSection(_discoverBlocksSection);
#endif

        updateBlocks(observerPosition);
    }

#ifndef DISABLE_SECTIONS
    iStatistics::getInstance().beginSection(_applyActionsSection);
#endif
    // apply all actions at once so they will be synced with next frame
    iNodeFactory::getInstance().applyActionsAsync(_actionQueue);
    _actionQueue.clear();
#ifndef DISABLE_SECTIONS
    iStatistics::getInstance().endSection(_applyActionsSection);
#endif

#ifndef DISABLE_SECTIONS
    iStatistics::getInstance().endSection(_totalSection);
#endif
}

void VoxelTerrain::updateBlocks(const iaVector3I& observerPosition)
{
    auto& voxelBlocks = _voxelBlocks[_lowestLOD];

    for (auto block : voxelBlocks)
    {
#ifndef DISABLE_SECTIONS
        iStatistics::getInstance().beginSection(_updateBlocksSection);
#endif
        update(block.second, observerPosition);
#ifndef DISABLE_SECTIONS
        iStatistics::getInstance().endSection(_updateBlocksSection);
#endif

#ifndef DISABLE_SECTIONS
        iStatistics::getInstance().beginSection(_updateVisBlocksSection);
#endif
        updateVisibility(block.second);
#ifndef DISABLE_SECTIONS
        iStatistics::getInstance().endSection(_updateVisBlocksSection);
#endif
    }
}

void VoxelTerrain::deleteBlocks()
{
    auto iter = _voxelBlocksToDelete.begin();
    while (iter != _voxelBlocksToDelete.end())
    {
        if (canBeDeleted((*iter)))
        {
            deleteBlock((*iter));
            iter = _voxelBlocksToDelete.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

bool VoxelTerrain::canBeDeleted(VoxelBlock* voxelBlock)
{
    for (int i = 0; i < 8; ++i)
    {
        if (voxelBlock->_children[i] != VoxelBlock::INVALID_VOXELBLOCKID)
        {
            return false;
        }
    }

    if (voxelBlock->_state == Stage::GeneratingMesh)
    {
        finalizeMesh(voxelBlock);
    }

    if (voxelBlock->_transformNodeIDQueued != iNode::INVALID_NODE_ID)
    {
        return false;
    }

    if (voxelBlock->_voxelGenerationTaskID != iTask::INVALID_TASK_ID)
    {
        return false;
    }

    return true;
}

void VoxelTerrain::deleteBlock(VoxelBlock* voxelBlock)
{
    auto parent = _voxelBlocksMap.find(voxelBlock->_parent);
    if (parent != _voxelBlocksMap.end())
    {
        for (int i = 0; i < 8; ++i)
        {
            if ((*parent).second->_children[i] == voxelBlock->_id)
            {
                (*parent).second->_children[i] = VoxelBlock::INVALID_VOXELBLOCKID;
            }
        }
    }

    if (voxelBlock->_transformNodeIDCurrent != iNode::INVALID_NODE_ID)
    {
        destroyNodeAsync(voxelBlock->_transformNodeIDCurrent);
    }

    auto block = _voxelBlocksMap.find(voxelBlock->_id);
    if (block != _voxelBlocksMap.end())
    {
        _voxelBlocksMap.erase(block);
    }

    /*if (voxelBlock->_voxelData != nullptr)
    {
        delete voxelBlock->_voxelData;
    }
        
    if (voxelBlock->_voxelBlockInfo != nullptr)
    {
        delete voxelBlock->_voxelBlockInfo;
    }

    delete voxelBlock;*/
}

void VoxelTerrain::setNeighboursDirty(VoxelBlock* voxelBlock)
{
    voxelBlock->_dirtyNeighbours = true;

    for (int i = 0; i < 6; ++i)
    {
        auto neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[i]);
        if (neighbour != _voxelBlocksMap.end())
        {
            (*neighbour).second->_dirtyNeighbours = true;
        }
        else
        {
            voxelBlock->_neighbours[i] = VoxelBlock::INVALID_VOXELBLOCKID;
        }
    }
}

VoxelBlock* VoxelTerrain::createVoxelBlock(uint32 lod, iaVector3I positionInLOD, uint8 childAdress)
{
    con_assert(lod >= 0 && lod <= _lowestLOD, "lod out of range");

    VoxelBlock* result = new VoxelBlock();

    result->_positionInLOD = positionInLOD;
    result->_positionInLOD += childOffsetPosition[childAdress];

    result->_lod = lod;
    result->_size = _voxelBlockSize * pow(2, lod);
    result->_childAdress = childAdress;

    for (int i = 0; i < 8; ++i)
    {
        result->_children[i] = VoxelBlock::INVALID_VOXELBLOCKID;
    }

    for (int i = 0; i < 6; ++i)
    {
        result->_neighbours[i] = VoxelBlock::INVALID_VOXELBLOCKID;
    }

    _voxelBlocks[lod][result->_positionInLOD] = result;
    _voxelBlocksMap[_nextVoxelBlockID] = result;
    result->_id = _nextVoxelBlockID++;

    return result;
}

void VoxelTerrain::discoverBlocks(const iaVector3I& observerPosition)
{
    float64 lodFactor = pow(2, _lowestLOD);
    float64 actualBlockSize = _voxelBlockSize * lodFactor;

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

        auto voxelBlocksToDelete = _voxelBlocks[_lowestLOD];
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
                        createVoxelBlock(_lowestLOD, voxelBlockPosition, 0);
                    }
                    else
                    {
                        auto blockIter = voxelBlocksToDelete.find(voxelBlockPosition);
                        if (blockIter != voxelBlocksToDelete.end())
                        {
                            voxelBlocksToDelete.erase(blockIter);
                        }
                    }
                }
            }
        }

        for (auto iter : voxelBlocksToDelete)
        {
            collectBlocksToDelete(iter.second, _voxelBlocksToDelete);
        }
    }
}

void VoxelTerrain::collectBlocksToDelete(VoxelBlock* currentBlock, vector<VoxelBlock*>& dst)
{
    detachNeighbours(currentBlock);

    auto foundBlock = _voxelBlocks[currentBlock->_lod].find(currentBlock->_positionInLOD);
    if (foundBlock != _voxelBlocks[currentBlock->_lod].end())
    {
        _voxelBlocks[currentBlock->_lod].erase(foundBlock);
        dst.push_back(currentBlock);

        for (int i = 0; i < 8; ++i)
        {
            auto child = _voxelBlocksMap.find(currentBlock->_children[i]);

            if (child != _voxelBlocksMap.end())
            {
                collectBlocksToDelete((*child).second, dst);
            }
        }
    }
}

void VoxelTerrain::setInRange(VoxelBlock* voxelBlock, bool inRange)
{
    if (voxelBlock->_inRange != inRange)
    {
        voxelBlock->_inRange = inRange;
        setNeighboursDirty(voxelBlock);
    }
}

void VoxelTerrain::detachNeighbours(VoxelBlock* voxelBlock)
{
    auto neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[0]);
    if (neighbour != _voxelBlocksMap.end())
    {
        (*neighbour).second->_neighbours[1] = VoxelBlock::INVALID_VOXELBLOCKID;
    }
    voxelBlock->_neighbours[0] = VoxelBlock::INVALID_VOXELBLOCKID;

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[1]);
    if (neighbour != _voxelBlocksMap.end())
    {
        (*neighbour).second->_neighbours[0] = VoxelBlock::INVALID_VOXELBLOCKID;
    }
    voxelBlock->_neighbours[1] = VoxelBlock::INVALID_VOXELBLOCKID;

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[2]);
    if (neighbour != _voxelBlocksMap.end())
    {
        (*neighbour).second->_neighbours[3] = VoxelBlock::INVALID_VOXELBLOCKID;
    }
    voxelBlock->_neighbours[2] = VoxelBlock::INVALID_VOXELBLOCKID;

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[3]);
    if (neighbour != _voxelBlocksMap.end())
    {
        (*neighbour).second->_neighbours[2] = VoxelBlock::INVALID_VOXELBLOCKID;
    }
    voxelBlock->_neighbours[3] = VoxelBlock::INVALID_VOXELBLOCKID;

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[4]);
    if (neighbour != _voxelBlocksMap.end())
    {
        (*neighbour).second->_neighbours[5] = VoxelBlock::INVALID_VOXELBLOCKID;
    }
    voxelBlock->_neighbours[4] = VoxelBlock::INVALID_VOXELBLOCKID;

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[5]);
    if (neighbour != _voxelBlocksMap.end())
    {
        (*neighbour).second->_neighbours[4] = VoxelBlock::INVALID_VOXELBLOCKID;
    }
    voxelBlock->_neighbours[5] = VoxelBlock::INVALID_VOXELBLOCKID;
}

void VoxelTerrain::attachNeighbours(VoxelBlock* voxelBlock)
{
    auto voxelBlocks = _voxelBlocks[voxelBlock->_lod];
    iaVector3I neighbourPos;

    if (voxelBlock->_neighbours[0] == VoxelBlock::INVALID_VOXELBLOCKID)
    {
        neighbourPos = voxelBlock->_positionInLOD;
        neighbourPos._x += 1;
        auto neighbour = voxelBlocks.find(neighbourPos);
        if (neighbour != voxelBlocks.end())
        {
            voxelBlock->_neighbours[0] = (*neighbour).second->_id;
            (*neighbour).second->_neighbours[1] = voxelBlock->_id;
        }
    }

    if (voxelBlock->_neighbours[1] == VoxelBlock::INVALID_VOXELBLOCKID)
    {
        neighbourPos = voxelBlock->_positionInLOD;
        neighbourPos._x -= 1;
        auto neighbour = voxelBlocks.find(neighbourPos);
        if (neighbour != voxelBlocks.end())
        {
            voxelBlock->_neighbours[1] = (*neighbour).second->_id;
            (*neighbour).second->_neighbours[0] = voxelBlock->_id;
        }
    }

    if (voxelBlock->_neighbours[2] == VoxelBlock::INVALID_VOXELBLOCKID)
    {
        neighbourPos = voxelBlock->_positionInLOD;
        neighbourPos._y += 1;
        auto neighbour = voxelBlocks.find(neighbourPos);
        if (neighbour != voxelBlocks.end())
        {
            voxelBlock->_neighbours[2] = (*neighbour).second->_id;
            (*neighbour).second->_neighbours[3] = voxelBlock->_id;
        }
    }

    if (voxelBlock->_neighbours[3] == VoxelBlock::INVALID_VOXELBLOCKID)
    {
        neighbourPos = voxelBlock->_positionInLOD;
        neighbourPos._y -= 1;
        auto neighbour = voxelBlocks.find(neighbourPos);
        if (neighbour != voxelBlocks.end())
        {
            voxelBlock->_neighbours[3] = (*neighbour).second->_id;
            (*neighbour).second->_neighbours[2] = voxelBlock->_id;
        }
    }

    if (voxelBlock->_neighbours[4] == VoxelBlock::INVALID_VOXELBLOCKID)
    {
        neighbourPos = voxelBlock->_positionInLOD;
        neighbourPos._z += 1;
        auto neighbour = voxelBlocks.find(neighbourPos);
        if (neighbour != voxelBlocks.end())
        {
            voxelBlock->_neighbours[4] = (*neighbour).second->_id;
            (*neighbour).second->_neighbours[5] = voxelBlock->_id;
        }
    }

    if (voxelBlock->_neighbours[5] == VoxelBlock::INVALID_VOXELBLOCKID)
    {
        neighbourPos = voxelBlock->_positionInLOD;
        neighbourPos._z -= 1;
        auto neighbour = voxelBlocks.find(neighbourPos);
        if (neighbour != voxelBlocks.end())
        {
            voxelBlock->_neighbours[5] = (*neighbour).second->_id;
            (*neighbour).second->_neighbours[4] = voxelBlock->_id;
        }
    }
}

void VoxelTerrain::update(VoxelBlock* voxelBlock, iaVector3I observerPosition)
{
    if (voxelBlock->_state == Stage::Empty)
    {
        return;
    }

    // distance in tiles of current lod in all three dimensions
    iaVector3I distance = observerPosition;
    distance /= voxelBlock->_size;
    distance -= voxelBlock->_positionInLOD;
    distance._x = abs(distance._x);
    distance._y = abs(distance._y);
    distance._z = abs(distance._z);

    bool inRange = false;

    if (voxelBlock->_lod == _lowestLOD)
    {
        if (distance._x <= _voxelBlockDiscoveryDistance && distance._y <= _voxelBlockDiscoveryDistance && distance._z <= _voxelBlockDiscoveryDistance)
        {
            inRange = true;
        }
    }

    if (voxelBlock->_children[0] != VoxelBlock::INVALID_VOXELBLOCKID)
    {
        bool childrenInRange = false;

        if (distance._x <= 1 && distance._y <= 1 && distance._z <= 1)
        {
            childrenInRange = true;
        }

        for (int i = 0; i < 8; ++i)
        {
            VoxelBlock* child = _voxelBlocksMap[voxelBlock->_children[i]];
            setInRange(child, childrenInRange);
        }
    }

    if (voxelBlock->_lod == _lowestLOD)
    {
        setInRange(voxelBlock, inRange);
    }

    switch (voxelBlock->_state)
    {
    case Stage::Initial:
        if (distance._x <= 4 && distance._y <= 4 && distance._z <= 4)
        {
            voxelBlock->_state = Stage::Setup;
        }
        break;

    case Stage::Setup:
    {
        if (voxelBlock->_voxelBlockInfo == nullptr)
        {
            voxelBlock->_voxelData = new iVoxelData();
            voxelBlock->_voxelData->setClearValue(0);

            voxelBlock->_voxelBlockInfo = new VoxelBlockInfo();
            voxelBlock->_voxelBlockInfo->_size = _voxelBlockSize + _voxelBlockOverlap;
            voxelBlock->_voxelBlockInfo->_positionInLOD = voxelBlock->_positionInLOD;
            voxelBlock->_voxelBlockInfo->_voxelData = voxelBlock->_voxelData;
            voxelBlock->_voxelBlockInfo->_lodOffset = iContouringCubes::calcLODOffset(voxelBlock->_lod);
            voxelBlock->_voxelBlockInfo->_lod = voxelBlock->_lod;

            // lower lods need higher priority to build
            uint32  priority = _lowestLOD - voxelBlock->_lod + 1;

            TaskGenerateVoxels* task = new TaskGenerateVoxels(voxelBlock->_voxelBlockInfo, priority, _generateVoxelsDelegate);
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
                    if (voxelBlock->_children[0] == 0)
                    {
                        float64 halfSize = static_cast<float64>(voxelBlock->_size >> 1);

                        uint32 childLOD = voxelBlock->_lod - 1;
                        iaVector3I positionInLOD = voxelBlock->_positionInLOD;
                        positionInLOD *= 2;
                        VoxelBlock* child = createVoxelBlock(childLOD, positionInLOD, 0);
                        voxelBlock->_children[0] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 1);
                        voxelBlock->_children[1] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 2);
                        voxelBlock->_children[2] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 3);
                        voxelBlock->_children[3] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 4);
                        voxelBlock->_children[4] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 5);
                        voxelBlock->_children[5] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 6);
                        voxelBlock->_children[6] = child->_id;
                        child->_parent = voxelBlock->_id;

                        child = createVoxelBlock(childLOD, positionInLOD, 7);
                        voxelBlock->_children[7] = child->_id;
                        child->_parent = voxelBlock->_id;

                        //   4-----5
                        //  /|    /|
                        // 7-----6 |
                        // | 0---|-1
                        // |/    |/
                        // 3-----2

                        _voxelBlocksMap[voxelBlock->_children[0]]->_neighbours[0] = voxelBlock->_children[1];
                        _voxelBlocksMap[voxelBlock->_children[1]]->_neighbours[1] = voxelBlock->_children[0];

                        _voxelBlocksMap[voxelBlock->_children[3]]->_neighbours[0] = voxelBlock->_children[2];
                        _voxelBlocksMap[voxelBlock->_children[2]]->_neighbours[1] = voxelBlock->_children[3];

                        _voxelBlocksMap[voxelBlock->_children[4]]->_neighbours[0] = voxelBlock->_children[4];
                        _voxelBlocksMap[voxelBlock->_children[5]]->_neighbours[1] = voxelBlock->_children[5];

                        _voxelBlocksMap[voxelBlock->_children[7]]->_neighbours[0] = voxelBlock->_children[6];
                        _voxelBlocksMap[voxelBlock->_children[6]]->_neighbours[1] = voxelBlock->_children[7];

                        _voxelBlocksMap[voxelBlock->_children[0]]->_neighbours[2] = voxelBlock->_children[4];
                        _voxelBlocksMap[voxelBlock->_children[4]]->_neighbours[3] = voxelBlock->_children[0];

                        _voxelBlocksMap[voxelBlock->_children[1]]->_neighbours[2] = voxelBlock->_children[5];
                        _voxelBlocksMap[voxelBlock->_children[5]]->_neighbours[3] = voxelBlock->_children[1];

                        _voxelBlocksMap[voxelBlock->_children[2]]->_neighbours[2] = voxelBlock->_children[6];
                        _voxelBlocksMap[voxelBlock->_children[6]]->_neighbours[3] = voxelBlock->_children[2];

                        _voxelBlocksMap[voxelBlock->_children[3]]->_neighbours[2] = voxelBlock->_children[7];
                        _voxelBlocksMap[voxelBlock->_children[7]]->_neighbours[3] = voxelBlock->_children[3];

                        _voxelBlocksMap[voxelBlock->_children[0]]->_neighbours[4] = voxelBlock->_children[3];
                        _voxelBlocksMap[voxelBlock->_children[3]]->_neighbours[5] = voxelBlock->_children[0];

                        _voxelBlocksMap[voxelBlock->_children[1]]->_neighbours[4] = voxelBlock->_children[2];
                        _voxelBlocksMap[voxelBlock->_children[2]]->_neighbours[5] = voxelBlock->_children[1];

                        _voxelBlocksMap[voxelBlock->_children[4]]->_neighbours[4] = voxelBlock->_children[7];
                        _voxelBlocksMap[voxelBlock->_children[7]]->_neighbours[5] = voxelBlock->_children[4];

                        _voxelBlocksMap[voxelBlock->_children[5]]->_neighbours[4] = voxelBlock->_children[6];
                        _voxelBlocksMap[voxelBlock->_children[6]]->_neighbours[5] = voxelBlock->_children[5];

                        for (int i = 0; i < 8; ++i)
                        {
                            attachNeighbours(_voxelBlocksMap[voxelBlock->_children[i]]);
                        }
                    }
                }
                voxelBlock->_state = Stage::GeneratingMesh;
            }
        }
    }
    break;

    case Stage::GeneratingMesh:
        updateMesh(voxelBlock);
        break;

    case Stage::Ready:
    {
        if (voxelBlock->_inRange)
        {
            if (voxelBlock->_dirtyNeighbours)
            {
                uint8 neighborsLOD = calcLODTransition(voxelBlock);
                if (voxelBlock->_neighboursLOD != neighborsLOD)
                {
                    voxelBlock->_neighboursLOD = neighborsLOD;
                    voxelBlock->_dirty = true;
                }

                voxelBlock->_dirtyNeighbours = false;
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

    if (voxelBlock->_children[0] != VoxelBlock::INVALID_VOXELBLOCKID)
    {
        for (int i = 0; i < 8; ++i)
        {
            VoxelBlock* child = _voxelBlocksMap[voxelBlock->_children[i]];
            update(child, observerPosition);
        }
    }
}

bool VoxelTerrain::updateVisibility(VoxelBlock* voxelBlock)
{
    if (voxelBlock->_state == Stage::Empty)
    {
        return true;
    }

    bool childrenVisible = false;
    bool blockVisible = voxelBlock->_inRange;
    bool meshVisible = blockVisible;

    if (voxelBlock->_children[0] != VoxelBlock::INVALID_VOXELBLOCKID)
    {
        childrenVisible = true;
        for (int i = 0; i < 8; ++i)
        {
            if (!updateVisibility(_voxelBlocksMap[voxelBlock->_children[i]]))
            {
                childrenVisible = false;
            }
        }

        if (!childrenVisible)
        {
            for (int i = 0; i < 8; ++i)
            {
                VoxelBlock* child = _voxelBlocksMap[voxelBlock->_children[i]];
                if (child->_modelNodeIDCurrent != iNode::INVALID_NODE_ID)
                {
                    iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(child->_modelNodeIDCurrent));
                    if (modelNode != nullptr &&
                        modelNode->isLoaded())
                    {
                        iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(child->_transformNodeIDCurrent));
                        if (transformNode != nullptr)
                        {
                            setNodeActiveAsync(transformNode, false);
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

    if (voxelBlock->_modelNodeIDCurrent != iNode::INVALID_NODE_ID)
    {
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDCurrent));
        if (modelNode != nullptr &&
            modelNode->isLoaded())
        {
            iNode* group = static_cast<iNodeMesh*>(modelNode->getChild("group"));
            if (group != nullptr)
            {
                iNodeMesh* meshNode = static_cast<iNodeMesh*>(group->getChild("mesh"));
                if (meshNode != nullptr)
                {
                    meshNode->setVisible(meshVisible);
                }
            }

            iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(voxelBlock->_transformNodeIDCurrent));
            if (transformNode != nullptr)
            {
                setNodeActiveAsync(transformNode, meshVisible);
            }
        }
        else
        {
            meshVisible = false;
        }
    }

    return (meshVisible || childrenVisible);
}

#define HIGHER_NEIGHBOR_LOD_XPOSITIVE 0x20
#define HIGHER_NEIGHBOR_LOD_XNEGATIVE 0x10
#define HIGHER_NEIGHBOR_LOD_YPOSITIVE 0x08
#define HIGHER_NEIGHBOR_LOD_YNEGATIVE 0x04
#define HIGHER_NEIGHBOR_LOD_ZPOSITIVE 0x02
#define HIGHER_NEIGHBOR_LOD_ZNEGATIVE 0x01

uint8 VoxelTerrain::calcLODTransition(VoxelBlock* voxelBlock)
{
    uint8 result = 0;

    if (voxelBlock->_lod >= _lowestLOD)
    {
        return result;
    }

    auto neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[0]);
    if (neighbour != _voxelBlocksMap.end())
    {
        if (!(*neighbour).second->_inRange)
        {
            result |= HIGHER_NEIGHBOR_LOD_XPOSITIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_XPOSITIVE;
    }

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[1]);
    if (neighbour != _voxelBlocksMap.end())
    {
        if (!(*neighbour).second->_inRange)
        {
            result |= HIGHER_NEIGHBOR_LOD_XNEGATIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_XNEGATIVE;
    }

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[2]);
    if (neighbour != _voxelBlocksMap.end())
    {
        if (!(*neighbour).second->_inRange)
        {
            result |= HIGHER_NEIGHBOR_LOD_YPOSITIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_YPOSITIVE;
    }

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[3]);
    if (neighbour != _voxelBlocksMap.end())
    {
        if (!(*neighbour).second->_inRange)
        {
            result |= HIGHER_NEIGHBOR_LOD_YNEGATIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_YNEGATIVE;
    }

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[4]);
    if (neighbour != _voxelBlocksMap.end())
    {
        if (!(*neighbour).second->_inRange)
        {
            result |= HIGHER_NEIGHBOR_LOD_ZPOSITIVE;
        }
    }
    else
    {
        result |= HIGHER_NEIGHBOR_LOD_ZPOSITIVE;
    }

    neighbour = _voxelBlocksMap.find(voxelBlock->_neighbours[5]);
    if (neighbour != _voxelBlocksMap.end())
    {
        if (!(*neighbour).second->_inRange)
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

void VoxelTerrain::updateMesh(VoxelBlock* voxelBlock)
{
    if (voxelBlock->_transformNodeIDQueued == iNode::INVALID_NODE_ID)
    {
        if (voxelBlock->_voxelData != nullptr)
        {
            auto parent = _voxelBlocksMap.find(voxelBlock->_parent);
            if (parent != _voxelBlocksMap.end())
            {
                TileInformation tileInformation;

                tileInformation._materialID = _terrainMaterialID;
                tileInformation._voxelOffsetToNextLOD = childOffsetPosition[voxelBlock->_childAdress];
                tileInformation._voxelOffsetToNextLOD *= 16;
                tileInformation._voxelData = new iVoxelData();
                voxelBlock->_voxelData->getCopy(*(tileInformation._voxelData));
                tileInformation._voxelDataNextLOD = new iVoxelData();
                (*parent).second->_voxelData->getCopy(*(tileInformation._voxelDataNextLOD));
                tileInformation._lod = voxelBlock->_lod;
                tileInformation._neighboursLOD = voxelBlock->_neighboursLOD;
                voxelBlock->_neighboursLOD = tileInformation._neighboursLOD;

                iModelDataInputParameter* inputParam = new iModelDataInputParameter(); // will be deleted by iModel
                inputParam->_identifier = "vtg";
                inputParam->_joinVertexes = true;
                inputParam->_needsRenderContext = false;
                inputParam->_modelSourceType = iModelSourceType::Generated;
                inputParam->_loadPriority = 0;
                inputParam->_parameters.setData(reinterpret_cast<const char*>(&tileInformation), sizeof(TileInformation));

                iaString tileName = iaString::itoa(voxelBlock->_positionInLOD._x);
                tileName += ":";
                tileName += iaString::itoa(voxelBlock->_positionInLOD._y);
                tileName += ":";
                tileName += iaString::itoa(voxelBlock->_positionInLOD._z);
                tileName += ":";
                tileName += iaString::itoa(voxelBlock->_lod);
                tileName += ":";
                tileName += iaString::itoa(voxelBlock->_mutationCounter++);

                iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));

                iaVector3d transform;
                iaConvert::convert(voxelBlock->_positionInLOD, transform);
                transform *= voxelBlock->_size;
                transformNode->translate(transform);

                iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
                modelNode->setModel(tileName, iResourceCacheMode::Free, inputParam);

                transformNode->insertNode(modelNode);
                insertNodeAsync(_rootNode, transformNode);

                voxelBlock->_transformNodeIDQueued = transformNode->getID();
                voxelBlock->_modelNodeIDQueued = modelNode->getID();

                voxelBlock->_dirty = false;
            }
        }
    }
    else
    {
        finalizeMesh(voxelBlock);
    }
}

void VoxelTerrain::finalizeMesh(VoxelBlock* voxelBlock)
{
    static int count = 0;
    iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(voxelBlock->_modelNodeIDQueued));
    if (modelNode != nullptr &&
        modelNode->isLoaded())
    {
        if (voxelBlock->_transformNodeIDCurrent != iNode::INVALID_NODE_ID)
        {
            destroyNodeAsync(voxelBlock->_transformNodeIDCurrent);
        }

        voxelBlock->_transformNodeIDCurrent = voxelBlock->_transformNodeIDQueued;
        voxelBlock->_modelNodeIDCurrent = voxelBlock->_modelNodeIDQueued;
        voxelBlock->_modelNodeIDQueued = iNode::INVALID_NODE_ID;

        voxelBlock->_state = Stage::Ready;
    }
}