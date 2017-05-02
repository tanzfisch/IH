#ifndef __VOXEL_OPERATION__
#define __VOXEL_OPERATION__

#include <iAABox.h>
using namespace Igor;

class VoxelBlock;

class VoxelOperation
{

public:

    virtual void apply(VoxelBlock* voxelBlock) = 0;

    virtual void getBoundings(iAABoxI& _boundings) = 0;

};

#endif