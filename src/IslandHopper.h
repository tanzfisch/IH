#ifndef __ISLANDHOPPER_H__
#define __ISLANDHOPPER_H__

#include <igor/igor.h>
using namespace igor;
using namespace iaux;

class IslandHopper
{

public:
    IslandHopper();
    virtual ~IslandHopper();

    void run();

private:
    /*! main window of mica
	*/
    iWindowPtr _window = nullptr;

    /*! id of textures flush task
	*/
    iTaskID _taskFlushTextures = iTask::INVALID_TASK_ID;

    /*! id of models flush task
	*/
    iTaskID _taskFlushModels = iTask::INVALID_TASK_ID;
};

#endif // __ISLANDHOPPER_H__
