#ifndef __TASKVOXELTERRAIN__
#define __TASKVOXELTERRAIN__

#include <iTask.h>
using namespace Igor;

#include "VoxelTerrain.h"

class TaskVoxelTerrain : public iTask
{

public:

    /*! initializes member variables

    \param window window connected to render context
    */
    TaskVoxelTerrain(VoxelTerrain* voxelTerrain);

    /*! does nothing
    */
    virtual ~TaskVoxelTerrain() = default;

protected:

    /*! runs the task
    */
    void run();

private:

    /*! the data to work with
    */
    VoxelTerrain* _voxelTerrain = nullptr;

};

#endif
