#include "Player.h"

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
using namespace Igor;

#include <iaConsole.h>
#include <iaString.h>
using namespace IgorAux;

#include "EntityManager.h"

Player::Player(iScene* scene, iView* view, const iaMatrixd& matrix)
    : Entity(Fraction::Blue, EntityType::Vehicle)
{
    _scene = scene;

    setHealth(200.0);
    setShield(300.0);
    setDamage(1.0);
    setShieldDamage(1.0);

    iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().createNode(iNodeType::iNodeTransform));
    transformNode->setMatrix(matrix);
    _transformNodeID = transformNode->getID();
	_scene->getRoot()->insertNode(transformNode);

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
    physicsNode->setMass(10);
    physicsNode->setMaterial(EntityManager::getInstance().getEntityMaterialID());
    physicsNode->setForceAndTorqueDelegate(iApplyForceAndTorqueDelegate(this, &Player::onApplyForceAndTorque));
    physicsNode->setUserData(&_id);
    physicsNode->setAngularDamping(iaVector3d(100000, 100000, 100000));
    physicsNode->setLinearDamping(500);
	transformNode->insertNode(physicsNode);

    _materialReticle = iMaterialResourceFactory::getInstance().createMaterial();
    iMaterialResourceFactory::getInstance().getMaterial(_materialReticle)->getRenderStateSet().setRenderState(iRenderState::DepthTest, iRenderStateValue::Off);
    iMaterialResourceFactory::getInstance().getMaterial(_materialReticle)->getRenderStateSet().setRenderState(iRenderState::Blend, iRenderStateValue::On);
}

Player::~Player()
{
    iNodeFactory::getInstance().destroyNodeAsync(_transformNodeID);

    con_endl("player dead");
}

void Player::hitBy(uint64 entityID)
{
    Entity* entity = EntityManager::getInstance().getEntity(entityID);
    if (entity != nullptr &&
        entity->getFraction() != getFraction())
    {
        float32 shield = getShield();
        float32 health = getHealth();

        shield -= entity->getShieldDamage();

        if (shield <= 0)
        {
            shield = 0;

            health -= entity->getDamage();
            if (health <= 0)
            {
                health = 0;
            }
        }

        setShield(shield);
        setHealth(health);
    }
}

uint32 Player::getLODTriggerID()
{
    return _lodTriggerID;
}

iaVector3d Player::updatePos()
{
    iaVector3d result;

    iNodeTransform* transformNode = static_cast<iNodeTransform*>(iNodeFactory::getInstance().getNode(_transformNodeID));
    if (transformNode != nullptr)
    {
        iaMatrixd matrix;
        transformNode->getMatrix(matrix);
        result = matrix._pos;
    }

    return result;
}

void Player::drawReticle(const iWindow& window)
{
    iaVector3f weaponPos(window.getClientWidth() * 0.5, window.getClientHeight() * 0.5, 0);

    float32 scale = 0.001 * window.getClientWidth();

    iRenderer::getInstance().setMaterial(_materialReticle);
    iRenderer::getInstance().setLineWidth(1 * scale);

    iRenderer::getInstance().setColor(iaColor4f(1, 0, 0, 1));
    iRenderer::getInstance().drawLine(weaponPos + iaVector3f(-10 * scale, 0, 0), weaponPos + iaVector3f(10 * scale, 0, 0));
    iRenderer::getInstance().drawLine(weaponPos + iaVector3f(0, -10 * scale, 0), weaponPos + iaVector3f(0, 10 * scale, 0));
}

void Player::handle()
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

void Player::rotate(float32 heading, float32 pitch)
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

void Player::startFastTurn()
{
    _fastTurn = true;
}

void Player::stopFastTurn()
{
    _fastTurn = false;
}

void Player::startFastTravel()
{
	_fastTravel = true;
}

void Player::stopFastTravel()
{
	_fastTravel = false;
}

void Player::startForward()
{
    _forward = true;
}

void Player::stopForward()
{
    _forward = false;
}

void Player::startBackward()
{
    _backward = true;
}

void Player::stopBackward()
{
    _backward = false;
}

void Player::startUp()
{
    _up = true;
}

void Player::stopUp()
{
    _up = false;
}

void Player::startDown()
{
    _down = true;
}

void Player::stopDown()
{
    _down = false;
}

void Player::startLeft()
{
    _left = true;
}

void Player::stopLeft()
{
    _left = false;
}

void Player::startRight()
{
    _right = true;
}

void Player::stopRight()
{
    _right = false;
}

void Player::startRollLeft()
{
    _rollLeft = true;
}

void Player::stopRollLeft()
{
    _rollLeft = false;
}

void Player::startRollRight()
{
    _rollRight = true;
}

void Player::stopRollRight()
{
    _rollRight = false;
}

void Player::onApplyForceAndTorque(iPhysicsBody* body, float32 timestep)
{
    body->setForce(_force);
    body->setTorque(_torque);
}

void Player::setPosition(iaVector3d pos)
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
