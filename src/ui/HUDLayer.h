#ifndef __HUDLAYER_H__
#define __HUDLAYER_H__

#include <igor/igor.h>
using namespace igor;
using namespace iaux;

class HUDLayer : public iLayerWidgets
{

public:
    /*! nothing to do
	*/
    HUDLayer(iWindow *window, int32 zIndex);

    /*! deinit resources
	*/
    ~HUDLayer();

    /*! sets plane position

    \param position the planes position
    */
    void setPosition(const iaVector3d &position);

    /*! sets plane velocity

    \param velocity the planes velocity
    */
    void setVelocity(const iaVector3d &velocity);

private:
    /*! the main dialog
    */
    iDialogPtr _dialog = nullptr;

    /*! altitude label
    */
    iWidgetLabel *_labelAltitude = nullptr;

    /*! init ui
	*/
    void onInit() override;

    /*! clear resources
	*/
    void onDeinit() override;

    /*! called on application pre draw event
    */
    void onPreDraw() override;

    /*! called on any other event
    */
    void onEvent(iEvent &event) override;

    /*! called when key was pressed

    \param event the event to handle
    */
    bool onKeyDown(iEventKeyDown &event);

    /*! triggered when main dialog got closed

    \param dialog source of the event
    */
    void onCloseDialog(iDialogPtr dialog);
};

#endif // __UILAYER_H__
