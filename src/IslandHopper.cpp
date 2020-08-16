#include "IslandHopper.h"

#include "game/GameLayer.h"
#include "ui/HUDLayer.h"

IslandHopper::IslandHopper()
{
    _window = iApplication::getInstance().createWindow();
    _window->setSize(1280, 720);
    _window->setCentered();
    _window->setTitle("Island Hopper");
    _window->setDoubleClick(true);
    _window->open();

    HUDLayer *hudLayer = new HUDLayer(_window, 20);
    GameLayer *gameLayer = new GameLayer(_window, 10);
    gameLayer->setHud(hudLayer);
    iApplication::getInstance().addLayer(gameLayer);
    iApplication::getInstance().addLayer(hudLayer);
    igor::iApplication::getInstance().addLayer(new iLayerProfiler(_window, "Profiler", 100, iProfilerVerbosity::None));

    _taskFlushTextures = iTaskManager::getInstance().addTask(new iTaskFlushTextures(_window));
    _taskFlushModels = iTaskManager::getInstance().addTask(new iTaskFlushModels(_window));
}

IslandHopper::~IslandHopper()
{
    iApplication::getInstance().clearLayerStack();
    iApplication::getInstance().destroyWindow(_window);
}

void IslandHopper::run()
{
    iApplication::getInstance().run();

    iTaskManager::getInstance().abortTask(_taskFlushModels);
    iTaskManager::getInstance().abortTask(_taskFlushTextures);
}