#ifndef __VOXELTERRAINGENERATOR__
#define __VOXELTERRAINGENERATOR__

#include <iNode.h>
using namespace Igor;

#include <iaEvent.h>
#include <iaVector3.h>
using namespace IgorAux;

#include <unordered_map>
using namespace std;

#include "TaskGenerateVoxels.h"
#include "VoxelTerrainMeshGenerator.h"
#include "VoxelBlock.h"

namespace Igor
{
    class iNodeLODTrigger;
    class iVoxelData;
    class iScene;
}

iaEVENT(VoxelDataGeneratedEvent, VoxelDataGeneratedDelegate, void, (const iaVector3I& min, const iaVector3I& max), (min, max));

/*! 
    \todo should not be a singleton
*/
class VoxelTerrain
{

    friend class TaskVoxelTerrain;

    /*! voxel block adress hasher
    */
    class VectorHasher
    {
    public:
        size_t operator() (iaVector3I const& key) const
        {
            return (key._x << 1) ^ (key._y << 2) ^ (key._z << 3);
        }
    };

    /*! voxel block position comparsion
    */
    class VectorEqualFn
    {
    public:
        bool operator() (iaVector3I const& t1, iaVector3I const& t2) const
        {
            return t1 == t2;
        };
    };

public:

    /*! init
    */
    VoxelTerrain();

    /*! deinit
    */
    ~VoxelTerrain();
    
    /*! sets the scene and initializes the terrain

    \param scene the scene to put the terrain in
    */
    void setScene(iScene* scene);

    /*! sets lod trigger node to monitor

    \param lodTriggerID the lod trigger's id
    */
    void setLODTrigger(uint32 lodTriggerID);

    /*void castRay(const iaVector3I& from, const iaVector3I& to, iaVector3I& outside, iaVector3I& inside);
    void setVoxelDensity(iaVector3I pos, uint8 density);
    uint8 getVoxelDensity(iaVector3I pos);*/

    /*! called per frame
    */
    void onHandle();

private:

    /*! performance section
    */
    uint32 _discoverBlocksSection = 0;

    /*! performance section
    */
    uint32 _updateBlocksSection = 0;

    uint32 _debugSection = 0; // todo remove later

    /*! amount of configured LOD
    */
    static const uint32 _lodCount = 8;

    /*! visible distance for all the LOD
    */
    static const float64 _visibleDistance[_lodCount];

    /*! lowest configured LOD
    */
    static const uint32 _lowestLOD = _lodCount - 1;

    /*! voxel block discovery distance
    */
    static const int64 _voxelBlockDiscoveryDistance = 10;

    /*! the voxel data
    */
    vector<unordered_map<iaVector3I, VoxelBlock*, VectorHasher, VectorEqualFn>> _voxelBlocks;

    /*! keep observer position since last discovery
    */
	iaVector3I _lastDiscoveryPosition;

    /*! dirty flag if it is time for a rediscovery
    */
	bool _dirtyDiscovery = true;

	/*! root node of terrain
	*/
	iNode* _rootNode = nullptr;

    /*! terrain material id
    */
    uint32 _terrainMaterialID = 0;
    
    /*! lod trigger node id
    */
    uint32 _lodTrigger = iNode::INVALID_NODE_ID;

    /*! discovers if there are unknown blocks of lowest LOD near by

    \param observerPosition current observer position
    */
    void discoverBlocks(const iaVector3I& observerPosition);

    /*! updates blocks

    \param observerPosition current observer position
    */
    void updateBlocks(const iaVector3I& observerPosition);

    /*! finds and attaches neighbours of a block

    \param voxelBlock the block to find neighbour for
    */
    void attachNeighbours(VoxelBlock* voxelBlock);

    /*! detaches the blocks neighbours

    \param voxelBlock the block to detach
    */
    void detachNeighbours(VoxelBlock* voxelBlock);

    /*! clean up and release memory from a block

    \param voxelBlock the voxel block to clear
    */
    void cleanUpVoxelBlock(VoxelBlock* voxelBlock);

    /*! main handle callback
    */
    void handleVoxelBlocks();

	void update(VoxelBlock* voxelBlock, iaVector3I observerPosition);

    bool updateVisibility(VoxelBlock* voxelBlock);

	void updateMesh(VoxelBlock* voxelBlock, iaVector3I observerPosition);

    uint32 calcLODTransition(VoxelBlock* voxelBlock);

    /*! \deprecated
    */
    void setVoxelDensity(iaVector3I voxelBlock, iaVector3I voxelRelativePos, uint8 density);

    /*! init
    */
    void init(iScene* scene);

    /*! deinit
    */
    void deinit();

};

#endif