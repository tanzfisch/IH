#include "TaskGenerateVoxels.h"

#include <iaConsole.h>
using namespace IgorAux;

TaskGenerateVoxels::TaskGenerateVoxels(VoxelBlockInfo* voxelBlockInfo, uint32 priority, GenerateVoxelsDelegate generateVoxelsDelegate)
    : iTask(nullptr, priority, false)
{
    con_assert(voxelBlockInfo != nullptr, "zero pointer");
    con_assert(voxelBlockInfo->_voxelData != nullptr, "zero pointer");

    _generateVoxelsDelegate = generateVoxelsDelegate;
    _voxelBlockInfo = voxelBlockInfo;
}

void TaskGenerateVoxels::run()
{
    _generateVoxelsDelegate(_voxelBlockInfo);    
}

