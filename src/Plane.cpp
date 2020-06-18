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

Plane::Plane(iScene* scene, iView* view, const iaMatrixd& matrix)
{
	_transformNode = iNodeManager::getInstance().createNode<iNodeTransform>();
	_transformNode->setName("plane_somename");
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

	_timerHandle.setIntervall(10);
	_timerHandle.registerTimerDelegate(iTimerTickDelegate(this, &Plane::onTick));
}

Plane::~Plane()
{
	_timerHandle.unregisterTimerDelegate(iTimerTickDelegate(this, &Plane::onTick));

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

const iaVector3d& Plane::getVelocity() const
{
	return _velocity;
}

void Plane::onTick()
{	
	static const float64 rollTorque = 0.00005;
	static const float64 pitchTorque = 0.000025;
	static const float64 maxThrust = 0.02;
	static const float64 planeWeight = 0.01;

	iaMatrixd matrix;
	_transformNode->getMatrix(matrix);

	iaVector3d thrust = matrix._depth;
	thrust.negate();
	thrust.normalize();
	thrust *= _thrustLevel * maxThrust;

	iaVector3d up(0, 1, 0);
	iaVector3d lift = matrix._top;
	lift *=  (lift * up) * (_thrustLevel * maxThrust);

	iaVector3d weight(0, -planeWeight, 0);

	_velocity *= 0.99;

	iaVector3d netForce = thrust + lift + weight;
	netForce *= 0.001;
	_velocity += netForce;
	matrix._pos += _velocity;

	// con_endl(thrust << "+" << lift << "+" << weight << "=" << netForce);

	/*if (true)//  _forward)
	{
		
		foreward.negate();
		foreward.normalize();
		foreward *= thrust;
		resultingForce += foreward;

		iaVector3d worldUp(0, 1, 0);
		iaVector3d up = matrix._top;
		up.normalize();

		up *= thrust * 0.4 * (worldUp * up);
		resultingForce += up;
	}*/

	if (_propeller)
	{
		_propeller->rotate(_thrustLevel * 5.0, iaAxis::Z);
	}

	iaMatrixd roll;

	if (_rollLeft)
	{
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

		roll.rotate(rollTorque,iaAxis::Z);
	}
	else if (_rollRight)
	{
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
		roll.rotate(-rollTorque, iaAxis::Z);
	}
	else
	{
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

	iaMatrixd pitch;

	if (_pitchUp)
	{
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

		pitch.rotate(pitchTorque, iaAxis::X);
	}
	else if (_pitchDown)
	{
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

		pitch.rotate(-pitchTorque, iaAxis::X);
	}
	else
	{
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

	matrix *= pitch;
	matrix *= roll;
	_transformNode->setMatrix(matrix);
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
	_pitchUp = true;
}

void Plane::stopRollUp()
{
	_pitchUp = false;
}

void Plane::startRollDown()
{
	_pitchDown = true;
}

void Plane::stopRollDown()
{
	_pitchDown = false;
}