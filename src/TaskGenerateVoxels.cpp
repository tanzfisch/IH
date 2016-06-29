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

TaskGenerateVoxels::TaskGenerateVoxels(VoxelBlockInfo* voxelBlockInfo, uint32 lod, uint32 priority)
	: iTask(nullptr, priority, false)
{
	con_assert(voxelBlockInfo != nullptr, "zero pointer");
	con_assert(voxelBlockInfo->_voxelData != nullptr, "zero pointer");
	con_assert(lod >= 0, "lod out of range");

	_lodFactor = pow(2, lod);
	_voxelBlockInfo = voxelBlockInfo;
}

void TaskGenerateVoxels::run()
{   
	iPerlinNoise perlinNoise; // TODO move from here

	iVoxelData* voxelData = _voxelBlockInfo->_voxelData;
	iaVector3I& position = _voxelBlockInfo->_position;
	iaVector3i& size = _voxelBlockInfo->_size;
    
    voxelData->setClearValue(0);
	voxelData->initData(size._x, size._y, size._z);

    for (int64 x = 0; x < voxelData->getWidth(); ++x)
    {
        for (int64 z = 0; z < voxelData->getDepth(); ++z)        
		{
			iaVector3f pos(x * _lodFactor + position._x, 0, z * _lodFactor + position._z);

            float64 contour = perlinNoise.getValue(iaVector3d(pos._x * 0.0001, 0, pos._z * 0.0001), 3, 0.6);
			contour -= 0.7;

			if (contour > 0.0)
			{
				contour *= 1.0 / 0.3;
			}

			float64 noise = perlinNoise.getValue(iaVector3d(pos._x * 0.001, 0, pos._z * 0.001), 7, 0.55) * 0.15;
            noise += contour;

			if (noise < 0.0)
			{
				noise *= 0.25;
			}
			else
			{
				noise *= 2.0;
			}

			noise += 0.005;

            if (noise < 0.0)
            {
                noise *= 3.0;
            }

            noise += 0.1;

			if (noise < 0)
			{
				noise = 0;
			}

			float64 height = (noise * 2000);

			float64 diff = (height - static_cast<float64>(position._y)) / _lodFactor - 1.0;
			if (diff > 0)
			{
                if (diff > size._y)
                {
                    diff = size._y;
                }

				int64 diffi = static_cast<uint64>(diff);
				if (diffi > 0)
				{
					voxelData->setVoxelPole(iaVector3I(x, 0, z), diffi, 255);
				}

				if (diffi < voxelData->getHeight())
				{
					diff -= static_cast<float64>(diffi);
					voxelData->setVoxelDensity(iaVector3I(x, diffi, z), (diff * 254) + 1);
				}
			}

			if (abs(diff + size._y * 0.5) < size._y)
			{
				_voxelBlockInfo->_transition = true;
			}
		}
    }

	_voxelBlockInfo->_generatedVoxels = true;

	finishTask();
}

