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

/*! tile information package to be able to generate a cetain tile
*/
struct TileInformation
{
    /*! absolute position of tile in voxel coordinates
    */
    iaVector3I _absolutePos;

    /*! level of details
    */
	uint32 _lod = 0;

    iVoxelData* _voxelData = nullptr;
    iVoxelData* _voxelDataNextLOD = nullptr;

    /*! offset to next LOD in real world coordinates
    */
    iaVector3I _voxelOffsetToNextLOD;

    /*! material ID of tile
    */
    uint32 _materialID = 0;

    /*! 
    */
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
