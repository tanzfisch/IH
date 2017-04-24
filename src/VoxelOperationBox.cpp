#include "VoxelOperationBox.h"
#include "VoxelBlock.h"

VoxelOperationBox::VoxelOperationBox(iaVector3I from, iaVector3I to, uint8 density)
{
    _from = from;
    _to = to;
    _density = density;
}

void VoxelOperationBox::getRange(iaVector3I& from, iaVector3I& to)
{
    from = _from;
    to = _to;
}

void VoxelOperationBox::apply(VoxelBlock* voxelBlock, const iaVector3I& worldPos, const iaVector3I& blockPos)
{
    iVoxelData* voxelData = voxelBlock->_voxelData;

    // TODO use iAABox
    if (worldPos._x >= _from._x &&
        worldPos._x <= _to._x &&
        worldPos._y >= _from._y &&
        worldPos._y <= _to._y &&
        worldPos._z >= _from._z &&
        worldPos._z <= _to._z)
    {
        voxelData->setVoxelDensity(blockPos, _density);
    }

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

