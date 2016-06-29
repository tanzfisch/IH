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

class VoxelBlock
{

public:

	VoxelBlock(uint32 lod, iaVector3I position, uint32 terrainMaterialID, iScene* scene);
	~VoxelBlock();

	void update(iaVector3I observerPosition, bool closeEnough);

private:

	const int32 _voxelBlockSize = 32;
	const int32 _voxelBlockOverlap = 5;
	const int32 _tileOverlap = 4;

	uint64 _taskID = iTask::INVALID_TASK_ID;

	/*! terrain material id
	*/
	uint32 _terrainMaterialID = 0;

	uint32 _transformNodeID = iNode::INVALID_NODE_ID;
	uint32 _modelNodeID = iNode::INVALID_NODE_ID;

	uint32 _mutationCounter = 0;

	iScene* _scene = nullptr;

	bool _generated = false;

	bool _dirtyMesh = true;

	iaVector3I _position;
	uint32 _lod = 0;
	uint32 _size = 0;
	
	iVoxelData* _voxelData = nullptr;

	VoxelBlockInfo* _voxelBlockInfo = nullptr;

	VoxelBlock* _cildren[8];

	void updateMesh();

};

#endif