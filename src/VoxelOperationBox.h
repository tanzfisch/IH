#ifndef __VOXEL_OPERATION_BOX__
#define __VOXEL_OPERATION_BOX__

#include "VoxelOperation.h"

class VoxelOperationBox : public VoxelOperation
{

private:

    iAABoxI _box;
    uint8 _density = 0;

public:

    VoxelOperationBox(const iAABoxI& box, uint8 density);

    void apply(VoxelBlock* voxelBlock);

    void getBoundings(iAABoxI& boundings);

};

#endif