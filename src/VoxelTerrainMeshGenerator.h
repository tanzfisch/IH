#ifndef __GENERATORTERRAIN__
#define __GENERATORTERRAIN__

#include <iaVector3.h>
using namespace IgorAux;

#include <iModelDataIO.h>
using namespace Igor;

namespace Igor
{
    class iContouringCubes;
    class iVoxelData;
    class iMeshBuilder;
    class iTargetMaterial;
}

struct TileInformation
{
    iaVector3I _absolutePos;
	uint32 _lod = 0;
    iVoxelData* _voxelData = nullptr;
    iVoxelData* _voxelDataNextLOD = nullptr;
    iaVector3f _offsetToNextLOD;
    uint32 _materialID = 0;
	uint32 _neighborsLOD = 0;
};

class VoxelTerrainMeshGenerator : public iModelDataIO
{

public:

    /*! generates terrain tiles 

    !!! ATTENTION consumes and deletes "parameter"

    \param sectionname name of tile section
    \return parameter tile parameters
    */
    iNode* importData(const iaString& sectionName, iModelDataInputParameter* parameter);

    /*! initialize members
    */
    VoxelTerrainMeshGenerator();

    /*! does nothing
    */
    virtual ~VoxelTerrainMeshGenerator() = default;

    /*! creates an instance of this class

    \returns new instance
    */
    static iModelDataIO* createInstance();

};

#endif
