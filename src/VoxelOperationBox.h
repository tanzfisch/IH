#ifndef __VOXEL_OPERATION_BOX__
#define __VOXEL_OPERATION_BOX__

#include "VoxelOperation.h"

struct VoxelOperationBox : public VoxelOperation
{

private:

    iaVector3I _from;
    iaVector3I _to;
    uint8 _density = 0;

public:

    VoxelOperationBox(iaVector3I from, iaVector3I to, uint8 density);

    void apply(VoxelBlock* voxelBlock, const iaVector3I& worldPos, const iaVector3I& blockPos);

    void getRange(iaVector3I& from, iaVector3I& to);

};

#endif