#include "VoxelBlock.h"

#include <iScene.h>
#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodeModel.h>
#include <iNodeMesh.h>
using namespace Igor;

#include "TaskGenerateVoxels.h"
#include "VoxelTerrainMeshGenerator.h"

VoxelBlock::VoxelBlock(uint32 lod, iaVector3I position, iaVector3I parentAdress)
{
    con_assert(lod >= 0 && lod <= 7, "lod out of range");
    _position = position;
    _lod = lod;
    _size = _voxelBlockSize * pow(2, lod);
    _positionInLOD = _position;
    _positionInLOD /= _size;
    _parentAdress = parentAdress;

    float64 halfSize = static_cast<float64>(_size >> 1);
    _blockCenterPos.set(_position._x + halfSize, _position._y + halfSize, _position._z + halfSize);

    for (int i = 0; i < 8; ++i)
    {
        _children[i] = nullptr;
    }

    for (int i = 0; i < 6; ++i)
    {
        _neighbors[i] = nullptr;
    }
}

VoxelBlock::~VoxelBlock()
{
    if (_voxelData != nullptr)
    {
        delete _voxelData;
    }

    if (_voxelBlockInfo != nullptr)
    {
        delete _voxelBlockInfo;
    }
}

void VoxelBlock::setInVisibilityRange(bool visibility)
{
    if (_inVisibleRange != visibility)
    {
        _inVisibleRange = visibility;

        setNeighborsDirty();
    }
}

void VoxelBlock::setNeighborsDirty()
{
    _dirtyNeighbors = true;

    for (int i = 0; i < 6; ++i)
    {
        if (_neighbors[i] != nullptr)
        {
            _neighbors[i]->_dirtyNeighbors = true;
        }
    }
}

void VoxelBlock::setNeighbor(uint32 neighbourIndex, VoxelBlock* neighbour)
{
    con_assert(neighbourIndex > 0 && neighbourIndex < 6, "index out of range");
    _neighbors[neighbourIndex] = neighbour;
}

bool VoxelBlock::getInVisibilityRange() const
{
    return _inVisibleRange;
}

iVoxelData* VoxelBlock::getVoxelData() const
{
    return _voxelData;
}

VoxelBlock*  VoxelBlock::getParent() const
{
    return _parent;
}

