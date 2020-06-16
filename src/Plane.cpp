#include "Plane.h"

#include <igor/system/iWindow.h>
#include <igor/system/iTimer.h>
#include <igor/system/iApplication.h>
#include <igor/scene/nodes/iNodeManager.h>
#include <igor/scene/nodes/iNodeTransform.h>
#include <igor/scene/nodes/iNodePhysics.h>
#include <igor/scene/nodes/iNodeModel.h>
#include <igor/scene/nodes/iNodeEmitter.h>
#include <igor/scene/nodes/iNodeCamera.h>
#include <igor/scene/nodes/iNodeLODTrigger.h>
#include <igor/scene/iScene.h>
#include <igor/graphics/iRenderer.h>
#include <igor/graphics/iView.h>
#include <igor/resources/model/iModel.h>
#include <igor/resources/material/iMaterialResourceFactory.h>
#include <igor/physics/iPhysics.h>
#include <igor/terrain/iVoxelTerrain.h>
using namespace igor;

#include <iaux/system/iaConsole.h>
#include <iaux/data/iaString.h>
using namespace iaux;

static const float64 s_maxThrust = 100000;

Plane::Plane(iScene* scene, iView* view, const iaMatrixd& matrix)
{
	_transformNode = iNodeManager::getInstance().createNode<iNodeTransform>();
	_transformNode->setName("planeTransform");
	_transformNode->setMatrix(matrix);
	scene->getRoot()->insertNode(_transformNode);

	_planeModel = iNodeManager::getInstance().createNode<iNodeModel>();
	_planeModel->registerModelReadyDelegate(iModelReadyDelegate(this, &Plane::onModelReady));
	_planeModel->setModel("plane.ompf");
	_transformNode->insertNode(_planeModel);

	iNodeTransform* transformCam = iNodeManager::getInstance().createNode<iNodeTransform>();
	transformCam->translate(0, 2, 10);
	_transformNode->insertNode(transformCam);

	iNodeCamera* camera = iNodeManager::getInstance().createNode<iNodeCamera>();
	view->setCurrentCamera(camera->getID());
	transformCam->insertNode(camera);

	iNodeLODTrigger* lodTrigger = iNodeManager::getInstance().createNode<iNodeLODTrigger>();
	_lodTriggerID = lodTrigger->getID();
	camera->insertNode(lodTrigger);

	iaMatrixd offset;
	_physicsNode = iNodeManager::getInstance().createNode<iNodePhysics>();
	_physicsNode->addSphere(1, offset);
	_physicsNode->finalizeCollision();
	_physicsNode->setMass(500); // 500 kg
	_physicsNode->setForceAndTorqueDelegate(iApplyForceAndTorqueDelegate(this, &Plane::onApplyForceAndTorque));
	_physicsNode->setAngularDamping(iaVector3d(0.4, 0.4, 0.4));
	_physicsNode->setLinearDamping(0.5);
	_transformNode->insertNode(_physicsNode);

	_materialReticle = iMaterialResourceFactory::getInstance().createMaterial();
	iMaterialResourceFactory::getInstance().getMaterial(_materialReticle)->setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
	iMaterialResourceFactory::getInstance().getMaterial(_materialReticle)->setRenderState(iRenderState::Blend, iRenderStateValue::On);

	iApplication::getInstance().registerApplicationPreDrawHandleDelegate(iPreDrawDelegate(this, &Plane::onHandle));
}

Plane::~Plane()
{
	iApplication::getInstance().unregisterApplicationPreDrawHandleDelegate(iPreDrawDelegate(this, &Plane::onHandle));

	iNodeManager::getInstance().destroyNodeAsync(_transformNode);
}

void Plane::onModelReady(uint64 modelNodeID)
{
	if (_planeModel->getID() == modelNodeID)
	{
		_planeModel->unregisterModelReadyDelegate(iModelReadyDelegate(this, &Plane::onModelReady));

		_leftAileron = static_cast<iNodeTransform*>(_planeModel->getChild("plane")->getChild("body")->getChild("left_aileron")->getChild("rotate"));
		_rightAileron = static_cast<iNodeTransform*>(_planeModel->getChild("plane")->getChild("body")->getChild("right_aileron")->getChild("rotate"));
		_leftElevator = static_cast<iNodeTransform*>(_planeModel->getChild("plane")->getChild("body")->getChild("left_elevator")->getChild("rotate"));
		_rightElevator = static_cast<iNodeTransform*>(_planeModel->getChild("plane")->getChild("body")->getChild("right_elevator")->getChild("rotate"));
		_rudder = static_cast<iNodeTransform*>(_planeModel->getChild("plane")->getChild("body")->getChild("rudder")->getChild("rotate"));
		_propeller = static_cast<iNodeTransform*>(_planeModel->getChild("plane")->getChild("body")->getChild("propeller"));
	}
}

