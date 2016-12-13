#include "TaskVoxelTerrain.h"

#include <iaConsole.h>
using namespace IgorAux;

TaskVoxelTerrain::TaskVoxelTerrain(VoxelTerrain* voxelTerrain)
    : iTask(nullptr, 0, true)
{
    con_assert(voxelTerrain != nullptr, "zero pointer");

    if (voxelTerrain != nullptr)
    {
        _voxelTerrain = voxelTerrain;
    }
    else
    {
        con_err("zero pointer");
    }
}

void TaskVoxelTerrain::run()
{
    if (_voxelTerrain != nullptr)
    {
       // TODO _voxelTerrain->handleVoxelBlocks();
    }
    finishTask();
}

