#include "VoxelBlock.h"

#include <iScene.h>
#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodeModel.h>
#include <iNodeMesh.h>
using namespace Igor;

#include "TaskGenerateVoxels.h"
#include "VoxelTerrainMeshGenerator.h"

float64 creationDistance[] = { 200 * 200, 400 * 400, 800 * 800, 1600 * 1600, 3000 * 3000, 6000 * 6000, 10000 * 10000, 100000 * 100000 };

VoxelBlock::VoxelBlock(uint32 lod, iaVector3I position, uint32 terrainMaterialID, iScene* scene)
{
	con_assert(lod >= 0 && lod <= 7, "lod out of range");
	_position = position;
	_lod = lod;
	_size = _voxelBlockSize * pow(2, lod);
	_terrainMaterialID = terrainMaterialID;
	_scene = scene;

	if (_lod > 0)
	{
		float32 value = pow(2, _lod - 1) - 0.5;
		_offset.set(-value, value, -value);
	}

	for (int i = 0; i < 8; ++i)
	{
		_cildren[i] = nullptr;
	}
}

VoxelBlock::~VoxelBlock()
{
	if (_voxelData != nullptr)
	{
		delete _voxelData;
	}

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
		return visible;
	}

	uint32 halfSize = _size >> 1;

	switch (_stage)
	{
	case Stage::Initial:
	{
		visible = false;

		iaVector3I blockCenterPos = _position;
		blockCenterPos._x += halfSize;
		blockCenterPos._y += halfSize;
		blockCenterPos._z += halfSize;

		float64 distance = observerPosition.distance2(blockCenterPos);

		if (distance < creationDistance[_lod])
		{
			if (_voxelBlockInfo == nullptr)
			{
				_voxelData = new iVoxelData();
				_voxelData->setMode(iaRLEMode::Compressed);
				_voxelData->setClearValue(0);

				_voxelBlockInfo = new VoxelBlockInfo();
				_voxelBlockInfo->_size.set(_voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap);
				_voxelBlockInfo->_position = _position;
				_voxelBlockInfo->_offset = _offset;
				_voxelBlockInfo->_voxelData = _voxelData;

				TaskGenerateVoxels* task = new TaskGenerateVoxels(_voxelBlockInfo, _lod, static_cast<uint32>(distance * 0.9));
				_taskID = iTaskManager::getInstance().addTask(task);
			}

			_stage = Stage::GeneratingVoxel;
		}
	}
	break;

	case Stage::GeneratingVoxel:
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
	break;

	case Stage::GeneratingMesh:
	{
		visible = false;

		iaVector3I blockCenterPos = _position;
		blockCenterPos._x += halfSize;
		blockCenterPos._y += halfSize;
		blockCenterPos._z += halfSize;

		float64 distance = observerPosition.distance2(blockCenterPos);

		if (distance < creationDistance[_lod])
		{
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
	}
	break;

	case Stage::Ready:
	{
		visible = false;
		bool meshVisible = true;
		bool destroy = false;

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

			if (distance >= creationDistance[_lod] * 3.0)
			{
				destroy = true;
			}
		}

		iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
		if (transformNode != nullptr)
		{
			transformNode->setActive(meshVisible);
		}

		if (meshVisible)
		{
			iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(_modelNodeID));
			if (modelNode != nullptr)
			{
				iNode* group = static_cast<iNodeMesh*>(modelNode->getChild("group"));
				if (group != nullptr)
				{
					iNodeMesh* meshNode = static_cast<iNodeMesh*>(group->getChild("mesh"));
					if (meshNode != nullptr)
					{
						meshNode->setVisible(true);
					}
				}
			}

			visible = true;
		}
		else if (destroy)
		{
			iNodeFactory::getInstance().destroyNode(_transformNodeID);
			_transformNodeID = iNode::INVALID_NODE_ID;
			_modelNodeID = iNode::INVALID_NODE_ID;
			_stage = Stage::Initial;

			if (_voxelData != nullptr)
			{
				delete _voxelData;
				_voxelData = nullptr;
			}

			if (_voxelBlockInfo != nullptr)
			{
				delete _voxelBlockInfo;
				_voxelBlockInfo = nullptr;
			}

			visible = false;
		}
	}
	break;
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
			tileInformation._width = _size + _voxelBlockOverlap;
			tileInformation._depth = _size + _voxelBlockOverlap;
			tileInformation._height = _size + _voxelBlockOverlap;

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
			transform->translate(_offset);

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