float64 Plane::getAltitude() const
{
	iaMatrixd matrix;
	_transformNode->getMatrix(matrix);

	return matrix._pos._y;
}

void Plane::onApplyForceAndTorque(iPhysicsBody* body, float32 timestep)
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

void Plane::setPosition(const iaVector3d& pos)
{
	uint64 bodyID = _physicsNode->getBodyID();

	iPhysicsBody* body = iPhysics::getInstance().getBody(bodyID);
	if (body != nullptr)
	{
		iaMatrixd matrix;
		matrix._pos = pos;
		body->setMatrix(matrix);
	}
}

uint64 Plane::getLODTriggerID() const
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

float64 Plane::getVelocity() const
{
	iaMatrixd inverse = _physicsNode->getWorldMatrix();
	inverse.invert();
	iaVector3d vec = inverse * _physicsNode->getVelocity();
	return -vec._z;
}

void Plane::onHandle()
{	
	float64 velocity = getVelocity();
	con_endl(velocity);

	float32 thrust = s_maxThrust * _thrustLevel;
	const float32 offsetIncrease = 0.1;
	iaMatrixd matrix;
	iaVector3d resultingForce;
	iaVector3d resultingTorque;

	_transformNode->getMatrix(matrix);

	if (true)//  _forward)
	{
		iaVector3d foreward = matrix._depth;
		foreward.negate();
		foreward.normalize();
		foreward *= thrust;
		resultingForce += foreward;

		iaVector3d worldUp(0, 1, 0);
		iaVector3d up = matrix._top;
		up.normalize();

		up *= thrust * 0.4 * (worldUp * up);
		resultingForce += up;
	}

	if (_propeller)
	{
		_propeller->rotate(_thrustLevel * 5.0, iaAxis::Z);
	}

	if (_rollLeft)
	{
		resultingTorque._z = 200.0;

		if (_leftAileron)
		{
			iaMatrixd matrix;
			matrix.rotate(0.3, iaAxis::X);
			_leftAileron->setMatrix(matrix);
		}
		if (_rightAileron)
		{
			iaMatrixd matrix;
			matrix.rotate(-0.3, iaAxis::X);
			_rightAileron->setMatrix(matrix);
		}
	}
	else if (_rollRight)
	{
		resultingTorque._z = -200.0;

		if (_leftAileron)
		{
			iaMatrixd matrix;
			matrix.rotate(-0.3, iaAxis::X);
			_leftAileron->setMatrix(matrix);
		}
		if (_rightAileron)
		{
			iaMatrixd matrix;
			matrix.rotate(0.3, iaAxis::X);
			_rightAileron->setMatrix(matrix);
		}
	}
	else
	{
		resultingTorque._z = 0.0;

		
		if (_leftAileron)
		{
			iaMatrixd matrix;
			_leftAileron->setMatrix(matrix);
		}
		if (_rightAileron)
		{
			iaMatrixd matrix;
			_rightAileron->setMatrix(matrix);
		}
	}

	if (_rollUp)
	{
		resultingTorque._x = 200.0;

		iaMatrixd matrix;
		matrix.rotate(0.3, iaAxis::X);
		if (_leftElevator)
		{
			_leftElevator->setMatrix(matrix);
		}
		if (_rightElevator)
		{
			_rightElevator->setMatrix(matrix);
		}
	}
	else if (_rollDown)
	{
		resultingTorque._x = -200.0;

		iaMatrixd matrix;
		matrix.rotate(-0.3, iaAxis::X);
		if (_leftElevator)
		{
			_leftElevator->setMatrix(matrix);
		}
		if (_rightElevator)
		{
			_rightElevator->setMatrix(matrix);
		}
	}
	else
	{
		resultingTorque._x = 0.0;

		iaMatrixd matrix;
		if (_leftElevator)
		{
			_leftElevator->setMatrix(matrix);
		}
		if (_rightElevator)
		{
			_rightElevator->setMatrix(matrix);
		}
	}

	matrix._pos.set(0, 0, 0);
	_torque = matrix * resultingTorque;

 	_force = resultingForce;
}

void Plane::setThrustLevel(float64 thrustLevel)
{
	_thrustLevel = std::min(1.0, std::max(0.0, thrustLevel));
}

float64 Plane::getThrustLevel() const
{
	return _thrustLevel;
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