#ifndef __PLANE_H__
#define __PLANE_H__

#include <igor/igor.h>
using namespace igor;
using namespace iaux;

iaEVENT(PlaneCrashedEvent, PlaneCrashedDelegate, (), ());
iaEVENT(PlaneLandedEvent, PlaneLandedDelegate, (), ());

class Plane
{

public:
    Plane(iScene *scene, iView *view);
    virtual ~Plane();

    void startRollLeft();
    void stopRollLeft();
    void startRollRight();
    void stopRollRight();

    void startRollUp();
    void stopRollUp();
    void startRollDown();
    void stopRollDown();

    void setThrustLevel(float64 thrustLevel);
    float64 getThrustLevel() const;

    float64 getVelocity() const;

    iaVector3d getPosition() const;
    uint64 getLODTriggerID() const;

    void setMatrix(const iaMatrixd &matrix);

    PlaneCrashedEvent _planeCrashedEvent;
    PlaneLandedEvent _planeLandedEvent;

private:
    iaVector3d _velocity;
    float64 _rollTorque = 0.0;
    float64 _pitchTorque = 0.0;

    float64 _thrustLevel = 0.0;
    bool _rollLeft = false;
    bool _rollRight = false;
    bool _pitchUp = false;
    bool _pitchDown = false;

    iNodeTransform *_camOrientationNode = nullptr;
    iNodeTransform *_transformNode = nullptr;
    iNodeModel *_planeModel = nullptr;

    iPhysicsCollision *_collisionCast = nullptr;

    iNodeTransform *_leftAileron = nullptr;
    iNodeTransform *_rightAileron = nullptr;
    iNodeTransform *_propeller = nullptr;
    iNodeTransform *_leftElevator = nullptr;
    iNodeTransform *_rightElevator = nullptr;
    iNodeTransform *_rudder = nullptr;

    uint64 _lodTriggerID = iNode::INVALID_NODE_ID;

    uint64 _materialReticle = 0;

    iTimerHandle _timerHandle;

    unsigned onRayPreFilter(iPhysicsBody *body, iPhysicsCollision *collision, const void *userData);
    void onModelReady(uint64 modelNodeID);
    void onTick();
};

#endif // __PLANE_H__