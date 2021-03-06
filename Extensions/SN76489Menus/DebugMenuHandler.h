#ifndef __DEBUGMENUHANDLER_H__
#define __DEBUGMENUHANDLER_H__
#include "DeviceInterface/DeviceInterface.pkg"
#include "SN76489Menus.h"
#include "SN76489/ISN76489.h"

class DebugMenuHandler :public MenuHandlerBase
{
public:
	// Constructors
	DebugMenuHandler(SN76489Menus& owner, const IDevice& modelInstanceKey, ISN76489& model);

protected:
	// Management functions
	virtual void GetMenuItems(std::list<MenuItemDefinition>& menuItems) const;
	virtual IViewPresenter* CreateViewForItem(int menuItemID, const std::wstring& viewName);
	virtual void DeleteViewForItem(int menuItemID, IViewPresenter* viewPresenter);

private:
	SN76489Menus& _owner;
	const IDevice& _modelInstanceKey;
	ISN76489& _model;
};

#endif
