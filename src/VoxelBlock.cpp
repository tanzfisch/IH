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
    _parentAdress = parentAdress;

    float64 halfSize = static_cast<float64>(_size >> 1);
    _blockCenterPos.set(static_cast<float64>(_position._x) + halfSize,
        static_cast<float64>(_position._y) + halfSize,
        static_cast<float64>(_position._z) + halfSize);
    
    for (int i = 0; i < 8; ++i)
	{
		_children[i] = nullptr;
	}

	// todo clean up scene and tasks
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

iVoxelData* VoxelBlock::getVoxelData() const
{
    return _voxelData;
}

VoxelBlock*  VoxelBlock::getParent() const
{
    return _parent;
}

