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
	transformCam->translate(0,2,10);
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
    physicsNode->setAngularDamping(iaVector3d(1000, 1000, 1000));
    physicsNode->setLinearDamping(10);
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
    float32 speed = 2000;

	if (_fastTravel)
	{
		speed = 25000;
	}

    const float32 offsetIncrease = 0.1;
    iaMatrixd matrix;
    iaVector3d resultingForce;

    iNodeTransform* transformationNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
    transformationNode->getMatrix(matrix);

    if (_forward)
    {
        iaVector3d foreward = matrix._depth;
        foreward.negate();
        foreward.normalize();
        foreward *= speed;
        resultingForce += foreward;
    }

    if (_backward)
    {
        iaVector3d backward = matrix._depth;
        backward.normalize();
        backward *= speed;
        resultingForce += backward;
    }

    if (_left)
    {
        iaVector3d left = matrix._right;
        left.negate();
        left.normalize();
        left *= speed;
        resultingForce += left;
    }

    if (_right)
    {
        iaVector3d right = matrix._right;
        right.normalize();
        right *= speed;
        resultingForce += right;
    }

    if (_up)
    {
        iaVector3d up = matrix._top;
        up.normalize();
        up *= speed;
        resultingForce += up;
    }

    if (_down)
    {
        iaVector3d down = matrix._top;
        down.negate();
        down.normalize();
        down *= speed;
        resultingForce += down;
    }

    if (_rollLeft)
    {
        _torque._z = 10.0;
    }
    else if (_rollRight)
    {
        _torque._z = -10.0;
    }
    else
    {
        _torque._z = 0.0;
    }

    _force = resultingForce;
}

void Plane::rotate(float32 heading, float32 pitch)
{
    iNodeTransform* transformationNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
    iaMatrixd matrix;

    transformationNode->getMatrix(matrix);
    matrix._pos.set(0, 0, 0);

    if (_fastTurn)
    {
        _torque.set(pitch * 700.0, heading * 700.0, _torque._z);
    }
    else
    {
        _torque.set(pitch * 400.0, heading * 400.0, _torque._z);
    }
    _torque = matrix * _torque;
}

void Plane::startFastTurn()
{
    _fastTurn = true;
}

void Plane::stopFastTurn()
{
    _fastTurn = false;
}

void Plane::startFastTravel()
{
	_fastTravel = true;
}

void Plane::stopFastTravel()
{
	_fastTravel = false;
}

void Plane::startForward()
{
    _forward = true;
}

void Plane::stopForward()
{
    _forward = false;
}

void Plane::startBackward()
{
    _backward = true;
}

void Plane::stopBackward()
{
    _backward = false;
}

void Plane::startUp()
{
    _up = true;
}

void Plane::stopUp()
{
    _up = false;
}

void Plane::startDown()
{
    _down = true;
}

void Plane::stopDown()
{
    _down = false;
}

void Plane::startLeft()
{
    _left = true;
}

void Plane::stopLeft()
{
    _left = false;
}

void Plane::startRight()
{
    _right = true;
}

void Plane::stopRight()
{
    _right = false;
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

void Plane::onApplyForceAndTorque(iPhysicsBody* body, float32 timestep)
{
    body->setForce(_force);
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
