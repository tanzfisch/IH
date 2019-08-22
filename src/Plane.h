#ifndef __PLANE__
#define __PLANE__

#include <iNode.h>
using namespace Igor;

#include <iaMatrix.h>
using namespace IgorAux;

namespace Igor
{
    class iScene;
    class iPhysicsBody;
    class iPhysicsJoint;
    class iView;
    class iWindow;
}

class Plane
{

public:

    Plane(iScene* scene, iView* view, const iaMatrixd& matrix);
    virtual ~Plane();

    void startRollLeft();
    void stopRollLeft();
    void startRollRight();
    void stopRollRight();

	void startRollUp();
	void stopRollUp();
	void startRollDown();
	void stopRollDown();

	void startFastTravel();
	void stopFastTravel();

    void drawReticle(const iWindow& window);

    uint32 getLODTriggerID();

    void setPosition(iaVector3d pos);

private:

	bool _fastTravel = false;
	bool _rollLeft = false;
	bool _rollRight = false;
	bool _rollUp = false;
	bool _rollDown = false;

	uint64 _physicsNodeID = iNode::INVALID_NODE_ID;
	uint64 _transformNodeID = iNode::INVALID_NODE_ID;

    uint64 _lodTriggerID = iNode::INVALID_NODE_ID;
    
    uint64 _materialReticle = 0;

    iaVector3d _force;
    iaVector3d _torque;

    void onHandle();
    void onApplyForceAndTorque(iPhysicsBody* body, float32 timestep);

};

#endif