#ifndef __PLANE__
#define __PLANE__

#include <igor/scene/nodes/iNode.h>
#include <igor/system/iTimerHandle.h>
using namespace igor;

#include <iaux/math/iaMatrix.h>
using namespace iaux;

namespace igor
{
	class iScene;
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

	const iaVector3d& getVelocity() const;

	float64 getAltitude() const;
	uint64 getLODTriggerID() const;

	void drawReticle(const iWindow& window);

private:

	iaVector3d _velocity;
	float64 _thrustLevel = 0.5;
	bool _rollLeft = false;
	bool _rollRight = false;
	bool _pitchUp = false;
	bool _pitchDown = false;

	iNodeTransform* _transformNode = nullptr;
	iNodeModel* _planeModel = nullptr;

	iNodeTransform* _leftAileron = nullptr;
	iNodeTransform* _rightAileron = nullptr;
	iNodeTransform* _propeller = nullptr;
	iNodeTransform* _leftElevator = nullptr;
	iNodeTransform* _rightElevator = nullptr;
	iNodeTransform* _rudder = nullptr;

	uint64 _lodTriggerID = iNode::INVALID_NODE_ID;

	uint64 _materialReticle = 0;

	iTimerHandle _timerHandle;

	void onModelReady(uint64 modelNodeID);
	void onTick();

};

#endif