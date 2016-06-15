#include "TaskGenerateVoxels.h""

#include <iModelResourceFactory.h>
#include <iTextureResourceFactory.h>
#include <iWindow.h>
#include <iPerlinNoise.h>
#include <iVoxelData.h>
#include <iSphere.h>
#include <iTimer.h>
using namespace Igor;

#include <iaConsole.h>
using namespace IgorAux;

int32 TaskGenerateVoxels::_seed = 0;
vector<iSpheref> TaskGenerateVoxels::_metaballs;
vector<iSpheref> TaskGenerateVoxels::_holes;
mutex TaskGenerateVoxels::_initMutex;

TaskGenerateVoxels::TaskGenerateVoxels(VoxelBlock* voxelBlock, uint32 priority)
	: iTask(nullptr, priority, false, iTaskContext::Default)
{
	con_assert(voxelBlock != nullptr, "zero pointer");
	con_assert(voxelBlock->_voxelData != nullptr, "zero pointer");

	_voxelBlock = voxelBlock;
}

void TaskGenerateVoxels::run()
{   
	iPerlinNoise perlinNoise;

	iVoxelData* voxelData = _voxelBlock->_voxelData;
	iaVector3I& offset = _voxelBlock->_offset;
	iaVector3i& size = _voxelBlock->_size;
    
    voxelData->setClearValue(0);
	voxelData->initData(size._x, size._y, size._z);

    // skip all the voxel blocks that are too far away
    const float64 from = 0.444;
    const float64 to = 0.45;
    float64 factor = 1.0 / (to - from);

    const float64 fromMeta = 0.017;
    const float64 toMeta = 0.0175;
    float64 factorMeta = 1.0 / (toMeta - fromMeta);

    for (int64 x = 0; x < voxelData->getWidth() - 0; ++x)
    {
        for (int64 y = 0; y < voxelData->getHeight() - 0; ++y)
        {
            float32 density = 0;

            // first figure out if a voxel is outside the sphere
            iaVector3f pos(x + offset._x, y + offset._y, offset._z);

            float64 height = perlinNoise.getValue(iaVector3d(pos._x * 0.04, pos._y * 0.04, 0), 4, 0.5);
            if (height > offset._z + size._z)
            {
            //    voxelData->setVoxelLine(iaVector3I(offset._x, offset._y, 0), 255);
            }
        }
    }/**/

	_voxelBlock->_generatedVoxels = true;

	finishTask();
}

