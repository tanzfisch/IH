#ifndef __TASKGENERATEVOXELS__
#define __TASKGENERATEVOXELS__

#include <iTask.h>
#include <iSphere.h>
using namespace Igor;

#include <iaVector3.h>
using namespace IgorAux;

#include <vector>
using namespace std;

namespace Igor
{
    class iVoxelData;
}

/*! voxel block information

contains information needed to be able to generate the voxel data for specified voxel block
*/
struct VoxelBlockInfo
{
    /*! absolute position of voxel block
    */
    iaVector3I _position;

    /*! level of detail dependent offset to absolute grid
    */
	iaVector3f _lodOffset;

    /*! level of detail of this block
    */
    uint32 _lod;

    /*! size of voxel block in voxel
    */
    iaVector3i _size;

    /*! the destination voxel data structure
    */
    iVoxelData* _voxelData = nullptr;

    /*! true if the generated voxel data contains a air-solid transition
    */
    bool _transition = false;
};

class TaskGenerateVoxels : public iTask
{

public:

    /*! initializes member variables

    \param window window connected to render context
    */
    TaskGenerateVoxels(VoxelBlockInfo* voxelBlock, uint32 priority);

    /*! does nothing
    */
    virtual ~TaskGenerateVoxels() = default;

protected:

    /*! runs the task
    */
    void run();

private:

    /*! the data to work with
    */
	VoxelBlockInfo* _voxelBlockInfo = nullptr;

};

#endif
