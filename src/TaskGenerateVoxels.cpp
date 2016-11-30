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
                    contour *= 3;
                }

                float64 noise = perlinNoise.getValue(iaVector3d(pos._x * 0.001, 0, pos._z * 0.001), 7, 0.55) * 0.15;
                noise += contour;

                if (noise < 0.0)
                {
                    noise *= 0.5;
                }
                else
                {
                    noise *= 2.0;
                }

                noise += 0.0025;

                if (noise < 0.0)
                {
                    noise *= 2.0;
                }

                noise += 0.0025;

                if (noise < 0.0)
                {
                    noise *= 2.0;
                }

                noise += 0.01;

                if (noise < 0.0)
                {
                    noise *= 1000.0;
                }

                noise += 0.12;

                if (noise < 0)
                {
                    noise = 0;
                }

                float64 height = (noise * 2000);
                height = 341 + (sin(pos._x * 0.125) + sin(pos._z * 0.125)) * 5.0;

                float64 transdiff = height - static_cast<float64>(position._y) - offset._y;
                if (transdiff > 0 && transdiff <= voxelData->getHeight() * _lodFactor)
                {
                    _voxelBlockInfo->_transition = true;
                }

                float64 diff = (transdiff / static_cast<float64>(_lodFactor));
                if (diff > 0)
                {
                    if (diff > size._y)
                    {
                        diff = size._y;
                    }

                    int64 diffi = static_cast<uint64>(diff);
                    if (diffi > 0)
                    {
                        voxelData->setVoxelPole(iaVector3I(x, 0, z), diffi, 128);
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

                        if (pos._y > 180 &&
                            pos._y > height - 50 &&
                            pos._y < height + 10)
                        {
                            float64 onoff = perlinNoise.getValue(iaVector3d(pos._x * 0.005, pos._y * 0.005, pos._z * 0.005), 4, 0.5);

                            float64 diff = (pos._y - (height - 50)) / 50.0;
                            onoff += (1.0 - diff) * 0.1;

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

                    if (pos._y > 0 && pos._y < 2000)
                    {
                        float64 onoff = perlinNoise.getValue(iaVector3d(pos._x * 0.001, pos._y * 0.001, pos._z * 0.001), 4, 0.5);

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
                                voxelData->setVoxelDensity(iaVector3I(x, y, z), 128);
                                _voxelBlockInfo->_transition = true;
                            }
                        }
                    }
                }
            }
        }
#endif

        _voxelBlockInfo->_generatedVoxels = true;
    }

    finishTask();
}

