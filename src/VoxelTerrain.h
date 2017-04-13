#ifndef __VOXELTERRAINGENERATOR__
#define __VOXELTERRAINGENERATOR__

#include <iNode.h>
#include <iNodeFactory.h>
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
    VoxelTerrain(GenerateVoxelsDelegate generateVoxelsDelegate);

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

	/*! \returns terrain material ID
	*/
	uint64 getMaterial() const;

private:

    /*! delegate registered by application to generate voxel data
    */
    GenerateVoxelsDelegate _generateVoxelsDelegate;

    /*! queue of actions
    */
    vector<iNodeFactory::iAction> _actionQueue;

    /*! mutex to protect action queue
    */
    mutex _mutexActionQueue;

    /*! performance section discover blocks
    */
    uint32 _discoverBlocksSection = 0;

    /*! performance section delete blocks
    */
    uint32 _deleteBlocksSection = 0;

    /*! performance section apply actions
    */
    uint32 _applyActionsSection = 0;

    /*! performance section update blocks
    */
    uint32 _updateBlocksSection = 0;

    /*! performance section update visibility blocks
    */
    uint32 _updateVisBlocksSection = 0;

    /*! performance section total handle
    */
    uint32 _totalSection = 0;

    /*! lowest allowed lod
    */
    static const uint32 _lowestLOD = 10; // max 10

    /*! voxel block discovery distance
    */
    static const int64 _voxelBlockDiscoveryDistance = 8;

    /*! voxel block setup distance
    */
    static const int64 _voxelBlockSetupDistance = 4;

    /*! block quibic size
    */
    static const int32 _voxelBlockSize = 32;

    /*! block overlap
    */
    static const int32 _voxelBlockOverlap = 2;

    /*! the voxel data
    */
    vector<unordered_map<iaVector3I, VoxelBlock*, VectorHasher, VectorEqualFn>> _voxelBlocks;

    /*! map of voxel block IDs to voxel blocks
    */
    map<uint64, VoxelBlock*> _voxelBlocksMap;

    /*! next voxel block id
    */
    uint64 _nextVoxelBlockID = 1;

    /*! map of voxel blocks that have to be deleted
    */
    vector<VoxelBlock*> _voxelBlocksToDelete;

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

    void collectBlocksToDelete(VoxelBlock* currentBlock, vector<VoxelBlock*>& dst);

    void setNeighboursDirty(VoxelBlock* voxelBlock);
    void setInRange(VoxelBlock* voxelBlock, bool inRange);

    VoxelBlock* createVoxelBlock(uint32 lod, iaVector3I parentPositionInLOD, uint8 childAdress);

    void deleteBlocks();
    void deleteBlock(VoxelBlock* voxelBlock);
    bool canBeDeleted(VoxelBlock* voxelBlock);

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

    /*! main handle callback
    */
    void handleVoxelBlocks();

    void update(VoxelBlock* voxelBlock, iaVector3I observerPosition);

    bool updateVisibility(VoxelBlock* voxelBlock);

    void updateMesh(VoxelBlock* voxelBlock);
    void finalizeMesh(VoxelBlock* voxelBlock);

    void setNodeActiveAsync(iNode* node, bool active);
    void insertNodeAsync(iNode* src, iNode* dst);
    void removeNodeAsync(iNode* src, iNode* dst);
    void destroyNodeAsync(uint32 nodeID);

    uint8 calcLODTransition(VoxelBlock* voxelBlock);

    /*! init
    */
    void init(iScene* scene);

    /*! deinit
    */
    void deinit();

};

#endif
