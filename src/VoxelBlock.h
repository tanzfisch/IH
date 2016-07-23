#ifndef __VOXEL_BLOCK__
#define __VOXEL_BLOCK__

#include <iaVector3.h>
using namespace IgorAux;

#include <iVoxelData.h>
#include <iTaskManager.h>
#include <iNode.h>
using namespace Igor;

#include "TaskGenerateVoxels.h"

namespace Igor
{
	class iScene;
}

enum class Stage
{
    Initial,
    GeneratingVoxel,
    GeneratingMesh,
    Ready,
    Empty
};

class VoxelBlock
{

	friend class VoxelTerrain;

public:

	VoxelBlock(uint32 lod, iaVector3I position);
	~VoxelBlock();

private:

	const int32 _voxelBlockSize = 32;
	const int32 _voxelBlockOverlap = 4;

    bool _edited = false;

	uint64 _taskID = iTask::INVALID_TASK_ID;
	
	uint32 _transformNodeID = iNode::INVALID_NODE_ID;
	uint32 _modelNodeID = iNode::INVALID_NODE_ID;

	uint32 _mutationCounter = 0;
	
	iaVector3I _position;
	iaVector3f _offset;
	uint32 _lod = 0;
	uint32 _size = 0;

    Stage _stage = Stage::Initial;
	
	iVoxelData* _voxelData = nullptr;

	VoxelBlockInfo* _voxelBlockInfo = nullptr;

	VoxelBlock* _cildren[8];

	

};

#endif