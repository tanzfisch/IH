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

    _lodFactor = static_cast<uint32>(pow(2, lod));
    _voxelBlockInfo = voxelBlockInfo;
}

void TaskGenerateVoxels::run()
{
    iPerlinNoise perlinNoise; // TODO move from here

    iVoxelData* voxelData = _voxelBlockInfo->_voxelData;
    iaVector3I& position = _voxelBlockInfo->_position;
    iaVector3f& offset = _voxelBlockInfo->_offset;
    iaVector3i& size = _voxelBlockInfo->_size;

    const float64 from = 0.35;
    const float64 to = 0.36;
    float64 factor = 1.0 / (to - from);

    if (voxelData != nullptr)
    {
        voxelData->initData(size._x, size._y, size._z);

#if 1
        for (int64 x = 0; x < voxelData->getWidth(); ++x)
        {
            for (int64 z = 0; z < voxelData->getDepth(); ++z)
            {
                iaVector3f pos(x * _lodFactor + position._x + offset._x, 0, z * _lodFactor + position._z + offset._z);

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

                //height = 128 + (sin(pos._x * 0.5) + sin(pos._z * 0.5)) * 0.499;
                //height = 512;

                float64 transdiff = height - static_cast<float64>(position._y);
                if (transdiff > 0 && transdiff <= voxelData->getHeight() * _lodFactor)
                {
                    _voxelBlockInfo->_transition = true;
                }

                float64 diff = (transdiff) / _lodFactor - 1.0;
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

                float64 cavelikeliness = perlinNoise.getValue(iaVector3d(pos._x * 0.0001 + 12345, 0, pos._z * 0.0001 + 12345), 3, 0.6);
                cavelikeliness -= 0.5;
                if (cavelikeliness > 0.0)
                {
                    cavelikeliness *= 1.0 / 0.5;
                    cavelikeliness *= 5;
                }

                if (cavelikeliness > 0)
                {
                    for (int64 y = 0; y < voxelData->getHeight(); ++y)
                    {
                        pos._y = y * _lodFactor + position._y + offset._y;

                        if (pos._y > 200 &&
                            pos._y > height - (50 * cavelikeliness) &&
                            pos._y < height + 10)
                        {
                            float64 onoff = perlinNoise.getValue(iaVector3d(pos._x * 0.005, pos._y * 0.01, pos._z * 0.005), 4, 0.5);
                            if (onoff <= from)
                            {
                                if (onoff >= to)
                                {
                                    float64 gradient = 1.0 - ((onoff - from) * factor);
                                    voxelData->setVoxelDensity(iaVector3I(x, y, z), (gradient * 254) + 1);
                                    _voxelBlockInfo->_transition = true;
                                }
                                else
                                {
                                    voxelData->setVoxelDensity(iaVector3I(x, y, z), 0);
                                    _voxelBlockInfo->_transition = true;
                                }
                            }
                        }
                    }
                }
            }
        }
#else
        for (int64 x = 0; x < voxelData->getWidth(); ++x)
        {
            for (int64 y = 0; y < voxelData->getHeight(); ++y)
            {
                for (int64 z = 0; z < voxelData->getDepth(); ++z)
                {
                    iaVector3f pos(x * _lodFactor + position._x + offset._x, 
                        y * _lodFactor + position._y + offset._y, 
                        z * _lodFactor + position._z + offset._z);

                    float32 dx = fmod(pos._x, 32);
                    float32 dy = fmod(pos._y, 32);
                    float32 dz = fmod(pos._z, 32);

                    if (dx > 12 && dx < 16 &&
                        dy > 12 && dy < 16 &&
                        dz > 12 && dz < 16)
                    {
                        voxelData->setVoxelDensity(iaVector3I(x, y, z), 255);
                        _voxelBlockInfo->_transition = true;
                    }
                }
            }
        }
#endif

        _voxelBlockInfo->_generatedVoxels = true;
    }

    finishTask();
}

