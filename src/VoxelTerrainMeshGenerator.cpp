#include "VoxelTerrainMeshGenerator.h"

#include <iVoxelData.h>
#include <iContouringCubes.h>
#include <iNodeFactory.h>
#include <iNodeMesh.h>
#include <iNodeLODSwitch.h>
#include <iModel.h>
#include <iaMemBlock.h>
#include <iMeshBuilder.h>
#include <iMaterialResourceFactory.h>
#include <iTextureResourceFactory.h>
#include <iTargetMaterial.h>
#include <iNodeTransform.h>
#include <iNodePhysics.h>
using namespace Igor;

#include "EntityManager.h"

VoxelTerrainMeshGenerator::VoxelTerrainMeshGenerator()
{
    _identifier = "vtg";
    _name = "Voxel Terrain Generator";
}

iModelDataIO* VoxelTerrainMeshGenerator::createInstance()
{
    VoxelTerrainMeshGenerator* result = new VoxelTerrainMeshGenerator();
    return static_cast<iModelDataIO*>(result);
}

iNode* VoxelTerrainMeshGenerator::importData(const iaString& sectionName, iModelDataInputParameter* parameter)
{
    TileInformation* tileInformation = reinterpret_cast<TileInformation*>(parameter->_parameters.getDataPointer());
    const iaVector3I& absolutePos = tileInformation->_absolutePos;
    int64 width = tileInformation->_width;
    int64 depth = tileInformation->_depth;
    int64 height = tileInformation->_height;
	float64 scale = pow(2, tileInformation->_lod);
    iVoxelData* voxelData = tileInformation->_voxelData;

    if (width >= voxelData->getWidth())
    {
        width = voxelData->getWidth() - 1;
    }

    if (depth >= voxelData->getDepth())
    {
        depth = voxelData->getDepth() - 1;
    }

    if (height >= voxelData->getHeight())
    {
        height = voxelData->getHeight() - 1;
    }

    iNode* result = iNodeFactory::getInstance().createNode(iNodeType::iNode);

    //voxelData->setMode(iaRLEMode::Uncompressed);

    iContouringCubes contouringCubes;
    contouringCubes.setVoxelData(voxelData);

    shared_ptr<iMesh> mesh = contouringCubes.compile(iaVector3I(), iaVector3I(width, height, depth), scale);

    if (mesh.get() != nullptr)
    {
        iNodeMesh* meshNode = static_cast<iNodeMesh*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeMesh));
        meshNode->setMesh(mesh);
        meshNode->setMaterial(tileInformation->_materialID);

		iTargetMaterial* targetMaterial = meshNode->getTargetMaterial();
#if 1
		targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("dirt.png"), 0);
		targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("grass.png"), 1);
		targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("rock.png"), 2);
#else
		switch (tileInformation->_lod)
		{
		case 0:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("dirt.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("grass.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("rock.png"), 2);
			break;
		case 1:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("green.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("green.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("green.png"), 2);
			break;
		case 2:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("blue.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("blue.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("blue.png"), 2);
			break;
		case 3:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("red.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("red.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("red.png"), 2);
			break;
		case 4:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("yellow.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("yellow.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("yellow.png"), 2);
			break;
		case 5:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("cyan.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("cyan.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("cyan.png"), 2);
			break;
		case 6:
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("magenta.png"), 0);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("magenta.png"), 1);
			targetMaterial->setTexture(iTextureResourceFactory::getInstance().requestFile("magenta.png"), 2);
			break;
		}
#endif
        targetMaterial->setAmbient(iaColor3f(0.7f, 0.7f, 0.7f));
        targetMaterial->setDiffuse(iaColor3f(0.9f, 0.9f, 0.9f));
        targetMaterial->setSpecular(iaColor3f(0.1f, 0.1f, 0.1f));
        targetMaterial->setEmissive(iaColor3f(0.05f, 0.05f, 0.05f));
        targetMaterial->setShininess(100.0f);

        result->insertNode(meshNode);

		/*if (tileInformation->_lod == 0)
		{
			iNodePhysics* physicsNode = static_cast<iNodePhysics*>(iNodeFactory::getInstance().createNode(iNodeType::iNodePhysics));
			iaMatrixf offset;
			physicsNode->addMesh(mesh, 1, offset);
			physicsNode->finalizeCollision(true);
			physicsNode->setMaterial(EntityManager::getInstance().getTerrainMaterialID());

			result->insertNode(physicsNode);
		}*/
    }

    //voxelData->setMode(iaRLEMode::Compressed);

    return result;
}

