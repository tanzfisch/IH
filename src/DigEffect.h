#ifndef __DIGEFFECT__
#define __DIGEFFECT__

#include "Entity.h"
#include <iNode.h>

namespace Igor
{
    class iScene;
}

class DigEffect : public Entity
{

public:

    DigEffect(iScene* scene, const iaMatrixf& matrix);
    virtual ~DigEffect();

private:

    uint32 _particleSystemNodeID = iNode::INVALID_NODE_ID;
	uint32 _transformNodeID = iNode::INVALID_NODE_ID;

	iaVector3f _pos;

    void handle();
    iaVector3f updatePos();
    void hitBy(uint64 entityID);

};

#endif