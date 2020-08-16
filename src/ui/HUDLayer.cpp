#include "HUDLayer.h"

static const float64 s_waterLevel = 10000;

HUDLayer::HUDLayer(iWindow *window, int32 zIndex)
    : iLayerWidgets(new iWidgetDefaultTheme("StandardFont.png", "dialogBackground.png"), window, "Widgets", zIndex)
{
}

HUDLayer::~HUDLayer()
{
}

void HUDLayer::onInit()
{
    iLayerWidgets::onInit();

    _dialog = new iDialog();
    _dialog->setHorizontalAlignment(iHorizontalAlignment::Strech);
    _dialog->setVerticalAlignment(iVerticalAlignment::Top);
    _dialog->setHeight(50);
    // it does not matter if we open it now or after adding all the child widgets
    _dialog->open(iDialogCloseDelegate(this, &HUDLayer::onCloseDialog));

    iWidgetGrid *grid = new iWidgetGrid(_dialog);
    grid->appendRows(1);
    grid->setHorizontalAlignment(iHorizontalAlignment::Strech);
    grid->setVerticalAlignment(iVerticalAlignment::Strech);
    grid->setBorder(5);
    grid->setCellSpacing(5);
    grid->setStrechRow(1);
    grid->setStrechColumn(0);
    grid->setSelectMode(iSelectionMode::NoSelection);

    _labelAltitude = new iWidgetLabel();
    _labelAltitude->setText("altitude");
    grid->addWidget(_labelAltitude, 0, 0);
}

void HUDLayer::setPosition(const iaVector3d &position)
{
    std::wstringstream stream;
    stream << std::setfill(L' ') << std::fixed << std::right << std::setprecision(1) << std::setw(6) << (position._y - s_waterLevel) << "m";
    _labelAltitude->setText(stream.str().c_str());
}

void HUDLayer::setVelocity(const iaVector3d &velocity)
{
}

void HUDLayer::onCloseDialog(iDialogPtr dialog)
{
    if (_dialog != dialog)
    {
        return;
    }

    delete _dialog;
    _dialog = nullptr;
}

void HUDLayer::onDeinit()
{
    iLayerWidgets::onDeinit();
}

void HUDLayer::onPreDraw()
{
    iLayerWidgets::onPreDraw();
}

void HUDLayer::onEvent(iEvent &event)
{
    iLayerWidgets::onEvent(event);

    /*    event.dispatch<iEventKeyDown>(IGOR_BIND_EVENT_FUNCTION(HUDLayer::onKeyDown));
    event.dispatch<iEventNodeAddedToScene>(IGOR_BIND_EVENT_FUNCTION(HUDLayer::onNodeAddedToScene));
    event.dispatch<iEventNodeRemovedFromScene>(IGOR_BIND_EVENT_FUNCTION(HUDLayer::onNodeRemovedFromScene));
    event.dispatch<iEventSceneSelectionChanged>(IGOR_BIND_EVENT_FUNCTION(HUDLayer::onSceneSelectionChanged));*/
}

bool HUDLayer::onKeyDown(iEventKeyDown &event)
{
    /*    switch (event.getKey())
    {
    case iKeyCode::N:
        return true;
    }*/

    return false;
}

/*void HUDLayer::onRenderOrtho()
{
    iaMatrixd matrix;
    iRenderer::getInstance().setViewMatrix(matrix);
    matrix.translate(0, 0, -30);
    iRenderer::getInstance().setModelMatrix(matrix);
    iRenderer::getInstance().setMaterial(_materialWithTextureAndBlending);
    iRenderer::getInstance().setFont(_font);

    if (_loading)
    {
        iRenderer::getInstance().setColor(iaColor4f(0, 0, 0, 1));
        iRenderer::getInstance().drawRectangle(0, 0, _window.getClientWidth(), _window.getClientHeight());

        iRenderer::getInstance().setColor(iaColor4f(0, 0, 1, 1));
        iRenderer::getInstance().setFontSize(40.0f);
        iRenderer::getInstance().drawString(_window.getClientWidth() * 0.5, _window.getClientHeight() * 0.5, "generating level ...", iHorizontalAlignment::Center, iVerticalAlignment::Center);
    }
    else
    {
        iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
        iRenderer::getInstance().setFontSize(15.0f);
        iaString altitude = "Altitude: ";
        altitude += iaString::toString(_plane->getAltitude() - s_waterLevel);
        iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.01, altitude);

        iaString thrustLevel = "Thrust: ";
        thrustLevel += iaString::toString(_plane->getThrustLevel() * 100.0);
        iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.04, thrustLevel);

        iaString velocity = "Velocity: ";
        velocity += iaString::toString(_plane->getVelocity());
        iRenderer::getInstance().drawString(_window.getClientWidth() * 0.01, _window.getClientHeight() * 0.07, velocity);
    }

    _profiler.draw(&_window, _font, iaColor4f(1.0, 1.0, 1.0, 1));

    iRenderer::getInstance().setColor(iaColor4f(1, 1, 1, 1));
}*/