#ifndef __PLANE__
#define __PLANE__

#include <igor/scene/nodes/iNode.h>
#include <igor/system/iTimerHandle.h>
#include <igor/physics/iPhysicsCollision.h>
#include <igor/physics/iPhysicsBody.h>
using namespace igor;

#include <iaux/math/iaMatrix.h>
#include <iaux/system/iaEvent.h>
using namespace iaux;

namespace igor
{
	class iScene;
	class iNodeModel;
	class iView;
	class iWindow;
	class iNodeTransform;
} // namespace igor

iaEVENT(PlaneCrashedEvent, PlaneCrashedDelegate, void, (), ());
iaEVENT(PlaneLandedEvent, PlaneLandedDelegate, void, (), ());

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

	float64 getAltitude() const;
	uint64 getLODTriggerID() const;

	void setMatrix(const iaMatrixd &matrix);

	PlaneCrashedEvent _planeCrashedEvent;
	PlaneLandedEvent _planeLandedEvent;

private:
	iaVector3d _velocity;
	float64 _rollTorque = 0.0;
	float64 _pitchTorque = 0.0;

	float64 _thrustLevel = 0.5;
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

#endif