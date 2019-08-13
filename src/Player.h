#ifndef __PLAYER__
#define __PLAYER__

#include "Entity.h"

#include <iNode.h>
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

class Player : public Entity
{

public:

    Player(iScene* scene, iView* view, const iaMatrixd& matrix);
    virtual ~Player();

    void startUp();
    void stopUp();
    void startDown();
    void stopDown();

    void startForward();
    void stopForward();
    void startBackward();
    void stopBackward();

    void startLeft();
    void stopLeft();
    void startRight();
    void stopRight();

    void startRollLeft();
    void stopRollLeft();
    void startRollRight();
    void stopRollRight();

    void startFastTurn();
    void stopFastTurn();

	void startFastTravel();
	void stopFastTravel();

    void rotate(float32 heading, float32 pitch);

    void drawReticle(const iWindow& window);

    uint32 getLODTriggerID();

    void setPosition(iaVector3d pos);

private:

    bool _up = false;
    bool _forward = false;
    bool _backward = false;
    bool _down = false;
    bool _left = false;
    bool _right = false;
    bool _rollLeft = false;
    bool _rollRight = false;
    bool _fastTurn = false;
	bool _fastTravel = false;

	uint64 _physicsNodeID = iNode::INVALID_NODE_ID;
	uint64 _transformNodeID = iNode::INVALID_NODE_ID;

    uint64 _lodTriggerID = iNode::INVALID_NODE_ID;
    
    uint64 _materialReticle = 0;

    iScene* _scene = nullptr;

    iaVector3d _force;
    iaVector3d _torque;

    void handle();
    iaVector3d updatePos();
    void hitBy(uint64 entityID);

    void onApplyForceAndTorque(iPhysicsBody* body, float32 timestep);

};

#endif