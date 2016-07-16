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

struct VoxelBlockInfo
{
    bool _generatedVoxels = false;
	bool _transition = false;
    bool _generatedEnemies = false;
    iaVector3I _position;
	iaVector3f _offset;
    iaVector3i _size;
    iVoxelData* _voxelData = nullptr;
};

class TaskGenerateVoxels : public iTask
{

public:

    /*! initializes member variables

    \param window window connected to render context
    */
    TaskGenerateVoxels(VoxelBlockInfo* voxelBlock, uint32 lod, uint32 priority);

    /*! does nothing
    */
    virtual ~TaskGenerateVoxels() = default;

protected:

    /*! runs the task
    */
    void run();

private:

    static vector<iSpheref> _metaballs;
    static vector<iSpheref> _holes;
    static int32 _seed;
    static mutex _initMutex;

    /*! the data to work with
    */
	VoxelBlockInfo* _voxelBlockInfo = nullptr;

	uint32 _lodFactor = 0;
};

#endif
