#ifndef __TASKGENERATEVOXELS__
#define __TASKGENERATEVOXELS__

#include <iTask.h>
#include <iSphere.h>
using namespace Igor;

#include <iaVector3.h>
#include <iaDelegate.h>
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
    iaVector3I _positionInLOD;

    /*! level of detail dependent offset to absolute grid
    */
	iaVector3f _lodOffset;

    /*! level of detail of this block
    */
    uint32 _lod;

    /*! size of voxel block in voxel
    */
    uint64 _size;

    /*! the destination voxel data structure
    */
    iVoxelData* _voxelData = nullptr;

    /*! true if the generated voxel data contains an air to solid transition
    */
    bool _transition = false;
};

/*! callback to generate voxel data

\param voxelBlockInfo contains all information to generate the voxels
*/
iaDELEGATE(GenerateVoxelsDelegate, void, (VoxelBlockInfo* voxelBlockInfo), (voxelBlockInfo));

/*! task to generate voxels.

the actual voxel generation happens in the callback function to be implemented by application
*/
class TaskGenerateVoxels : public iTask
{

public:

    /*! initializes member variables

    \param voxelBlockInfo the voxel block to generate the data for
    \param priority the priority to run this task with
    \param generateVoxelsDelegate the delegate to do the actual work
    */
    TaskGenerateVoxels(VoxelBlockInfo* voxelBlockInfo, uint32 priority, GenerateVoxelsDelegate generateVoxelsDelegate);

    /*! does nothing
    */
    virtual ~TaskGenerateVoxels() = default;

protected:

    /*! runs the task
    */
    void run();

private:

    /*! delegate that does the actual work
    */
    GenerateVoxelsDelegate _generateVoxelsDelegate;

    /*! the data to work with
    */
	VoxelBlockInfo* _voxelBlockInfo = nullptr;

};

#endif
