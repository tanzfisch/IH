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
    Setup,
    GeneratingVoxel,
    GeneratingMesh,
    Ready,
    Empty
};

class VoxelBlock
{

	friend class VoxelTerrain;

public:

    static const int32 _voxelBlockSize = 32;
    static const int32 _voxelBlockOverlap = 2;

	VoxelBlock(uint32 lod, iaVector3I position, iaVector3I parentAdress);
	~VoxelBlock();

    iVoxelData* getVoxelData() const;
    VoxelBlock* getParent() const;

private:

    /*! bit mask with current neighbors LOD settings
    */
	uint32 _neighborsLOD = 0;

    /*! if dirty the tile has to be regenerated
    */
    bool _dirty = false;

    /*! list of nodes that are not used anymore
    */
    vector<uint32> _nodesToDestroy;

    /*! id of voxel generation task
    
    there is only one at a time needed
    */
	uint64 _voxelGenerationTaskID = iTask::INVALID_TASK_ID;
	
	uint32 _transformNodeID = iNode::INVALID_NODE_ID;
	uint32 _modelNodeID = iNode::INVALID_NODE_ID;

    /*! everytime the tile changes this counter goes up so Igor can tell the difference between the models
    */
	uint32 _mutationCounter = 0;

    iaVector3I _parentAdress;
	
	iaVector3I _position;
	uint32 _lod = 0;
	uint32 _size = 0;

    Stage _stage = Stage::Initial;
	
	iVoxelData* _voxelData = nullptr;

	VoxelBlockInfo* _voxelBlockInfo = nullptr;

    VoxelBlock* _parent = nullptr;
	VoxelBlock* _children[8];

};

#endif