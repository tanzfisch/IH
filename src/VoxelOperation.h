#ifndef __VOXEL_OPERATION__
#define __VOXEL_OPERATION__

#include <iaVector3.h>
using namespace IgorAux;

class VoxelBlock;

struct VoxelOperation
{

public:

    virtual void apply(VoxelBlock* voxelBlock, const iaVector3I& worldPos, const iaVector3I& blockPos) = 0;

    virtual void getRange(iaVector3I& from, iaVector3I& to) = 0;

};

#endif