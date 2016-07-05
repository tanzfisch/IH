#include "VoxelBlock.h"

#include <iScene.h>
#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodeModel.h>
using namespace Igor;

#include "TaskGenerateVoxels.h"
#include "VoxelTerrainMeshGenerator.h"

float64 creationDistance[] = { 250 * 250, 500 * 500, 1000 * 1000, 2000 * 2000, 4000 * 4000, 8000 * 8000, 16000 * 16000, 32000 * 32000 };

VoxelBlock::VoxelBlock(uint32 lod, iaVector3I position, uint32 terrainMaterialID, iScene* scene)
{
    con_assert(lod >= 0 && lod <= 7, "lod out of range");
    _position = position;
    _lod = lod;
    _size = _voxelBlockSize * pow(2, lod);
    _terrainMaterialID = terrainMaterialID;
    _scene = scene;

    for (int i = 0; i < 8; ++i)
    {
        _cildren[i] = nullptr;
    }
}

VoxelBlock::~VoxelBlock()
{
    if (_voxelBlockInfo != nullptr)
    {
        delete _voxelBlockInfo;
    }
}

bool VoxelBlock::update(iaVector3I observerPosition)
{
    bool visible = true;

    if (_stage == Stage::Empty)
    {
        visible = true;
        return visible;
    }

    uint32 halfSize = _size >> 1;

    if (_stage == Stage::Initial)
    {
        visible = false;

        iaVector3I blockCenterPos = _position;
        blockCenterPos._x += halfSize;
        blockCenterPos._y += halfSize;
        blockCenterPos._z += halfSize;

        float64 distance = observerPosition.distance2(blockCenterPos);

        if (distance < creationDistance[_lod])
        {
            _voxelData = new iVoxelData();
            _voxelData->setMode(iaRLEMode::Compressed);
            _voxelData->setClearValue(0);

            _voxelBlockInfo = new VoxelBlockInfo();
            _voxelBlockInfo->_size.set(_voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap);
            _voxelBlockInfo->_position = _position;
            _voxelBlockInfo->_voxelData = _voxelData;

            _stage = Stage::GeneratingVoxel;

            TaskGenerateVoxels* task = new TaskGenerateVoxels(_voxelBlockInfo, _lod, static_cast<uint32>(distance * 0.9));
            _taskID = iTaskManager::getInstance().addTask(task);
        }
    }
    else if (_stage == Stage::GeneratingVoxel)
    {
        visible = false;

        iTask* task = iTaskManager::getInstance().getTask(_taskID);
        if (task == nullptr)
        {
            _taskID = iTask::INVALID_TASK_ID;
            if (!_voxelBlockInfo->_transition)
            {
                delete _voxelData;
                _voxelData = nullptr;
                _voxelBlockInfo->_voxelData = nullptr;

                _stage = Stage::Empty;
            }
            else
            {
                _stage = Stage::GeneratingMesh;
            }
        }
    }
    else if (_stage == Stage::GeneratingMesh)
    {
        visible = false;

        updateMesh();

        if (_lod > 0)
        {
            if (_cildren[0] == nullptr)
            {
                _cildren[0] = new VoxelBlock(_lod - 1, _position, _terrainMaterialID, _scene);
                _cildren[1] = new VoxelBlock(_lod - 1, _position + iaVector3I(halfSize, 0, 0), _terrainMaterialID, _scene);
                _cildren[2] = new VoxelBlock(_lod - 1, _position + iaVector3I(halfSize, 0, halfSize), _terrainMaterialID, _scene);
                _cildren[3] = new VoxelBlock(_lod - 1, _position + iaVector3I(0, 0, halfSize), _terrainMaterialID, _scene);

                _cildren[4] = new VoxelBlock(_lod - 1, _position + iaVector3I(0, halfSize, 0), _terrainMaterialID, _scene);
                _cildren[5] = new VoxelBlock(_lod - 1, _position + iaVector3I(halfSize, halfSize, 0), _terrainMaterialID, _scene);
                _cildren[6] = new VoxelBlock(_lod - 1, _position + iaVector3I(halfSize, halfSize, halfSize), _terrainMaterialID, _scene);
                _cildren[7] = new VoxelBlock(_lod - 1, _position + iaVector3I(0, halfSize, halfSize), _terrainMaterialID, _scene);
            }
        }
    }
    else if (_stage == Stage::Ready)
    {
        visible = false;
        bool meshVisible = true;

        iaVector3I blockCenterPos = _position;
        blockCenterPos._x += halfSize;
        blockCenterPos._y += halfSize;
        blockCenterPos._z += halfSize;

        float32 distance = observerPosition.distance2(blockCenterPos);

        if (_lod > 0)
        {
            bool childrenVisible = true;
            for (int i = 0; i < 8; ++i)
            {
                if (!_cildren[i]->update(observerPosition))
                {
                    childrenVisible = false;
                }
            }

            if (childrenVisible)
            {
                visible = true;
                meshVisible = false;
            }
            else
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (_cildren[i]->_stage == Stage::Ready)
                    {
                        iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_cildren[i]->_transformNodeID));
                        if (transformNode != nullptr)
                        {
                            transformNode->setActive(false);
                        }
                    }
                }
            }
        }

        if (distance >= creationDistance[_lod])
        {
            meshVisible = false;
        }

        iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
        if (transformNode != nullptr)
        {
            transformNode->setActive(meshVisible);
        }

        if (meshVisible)
        {
            visible = true;
        }
    }

    return visible;
}

void VoxelBlock::updateMesh()
{
    if (_transformNodeID == iNode::INVALID_NODE_ID)
    {
        if (_voxelData != nullptr)
        {
            TileInformation tileInformation;
            tileInformation._materialID = _terrainMaterialID;
            tileInformation._voxelData = _voxelData;
            tileInformation._lod = _lod;
            tileInformation._width = _size + _tileOverlap;
            tileInformation._depth = _size + _tileOverlap;
            tileInformation._height = _size + _tileOverlap;

            iModelDataInputParameter* inputParam = new iModelDataInputParameter(); // will be deleted by iModel
            inputParam->_identifier = "vtg";
            inputParam->_joinVertexes = true;
            inputParam->_needsRenderContext = false;
            inputParam->_modelSourceType = iModelSourceType::Generated;
            inputParam->_loadPriority = 0;
            inputParam->_parameters.setData(reinterpret_cast<const char*>(&tileInformation), sizeof(TileInformation));

            iaString tileName = iaString::itoa(_position._x);
            tileName += ":";
            tileName += iaString::itoa(_position._y);
            tileName += ":";
            tileName += iaString::itoa(_position._z);
            tileName += ":";
            tileName += iaString::itoa(_lod);
            tileName += ":";
            tileName += iaString::itoa(_mutationCounter++);

            iNodeTransform* transform = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
            transform->translate(_position._x, _position._y, _position._z);

            iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
            modelNode->setModel(tileName, inputParam);

            transform->insertNode(modelNode);

            _scene->getRoot()->insertNode(transform);

            _transformNodeID = transform->getID();
            _modelNodeID = modelNode->getID();
        }
    }
    else
    {
        iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(_modelNodeID));
        if (modelNode != nullptr &&
            modelNode->isLoaded())
        {
            _stage = Stage::Ready;
        }
    }
}

