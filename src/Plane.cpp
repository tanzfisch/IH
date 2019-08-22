#include "Plane.h"

#include <iNodeFactory.h>
#include <iNodeTransform.h>
#include <iNodePhysics.h>
#include <iNodeModel.h>
#include <iModel.h>
#include <iScene.h>
#include <iPhysics.h>
#include <iPhysicsBody.h>
#include <iPhysicsJoint.h>
#include <iPhysicsCollision.h>
#include <iNodeCamera.h>
#include <iNodeLODTrigger.h>
#include <iRenderer.h>
#include <iView.h>
#include <iWindow.h>
#include <iMaterialResourceFactory.h>
#include <iNodeEmitter.h>
#include <iTimer.h>
#include <iVoxelTerrain.h>
#include <iApplication.h>
using namespace Igor;

#include <iaConsole.h>
#include <iaString.h>
using namespace IgorAux;

Plane::Plane(iScene* scene, iView* view, const iaMatrixd& matrix)
{
	iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	transformNode->setMatrix(matrix);
	_transformNodeID = transformNode->getID();
	scene->getRoot()->insertNode(transformNode);

	iNodeModel* planeModel = static_cast<iNodeModel*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeModel));
	planeModel->setModel("plane.ompf");
	transformNode->insertNode(planeModel);

	iNodeTransform* transformCam = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
	transformCam->translate(0, 2, 10);
	transformNode->insertNode(transformCam);

	iNodeCamera* camera = static_cast<iNodeCamera*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeCamera));
	view->setCurrentCamera(camera->getID());
	transformCam->insertNode(camera);

	iNodeLODTrigger* lodTrigger = static_cast<iNodeLODTrigger*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeLODTrigger));
	_lodTriggerID = lodTrigger->getID();
	camera->insertNode(lodTrigger);

	iaMatrixd offset;
	iNodePhysics* physicsNode = static_cast<iNodePhysics*>(iNodeFactory::getInstance().createNode(iNodeType::iNodePhysics));
	_physicsNodeID = physicsNode->getID();
	physicsNode->addSphere(1, offset);
	physicsNode->finalizeCollision();
	physicsNode->setMass(500); // 500 kg
	physicsNode->setForceAndTorqueDelegate(iApplyForceAndTorqueDelegate(this, &Plane::onApplyForceAndTorque));
	physicsNode->setAngularDamping(iaVector3d(1, 1, 1));
	physicsNode->setLinearDamping(1);
	transformNode->insertNode(physicsNode);

	_materialReticle = iMaterialResourceFactory::getInstance().createMaterial();
	iMaterialResourceFactory::getInstance().getMaterial(_materialReticle)->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_materialReticle)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);

	iApplication::getInstance().registerApplicationPreDrawHandleDelegate(iApplicationPreDrawHandleDelegate(this, &Plane::onHandle));
}

Plane::~Plane()
{
	iNodeFactory::getInstance().destroyNodeAsync(_transformNodeID);
}

void Plane::onApplyForceAndTorque(iPhysicsBody * body, float32 timestep)
{
	float64 Ixx;
	float64 Iyy;
	float64 Izz;
	float64 mass;

	iPhysics::getInstance().getMassMatrix(static_cast<void*>(body->getNewtonBody()), mass, Ixx, Iyy, Izz);
	iaVector3d graviation(0.0f, -mass * static_cast<float32>(__IGOR_GRAVITY__), 0.0f);

	body->setForce(_force + graviation);
	body->setTorque(_torque);
}

void Plane::setPosition(iaVector3d pos)
{
	iNodePhysics* physicsNode = static_cast<iNodePhysics*>(iNodeFactory::getInstance().getNode(_physicsNodeID));
	if (physicsNode != nullptr)
	{
		uint64 bodyID = physicsNode->getBodyID();

		iPhysicsBody* body = iPhysics::getInstance().getBody(bodyID);
		if (body != nullptr)
		{
			iaMatrixd matrix;
			matrix._pos = pos;
			body->setMatrix(matrix);
		}
	}
}

uint32 Plane::getLODTriggerID()
{
	return _lodTriggerID;
}

void Plane::drawReticle(const iWindow& window)
{
	iaVector3f weaponPos(window.getClientWidth() * 0.5, window.getClientHeight() * 0.5, 0);

	float32 scale = 0.001 * window.getClientWidth();

	iRenderer::getInstance().setMaterial(_materialReticle);
	iRenderer::getInstance().setLineWidth(1 * scale);

	iRenderer::getInstance().setColor(iaColor4f(1, 0, 0, 1));
	iRenderer::getInstance().drawLine(weaponPos + iaVector3f(-10 * scale, 0, 0), weaponPos + iaVector3f(10 * scale, 0, 0));
	iRenderer::getInstance().drawLine(weaponPos + iaVector3f(0, -10 * scale, 0), weaponPos + iaVector3f(0, 10 * scale, 0));
}

void Plane::onHandle()
{
	float32 speed = 40000;

	if (_fastTravel)
	{
		speed *= 2.0;
	}

	const float32 offsetIncrease = 0.1;
	iaMatrixd matrix;
	iaVector3d resultingForce;
	iaVector3d resultingTorque;

	iNodeTransform* transformationNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
	transformationNode->getMatrix(matrix);

	if (true)//  _forward)
	{
		iaVector3d foreward = matrix._depth;
		foreward.negate();
		foreward.normalize();
		foreward *= speed;
		resultingForce += foreward;

		iaVector3d worldUp(0,1,0);
		iaVector3d up = matrix._top;
		up.normalize();

		up *= speed * 0.4 * (worldUp * up);
		resultingForce += up;
	}

	if (_rollLeft)
	{
		resultingTorque._z = 200.0;
	}
	else if (_rollRight)
	{
		resultingTorque._z = -200.0;
	}
	else
	{
		resultingTorque._z = 0.0;
	}

	if (_rollUp)
	{
		resultingTorque._x = 200.0;
	}
	else if (_rollDown)
	{
		resultingTorque._x = -200.0;
	}
	else
	{
		resultingTorque._x = 0.0;
	}

	matrix._pos.set(0,0,0);
	_torque = matrix * resultingTorque;

	_force = resultingForce;
}

void Plane::startFastTravel()
{
	_fastTravel = true;
}

void Plane::stopFastTravel()
{
	_fastTravel = false;
}

void Plane::startRollLeft()
{
	_rollLeft = true;
}

void Plane::stopRollLeft()
{
	_rollLeft = false;
}

void Plane::startRollRight()
{
	_rollRight = true;
}

void Plane::stopRollRight()
{
	_rollRight = false;
}

void Plane::startRollUp()
{
	_rollUp = true;
}

void Plane::stopRollUp()
{
	_rollUp = false;
}

void Plane::startRollDown()
{
	_rollDown = true;
}

void Plane::stopRollDown()
{
	_rollDown = false;
}