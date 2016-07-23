#include "VoxelBlock.h"

#include <iScene.h>
#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodeModel.h>
#include <iNodeMesh.h>
using namespace Igor;

#include "TaskGenerateVoxels.h"
#include "VoxelTerrainMeshGenerator.h"

VoxelBlock::VoxelBlock(uint32 lod, iaVector3I position)
{
	con_assert(lod >= 0 && lod <= 7, "lod out of range");
	_position = position;
	_lod = lod;
	_size = _voxelBlockSize * pow(2, lod);

	if (_lod > 0)
	{
		float32 value = pow(2, _lod - 1) - 0.5;
		_offset.set(-value, value, -value);
	}

	for (int i = 0; i < 8; ++i)
	{
		_cildren[i] = nullptr;
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

