#include "VoxelOperationBox.h"
#include "VoxelBlock.h"
#include "VoxelTerrain.h"

VoxelOperationBox::VoxelOperationBox(const iAABoxI& box, uint8 density)
{
    _box = box;
    _density = density;
}

void VoxelOperationBox::getBoundings(iAABoxI& boundings)
{
    boundings = _box;
}

void VoxelOperationBox::apply(VoxelBlock* voxelBlock)
{
    iVoxelData* voxelData = voxelBlock->_voxelData;

    iaVector3I from = _box._center;
    from -= _box._halfWidths;
    iaVector3I to = _box._center;
    to += _box._halfWidths;

    int64 lodFactor = pow(2, voxelBlock->_lod);
    from /= lodFactor;
    to /= lodFactor;

    iaVector3I voxelBlockFrom = voxelBlock->_positionInLOD * VoxelTerrain::_voxelBlockSize;
    voxelBlockFrom._x -= VoxelTerrain::_voxelBlockOverlap;
    voxelBlockFrom._y -= VoxelTerrain::_voxelBlockOverlap;
    voxelBlockFrom._z -= VoxelTerrain::_voxelBlockOverlap;

    from -= voxelBlockFrom;
    to -= voxelBlockFrom;

    if (from._x < 0)
    {
        from._x = 0;
    }
    if (from._y < 0)
    {
        from._y = 0;
    }
    if (from._z < 0)
    {
        from._z = 0;
    }

    int64 fullVoxelBlockSize = VoxelTerrain::_voxelBlockSize + VoxelTerrain::_voxelBlockOverlap;

    if (to._x > fullVoxelBlockSize)
    {
        to._x = fullVoxelBlockSize;
    }
    if (to._y > fullVoxelBlockSize)
    {
        to._y = fullVoxelBlockSize;
    }
    if (to._z > fullVoxelBlockSize)
    {
        to._z = fullVoxelBlockSize;
    }

    int64 poleHeight = to._y - from._y + 1;

    for (int64 x = from._x; x < to._x; ++x)
    {
        for (int64 z = from._z; z < to._z; ++z)
        {
            voxelData->setVoxelPole(iaVector3I(x, 0, z), poleHeight, _density);
        }
    }

    // TODO use iAABox
 /*   if (worldPos._x >= _from._x &&
        worldPos._x <= _to._x &&
        worldPos._y >= _from._y &&
        worldPos._y <= _to._y &&
        worldPos._z >= _from._z &&
        worldPos._z <= _to._z)
    {
        voxelData->setVoxelDensity(blockPos, _density);
    }*/

  /*      for (int64 y = 0; y < voxelData->getHeight(); ++y)
        {
            pos._y = y * lodFactor + position._y + lodOffset._y;

            if (pos._x >= 706000 &&
                pos._x <= 707000 &&
                pos._y >= 1000 &&
                pos._y <= 2000 &&
                pos._z >= 553000 &&
                pos._z <= 554000)
            {
                float64 distance = 0;
                for (auto hole : _holes)
                {
                    distance += metaballFunction(hole._center, pos) * hole._radius;
                }

                if (distance >= toMeta)
                {
                    voxelData->setVoxelDensity(iaVector3I(x, y, z), 0);
                    voxelBlockInfo->_transition = true;
                }
            }

            if (pos._x >= 759832 &&
                pos._x <= 759836 &&
                pos._y >= 4654 &&
                pos._y <= 4660 &&
                pos._z >= 381254 &&
                pos._z <= 381258)
            {
                voxelData->setVoxelDensity(iaVector3I(x, y, z), 0);
                voxelBlockInfo->_transition = true;
            }

            if (pos._x >= 759832 &&
                pos._x <= 759836 &&
                pos._y == 4653 &&
                pos._z >= 381258 &&
                pos._z <= 381262)
            {
                voxelData->setVoxelDensity(iaVector3I(x, y, z), 40);
                voxelBlockInfo->_transition = true;
            }
        }*/
}

