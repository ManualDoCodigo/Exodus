#include "RegistersViewModel.h"
#include "RegistersView.h"
namespace M68000{

//----------------------------------------------------------------------------------------
//Constructors
//----------------------------------------------------------------------------------------
M68000::RegistersViewModel::RegistersViewModel(const std::wstring& amenuHandlerName, int aviewModelID, M68000* adevice)
:ViewModelBase(amenuHandlerName, aviewModelID, false, true, adevice->GetDeviceInstanceName(), adevice->GetDeviceModuleID()), device(adevice)
{}

//----------------------------------------------------------------------------------------
//View creation and deletion
//----------------------------------------------------------------------------------------
IView* M68000::RegistersViewModel::CreateView()
{
	return new RegistersView(device);
}

//----------------------------------------------------------------------------------------
void M68000::RegistersViewModel::DeleteView(IView* aview)
{
	delete aview;
}

} //Close namespace M68000