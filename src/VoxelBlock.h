#ifndef __VOXEL_BLOCK__
#define __VOXEL_BLOCK__

#include <iaVector3.h>
using namespace IgorAux;

#include <iVoxelData.h>
#include <iTaskManager.h>
#include <iNode.h>
using namespace Igor;

namespace Igor
{
	class iScene;
}

enum class Stage : uint8
{
    Initial,
    Setup,
    GeneratingVoxel,
    GeneratingMesh,
    Ready,
    Empty
};

/*! voxel block (or tile) or specific LOD
*/
struct VoxelBlock
{

public:

    /*! invalid voxel block id definition
    */
    static const uint64 INVALID_VOXELBLOCKID = 0;

    /*! block ID
    */
    uint64 _id = INVALID_VOXELBLOCKID;

    /*! bit mask with current neighbors LOD settings
    */
    uint8 _neighboursLOD = 0;

    /*! if dirty the tile has to be regenerated
    */
    bool _dirty = true;

    /*! if true mesh is in visible range (and SHOULD be visible)

    but can be actually not rendered because e.g. the mesh is not ready yet
    */
    bool _inRange = false;

    /*! if true neighbours changed and we might have to regenerate the mesh
    */
    bool _dirtyNeighbours = true;

    /*! id of voxel generation task

    there is only one at a time needed
    */
    uint64 _voxelGenerationTaskID = iTask::INVALID_TASK_ID;

    /*! id to transform node to control if a tile is in the scene and therefore visible
    */
    uint32 _transformNodeIDCurrent = iNode::INVALID_NODE_ID;

    /*! id to generated model currently in use
    */
    uint32 _modelNodeIDCurrent = iNode::INVALID_NODE_ID;

    /*! tempoary transform node id to control where we have to regenerate a new tile or not
    */
    uint32 _transformNodeIDQueued = iNode::INVALID_NODE_ID;

    /*! temporary id of node so we can tell if it was already loaded
    */
    uint32 _modelNodeIDQueued = iNode::INVALID_NODE_ID;

    /*! everytime the tile changes this counter goes up so Igor can tell the difference between the models before and after
    */
    uint16 _mutationCounter = 0;

    /*! index position of block relative to parent
    */
    uint8 _childAdress;

    /*! blocks position as index in corresponding LOD
    */
    iaVector3I _positionInLOD;

    /*! level of detail of this block
    */
    uint32 _lod = 0;

    /*! world size of block
    */
    uint16 _size = 0;

    /*! current state of the block
    */
    Stage _state = Stage::Initial;

    /*! the actual voxel data
    */
    iVoxelData* _voxelData = nullptr;

    /*! voxel block info needed to generated voxel data
    */
    VoxelBlockInfo* _voxelBlockInfo = nullptr;

    /*! pointer to parenting block
    */
    uint64 _parent = INVALID_VOXELBLOCKID;

    /*! indexes to children
    */
    uint64 _children[8];

    /*! indexes to neighbour in same LOD
    */
    uint64 _neighbours[6];

};

#endif