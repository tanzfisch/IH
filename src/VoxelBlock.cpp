#include "VoxelBlock.h"

#include <iScene.h>
#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodeModel.h>
#include <iNodeMesh.h>
using namespace Igor;

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
        _neighbours[i] = nullptr;
    }
}

VoxelBlock::~VoxelBlock()
{
    if (_children[0] != nullptr)
    {
        for (int i = 0; i < 8; ++i)
        {
            delete _children[i];
            _children[i] = nullptr;
        }
    }

    if (_voxelData != nullptr)
    {
        delete _voxelData;
    }

    if (_voxelBlockInfo != nullptr)
    {
        delete _voxelBlockInfo;
    }
}

void VoxelBlock::setInRange(bool visibility)
{
    if (_inRange != visibility)
    {
        _inRange = visibility;

        setNeighboursDirty();
    }
}

void VoxelBlock::setNeighboursDirty()
{
    _dirtyNeighbours = true;

    for (int i = 0; i < 6; ++i)
    {
        if (_neighbours[i] != nullptr)
        {
            _neighbours[i]->_dirtyNeighbours = true;
        }
    }
}

void VoxelBlock::setNeighbour(uint32 neighbourIndex, VoxelBlock* neighbour)
{
    con_assert(neighbourIndex > 0 && neighbourIndex < 6, "index out of range");
    _neighbours[neighbourIndex] = neighbour;
}

bool VoxelBlock::getInRange() const
{
    return _inRange;
}

iVoxelData* VoxelBlock::getVoxelData() const
{
    return _voxelData;
}

VoxelBlock*  VoxelBlock::getParent() const
{
    return _parent;
}

