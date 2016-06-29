#include "VoxelBlock.h"

#include <iScene.h>
#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodeModel.h>
using namespace Igor;

#include "TaskGenerateVoxels.h"
#include "VoxelTerrainMeshGenerator.h"

float64 creationDistance[] = { 500 * 500, 1200 * 1200, 2500 * 2500, 5000 * 5000, 10000 * 10000, 20000 * 20000, 40000 * 40000 };

VoxelBlock::VoxelBlock(uint32 lod, iaVector3I position, uint32 terrainMaterialID, iScene* scene)
{
	con_assert(lod >= 0 && lod <= 6, "lod out of range");
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

void VoxelBlock::update(iaVector3I observerPosition, bool closeEnough)
{
	if (_taskID != iTask::INVALID_TASK_ID)
	{
		iTask* task = iTaskManager::getInstance().getTask(_taskID);
		if (task != nullptr)
		{
			return;
		}
		else
		{
			_taskID = iTask::INVALID_TASK_ID;
			if (!_voxelBlockInfo->_transition)
			{
				delete _voxelData;
				_voxelData = nullptr;
				_voxelBlockInfo->_voxelData = nullptr;
			}
		}
	}

	uint32 halfSize = _size >> 1;

	iaVector3I blockCenterPos = _position;
	blockCenterPos._x += halfSize;
	blockCenterPos._y += halfSize;
	blockCenterPos._z += halfSize;

	float32 distance = observerPosition.distance2(blockCenterPos);

	if (closeEnough || distance < creationDistance[_lod])
	{
		if (_voxelBlockInfo == nullptr)
		{
			_voxelData = new iVoxelData();
			_voxelData->setMode(iaRLEMode::Compressed);
			_voxelData->setClearValue(0);

			_voxelBlockInfo = new VoxelBlockInfo();
			_voxelBlockInfo->_size.set(_voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap, _voxelBlockSize + _voxelBlockOverlap);
			_voxelBlockInfo->_position = _position;
			_voxelBlockInfo->_voxelData = _voxelData;

			TaskGenerateVoxels* task = new TaskGenerateVoxels(_voxelBlockInfo, _lod, static_cast<uint32>(distance * 0.9));
			_taskID = iTaskManager::getInstance().addTask(task);
		}

		if (_lod > 0 && _voxelBlockInfo->_transition)
		{
			if (distance >= creationDistance[_lod - 1])
			{
				if (_dirtyMesh)
				{
					updateMesh();
				}
				else
				{
					if (_cildren[0] != nullptr)
					{
						if (distance >= creationDistance[_lod] * 0.5)
						{
							for (int i = 0; i < 8; ++i)
							{
								if (_cildren[i]->_transformNodeID != iNode::INVALID_NODE_ID)
								{
									iNodeFactory::getInstance().destroyNode(_cildren[i]->_transformNodeID);
									_cildren[i]->_transformNodeID = iNode::INVALID_NODE_ID;
									_cildren[i]->_dirtyMesh;
								}
							}
						}
						else
						{
							for (int i = 0; i < 8; ++i)
							{
								iNodeTransform* childTransformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_cildren[i]->_transformNodeID));
								if (childTransformNode != nullptr)
								{
									childTransformNode->setActive(false);
								}
							}
						}
					}
				}
			}
			else
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

				bool childrenDone = true;
				for (int i = 0; i < 8; ++i)
				{
					_cildren[i]->update(observerPosition, true);
				}

				iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
				if (transformNode != nullptr)
				{
					if (transformNode->isActive())
					{
						bool childrensdone = true;
						for (int i = 0; i < 8; ++i)
						{
							if (_cildren[i]->_dirtyMesh)
							{
								childrensdone = false;
							}
						}

						if (childrensdone)
						{
							transformNode->setActive(false);
						}
					}
				}
			}
		}
	}

	/*if (lod == 0 &&
	!block->_generatedEnemies)
	{
	iaVector3I blockMax = blockPos;
	blockMax._x += _voxelBlockSize;
	blockMax._y += _voxelBlockSize;
	blockMax._z += _voxelBlockSize;
	_dataGeneratedEvent(blockPos, blockMax);
	block->_generatedEnemies = true;
	}*/

}

void VoxelBlock::updateMesh()
{
	TileInformation tileInformation;
	tileInformation._materialID = _terrainMaterialID;
	tileInformation._voxelData = _voxelData;
	tileInformation._lod = _lod;

	if (_transformNodeID == iNode::INVALID_NODE_ID)
	{
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
	else
	{
		if (_dirtyMesh)
		{
			iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode(_modelNodeID));
			if (modelNode != nullptr &&
				modelNode->isLoaded())
			{
				_dirtyMesh = false;
			}
		}

		iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
		if (transformNode != nullptr)
		{
			transformNode->setActive();
		}
	}

	/*if (!(*tileIter).second._destroyNodeIDs.empty())
	{
		bool destroy = false;
		iNodeModel* modelNode = static_cast<iNodeModel*>(iNodeFactory::getInstance().getNode((*tileIter).second._modelNodeID));
		if (modelNode != nullptr)
		{
			if (modelNode->isLoaded())
			{
				destroy = true;
			}
		}
		else
		{
			destroy = true;
		}

		if (destroy)
		{
			for (auto id : (*tileIter).second._destroyNodeIDs)
			{
				iNode* node = iNodeFactory::getInstance().getNode(id);
				if (node != nullptr)
				{
					iNodeFactory::getInstance().destroyNode(node);
				}
			}

			(*tileIter).second._destroyNodeIDs.clear();
		}
	}*/

	/*if (distance > _tileDestructionDistance)
	{
		if ((*tileIter).second._transformNodeID != iNode::INVALID_NODE_ID)
		{
			iNodeFactory::getInstance().destroyNode((*tileIter).second._transformNodeID);
		}

		if (!(*tileIter).second._destroyNodeIDs.empty())
		{
			for (auto id : (*tileIter).second._destroyNodeIDs)
			{
				iNodeFactory::getInstance().destroyNode(id);
			}

			(*tileIter).second._destroyNodeIDs.clear();
		}

		_tileDataSets.erase(tileIter);
	}*/
}

