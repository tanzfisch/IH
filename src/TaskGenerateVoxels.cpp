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
	: iTask(nullptr, priority, false)
{
	con_assert(voxelBlock != nullptr, "zero pointer");
	con_assert(voxelBlock->_voxelData != nullptr, "zero pointer");

	_voxelBlock = voxelBlock;
}

void TaskGenerateVoxels::run()
{   
	iPerlinNoise perlinNoise; // TODO move from here

	iVoxelData* voxelData = _voxelBlock->_voxelData;
	iaVector3I& offset = _voxelBlock->_offset;
	iaVector3i& size = _voxelBlock->_size;
    
    voxelData->setClearValue(0);
	voxelData->initData(size._x, size._y, size._z);

    for (int64 x = 0; x < voxelData->getWidth(); ++x)
    {
        for (int64 z = 0; z < voxelData->getDepth(); ++z)        
		{
			iaVector3f pos(x + offset._x, 0, z + offset._z);
            float64 contour = perlinNoise.getValue(iaVector3d(pos._x * 0.0005, 0, pos._z * 0.0005), 3, 0.5);
			float64 noise = perlinNoise.getValue(iaVector3d(pos._x * 0.003, 0, pos._z * 0.003), 6, 0.65);

            noise *= contour;
            noise -= 0.3;

			if (noise < 0.0)
			{
				noise *= 0.25;
			}

			noise += 0.005;

            if (noise < 0.0)
            {
                noise *= 3.0;
            }

            noise += 0.02;

			if (noise < 0)
			{
				noise = 0;
			}

			float64 height = noise * 600;

			float64 diff = height - static_cast<float64>(offset._y) - 1.0;
			if (diff > 0)
			{
                if (diff > size._y)
                {
                    diff = size._y;
                }

				_voxelBlock->_changedVoxels = true;
				int64 diffi = static_cast<uint64>(diff);
				if (diffi > 0)
				{
					voxelData->setVoxelPole(iaVector3I(x, 0, z), diffi, 255);
				}

				diff -= static_cast<float64>(diffi);
		        voxelData->setVoxelDensity(iaVector3I(x, diffi, z), (diff * 254) + 1);
			}
		}
    }

	_voxelBlock->_generatedVoxels = true;
	finishTask();
}

