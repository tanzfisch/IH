#include "Plane.h"

Plane::Plane(iScene *scene, iView *view)
{
    _transformNode = iNodeManager::getInstance().createNode<iNodeTransform>();
    _transformNode->setName("plane transform");
    scene->getRoot()->insertNode(_transformNode);

    _planeModel = iNodeManager::getInstance().createNode<iNodeModel>();
    _planeModel->registerModelReadyDelegate(iModelReadyDelegate(this, &Plane::onModelReady));
    _planeModel->setModel("plane.ompf");
    _transformNode->insertNode(_planeModel);

    _camOrientationNode = iNodeManager::getInstance().createNode<iNodeTransform>();
    _camOrientationNode->setName("cam orientation");
    _transformNode->insertNode(_camOrientationNode);

    iNodeTransform *transformCam = iNodeManager::getInstance().createNode<iNodeTransform>();
    transformCam->translate(0, 2.5, 10);
    _camOrientationNode->insertNode(transformCam);

    iNodeCamera *camera = iNodeManager::getInstance().createNode<iNodeCamera>();
    view->setCurrentCamera(camera->getID());
    transformCam->insertNode(camera);

    iNodeLODTrigger *lodTrigger = iNodeManager::getInstance().createNode<iNodeLODTrigger>();
    _lodTriggerID = lodTrigger->getID();
    camera->insertNode(lodTrigger);

    _timerHandle.setIntervall(10);
    _timerHandle.registerTimerDelegate(iTimerTickDelegate(this, &Plane::onTick));

    _collisionCast = iPhysics::getInstance().createSphere(2.0, iaMatrixd());
}

Plane::~Plane()
{
    _timerHandle.unregisterTimerDelegate(iTimerTickDelegate(this, &Plane::onTick));
    iNodeManager::getInstance().destroyNodeAsync(_transformNode);
    iPhysics::getInstance().destroyCollision(_collisionCast);
}

void Plane::setMatrix(const iaMatrixd &matrix)
{
    _transformNode->setMatrix(matrix);
}

void Plane::onModelReady(uint64 modelNodeID)
{
    if (_planeModel->getID() == modelNodeID)
    {
        _planeModel->unregisterModelReadyDelegate(iModelReadyDelegate(this, &Plane::onModelReady));

        _leftAileron = static_cast<iNodeTransform *>(_planeModel->getChild("plane")->getChild("body")->getChild("left_aileron")->getChild("rotate"));
        _rightAileron = static_cast<iNodeTransform *>(_planeModel->getChild("plane")->getChild("body")->getChild("right_aileron")->getChild("rotate"));
        _leftElevator = static_cast<iNodeTransform *>(_planeModel->getChild("plane")->getChild("body")->getChild("left_elevator")->getChild("rotate"));
        _rightElevator = static_cast<iNodeTransform *>(_planeModel->getChild("plane")->getChild("body")->getChild("right_elevator")->getChild("rotate"));
        _rudder = static_cast<iNodeTransform *>(_planeModel->getChild("plane")->getChild("body")->getChild("rudder")->getChild("rotate"));
        _propeller = static_cast<iNodeTransform *>(_planeModel->getChild("plane")->getChild("body")->getChild("propeller"));
    }
}

iaVector3d Plane::getPosition() const
{
    iaMatrixd matrix;
    _transformNode->getMatrix(matrix);

    return matrix._pos;
}

uint64 Plane::getLODTriggerID() const
{
    return _lodTriggerID;
}

float64 Plane::getVelocity() const
{
    return _velocity.length();
}

void Plane::onTick()
{
    static const float64 fps = 100.0;
    static const float64 maxRollSpeed = M_PI * 0.0005 / fps;
    static const float64 maxPitchSpeed = M_PI * 0.0005 / fps;
    static const float64 maxVelocity = 0.1 / fps;

    // appliy drag
    _rollTorque *= 0.99995;
    _pitchTorque *= 0.99995;

    iaMatrixd matrix;
    _transformNode->getMatrix(matrix);

    _velocity = matrix._depth;
    _velocity.negate();
    _velocity.normalize();
    _velocity *= _thrustLevel * maxVelocity;

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

        if (fabs(_rollTorque) < maxRollSpeed)
        {
            _rollTorque += 0.000000001;
        }
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

        if (fabs(_rollTorque) < maxRollSpeed)
        {
            _rollTorque -= 0.000000001;
        }
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

        if (fabs(_pitchTorque) < maxPitchSpeed)
        {
            _pitchTorque += 0.000000001;
        }
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

        if (fabs(_pitchTorque) < maxPitchSpeed)
        {
            _pitchTorque -= 0.000000001;
        }
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

    std::vector<ConvexCastReturnInfo> info;
    iaVector3d target = matrix._pos + _velocity;
    iPhysics::getInstance().convexCast(matrix, target, _collisionCast, iRayPreFilterDelegate(this, &Plane::onRayPreFilter), nullptr, info);

    if (!info.empty())
    {
        _planeCrashedEvent();
        return;
    }

    roll.rotate(_rollTorque, iaAxis::Z);
    pitch.rotate(_pitchTorque, iaAxis::X);

    matrix *= pitch;
    matrix *= roll;
    matrix._pos = target;
    _transformNode->setMatrix(matrix);

    _camOrientationNode->identity();
    _camOrientationNode->rotate(-_pitchTorque * 10000, iaAxis::X);
    _camOrientationNode->rotate(-_rollTorque * 10000, iaAxis::Z);
}

unsigned Plane::onRayPreFilter(iPhysicsBody *body, iPhysicsCollision *collision, const void *userData)
{
    // our plane has no body
    return 1;
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