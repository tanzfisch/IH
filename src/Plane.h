#ifndef __PLANE__
#define __PLANE__

#include <igor/scene/nodes/iNode.h>
using namespace igor;

#include <iaux/math/iaMatrix.h>
using namespace iaux;

namespace igor
{
	class iScene;
	class iPhysicsBody;
	class iNodePhysics;
	class iNodeModel;
	class iView;
	class iWindow;
	class iNodeTransform;
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

	void setThrustLevel(float64 thrustLevel);
	float64 getThrustLevel() const;

	float64 getVelocity() const;

	float64 getAltitude() const;
	uint64 getLODTriggerID() const;

	void drawReticle(const iWindow& window);

	void setPosition(const iaVector3d& pos);

private:

	float64 _thrustLevel = 0.5;
	bool _rollLeft = false;
	bool _rollRight = false;
	bool _rollUp = false;
	bool _rollDown = false;

	iNodeTransform* _transformNode = nullptr;
	iNodePhysics* _physicsNode = nullptr;
	iNodeModel* _planeModel = nullptr;

	iNodeTransform* _leftAileron = nullptr;
	iNodeTransform* _rightAileron = nullptr;
	iNodeTransform* _propeller = nullptr;
	iNodeTransform* _leftElevator = nullptr;
	iNodeTransform* _rightElevator = nullptr;
	iNodeTransform* _rudder = nullptr;

	uint64 _lodTriggerID = iNode::INVALID_NODE_ID;

	uint64 _materialReticle = 0;

	iaVector3d _force;
	iaVector3d _torque;

	void onModelReady(uint64 modelNodeID);
	void onHandle();
	void onApplyForceAndTorque(iPhysicsBody* body, float32 timestep);

};

#endif