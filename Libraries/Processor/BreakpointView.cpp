#include "BreakpointView.h"
#include "resource.h"

//----------------------------------------------------------------------------------------
//Constructors
//----------------------------------------------------------------------------------------
Processor::BreakpointView::BreakpointView(Processor* adevice)
:device(adevice), initializedDialog(false), currentControlFocus(0)
{
	breakpointListIndex = -1;
	std::wstring windowTitle = BuildWindowTitle(device->GetModuleDisplayName(), device->GetDeviceClassName(), device->GetDeviceInstanceName(), L"Breakpoints");
	SetDialogTemplateSettings(windowTitle, (HINSTANCE)device->GetAssemblyHandle(), MAKEINTRESOURCE(IDD_PROCESSOR_BREAK));
}

//----------------------------------------------------------------------------------------
//Member window procedure
//----------------------------------------------------------------------------------------
INT_PTR Processor::BreakpointView::WndProcDialog(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	WndProcDialogImplementSaveFieldWhenLostFocus(hwnd, msg, wparam, lparam);
	switch(msg)
	{
	case WM_INITDIALOG:
		return msgWM_INITDIALOG(hwnd, wparam, lparam);
	case WM_CLOSE:
		return msgWM_CLOSE(hwnd, wparam, lparam);
	case WM_TIMER:
		return msgWM_TIMER(hwnd, wparam, lparam);
	case WM_COMMAND:
		return msgWM_COMMAND(hwnd, wparam, lparam);
	}
	return FALSE;
}

//----------------------------------------------------------------------------------------
//Event handlers
//----------------------------------------------------------------------------------------
INT_PTR Processor::BreakpointView::msgWM_INITDIALOG(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	boost::mutex::scoped_lock lock(device->debugMutex);
	for(BreakpointList::iterator i = device->breakpoints.begin(); i != device->breakpoints.end(); ++i)
	{
		int listIndex = (int)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_ADDSTRING, 0, (LPARAM)(*i)->GetName().c_str());
		SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETITEMDATA, listIndex, (LPARAM)*i);
	}

	SetTimer(hwnd, 1, 100, NULL);
	breakpoint.Initialize();
	UpdateBreakpoint(hwnd, breakpoint, device->GetAddressBusCharWidth());
	breakpointListIndex = -1;

	return TRUE;
}

//----------------------------------------------------------------------------------------
INT_PTR Processor::BreakpointView::msgWM_CLOSE(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	KillTimer(hwnd, 1);
	DestroyWindow(hwnd);

	return TRUE;
}

//----------------------------------------------------------------------------------------
INT_PTR Processor::BreakpointView::msgWM_TIMER(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	initializedDialog = true;
	if(breakpointListIndex >= 0)
	{
		Breakpoint* targetBreakpoint = (Breakpoint*)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETITEMDATA, breakpointListIndex, 0);
		UpdateDlgItemBin(hwnd, IDC_PROCESSOR_BREAK_HITCOUNT, targetBreakpoint->GetHitCounter());
	}

	return TRUE;
}

//----------------------------------------------------------------------------------------
INT_PTR Processor::BreakpointView::msgWM_COMMAND(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if(HIWORD(wparam) == EN_SETFOCUS)
	{
		previousText = GetDlgItemString(hwnd, LOWORD(wparam));
		currentControlFocus = LOWORD(wparam);
	}
	else if(HIWORD(wparam) == EN_KILLFOCUS)
	{
		std::wstring newText = GetDlgItemString(hwnd, LOWORD(wparam));
		if(newText != previousText)
		{
			boost::mutex::scoped_lock lock(device->debugMutex);

			switch(LOWORD(wparam))
			{
			case IDC_PROCESSOR_BREAK_NAME:
				breakpoint.SetName(GetDlgItemString(hwnd, LOWORD(wparam)));
				break;
			case IDC_PROCESSOR_BREAK_BREAKCOUNTER:
				breakpoint.SetBreakCounter(GetDlgItemBin(hwnd, LOWORD(wparam)));
				break;
			case IDC_PROCESSOR_BREAK_LOCCONDDATA1:{
				bool updateName = false;
				if((breakpoint.GetName().length() == 0) || (breakpoint.GetName() == breakpoint.GenerateName(device->GetAddressBusCharWidth())))
				{
					updateName = true;
				}
				breakpoint.SetLocationConditionData1(GetDlgItemHex(hwnd, LOWORD(wparam)), device->GetAddressBusWidth());
				UpdateDlgItemHex(hwnd, LOWORD(wparam), device->GetAddressBusCharWidth(), breakpoint.GetLocationConditionData1());
				if(updateName)
				{
					std::wstring newName = breakpoint.GenerateName(device->GetAddressBusCharWidth());
					breakpoint.SetName(newName);
					UpdateDlgItemString(hwnd, IDC_PROCESSOR_BREAK_NAME, newName);
				}
				break;}
			case IDC_PROCESSOR_BREAK_LOCCONDDATA2:
				breakpoint.SetLocationConditionData2(GetDlgItemHex(hwnd, LOWORD(wparam)), device->GetAddressBusWidth());
				UpdateDlgItemHex(hwnd, LOWORD(wparam), device->GetAddressBusCharWidth(), breakpoint.GetLocationConditionData2());
				break;
			}
		}
	}
	else if(HIWORD(wparam) == BN_CLICKED)
	{
		boost::mutex::scoped_lock lock(device->debugMutex);

		switch(LOWORD(wparam))
		{
		case IDC_PROCESSOR_BREAK_LOG:
			breakpoint.SetLogEvent(IsDlgButtonChecked(hwnd, LOWORD(wparam)) == BST_CHECKED);
			break;
		case IDC_PROCESSOR_BREAK_BREAK:
			breakpoint.SetBreakEvent(IsDlgButtonChecked(hwnd, LOWORD(wparam)) == BST_CHECKED);
			break;
		case IDC_PROCESSOR_BREAK_BREAKONCOUNTER:
			breakpoint.SetBreakOnCounter(IsDlgButtonChecked(hwnd, LOWORD(wparam)) == BST_CHECKED);
			break;
		case IDC_PROCESSOR_BREAK_HITCOUNT_RESET:
			if(breakpointListIndex != -1)
			{
				Breakpoint* targetBreakpoint = (Breakpoint*)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETITEMDATA, breakpointListIndex, 0);
				targetBreakpoint->SetHitCounter(0);
			}
			break;

		case IDC_PROCESSOR_BREAK_LOCCONDNOT:
			breakpoint.SetLocationConditionNot(IsDlgButtonChecked(hwnd, LOWORD(wparam)) == BST_CHECKED);
			break;
		case IDC_PROCESSOR_BREAK_LOCCOND1:
			breakpoint.SetLocationCondition(Breakpoint::CONDITION_EQUAL);
			EnableWindow(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2), FALSE);
			break;
		case IDC_PROCESSOR_BREAK_LOCCOND2:
			breakpoint.SetLocationCondition(Breakpoint::CONDITION_GREATER);
			EnableWindow(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2), FALSE);
			break;
		case IDC_PROCESSOR_BREAK_LOCCOND3:
			breakpoint.SetLocationCondition(Breakpoint::CONDITION_LESS);
			EnableWindow(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2), FALSE);
			break;
		case IDC_PROCESSOR_BREAK_LOCCOND4:
			breakpoint.SetLocationCondition(Breakpoint::CONDITION_GREATER_AND_LESS);
			EnableWindow(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2), TRUE);
			break;

		case IDC_PROCESSOR_BREAK_NEW:
			breakpoint.Initialize();
			UpdateBreakpoint(hwnd, breakpoint, device->GetAddressBusCharWidth());
			SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETCURSEL, (WPARAM)-1, NULL);
			breakpointListIndex = -1;
			break;
		case IDC_PROCESSOR_BREAK_SAVE:
			if(breakpoint.GetName().length() == 0)
			{
				std::wstring newName = breakpoint.GenerateName(device->GetAddressBusCharWidth());
				breakpoint.SetName(newName);
				UpdateDlgItemString(hwnd, IDC_PROCESSOR_BREAK_NAME, newName);
			}
			if(breakpointListIndex != -1)
			{
				Breakpoint* targetBreakpoint = (Breakpoint*)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETITEMDATA, breakpointListIndex, 0);
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_DELETESTRING, breakpointListIndex, NULL);
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_INSERTSTRING, breakpointListIndex, (LPARAM)breakpoint.GetName().c_str());
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETITEMDATA, breakpointListIndex, (LPARAM)targetBreakpoint);
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETCURSEL, breakpointListIndex, NULL);
				breakpoint.SetHitCounter(targetBreakpoint->GetHitCounter());
				*targetBreakpoint = breakpoint;
			}
			else
			{
				Breakpoint* targetBreakpoint = new Breakpoint(breakpoint);
				device->breakpoints.push_back(targetBreakpoint);
				device->breakpointExists = !device->breakpoints.empty();

				breakpointListIndex = (int)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_ADDSTRING, 0, (LPARAM)breakpoint.GetName().c_str());
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETITEMDATA, breakpointListIndex, (LPARAM)targetBreakpoint);
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETCURSEL, breakpointListIndex, NULL);
			}
			break;
		case IDC_PROCESSOR_BREAK_DELETE:
			if(breakpointListIndex != -1)
			{
				Breakpoint* targetBreakpoint = (Breakpoint*)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETITEMDATA, breakpointListIndex, 0);
				bool done = false;
				BreakpointList::iterator i = device->breakpoints.begin();
				while(!done && (i != device->breakpoints.end()))
				{
					if(*i == targetBreakpoint)
					{
						delete *i;
						device->breakpoints.erase(i);
						done = true;
					}
					else 
					{
						++i;
					}
				}
				device->breakpointExists = !device->breakpoints.empty();
				SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_DELETESTRING, breakpointListIndex, NULL);
				breakpointListIndex -= 1;
			}
			SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_SETCURSEL, breakpointListIndex, NULL);

			//Load the data for the new breakpoint
			if(breakpointListIndex != -1)
			{
				Breakpoint* targetBreakpoint = (Breakpoint*)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETITEMDATA, breakpointListIndex, 0);
				breakpoint = *targetBreakpoint;
			}
			else
			{
				breakpoint.Initialize();
			}
			UpdateBreakpoint(hwnd, breakpoint, device->GetAddressBusCharWidth());
			break;
		case IDC_PROCESSOR_BREAK_DELETEALL:
			SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_RESETCONTENT, 0, NULL);
			for(BreakpointList::iterator i = device->breakpoints.begin(); i != device->breakpoints.end(); ++i)
			{
				delete *i;
			}
			device->breakpoints.clear();
			device->breakpointExists = false;

			//Clear the current breakpoint info
			breakpoint.Initialize();
			UpdateBreakpoint(hwnd, breakpoint, device->GetAddressBusCharWidth());
			breakpointListIndex = -1;
			break;
		}
	}
	else if(HIWORD(wparam) == LBN_SELCHANGE)
	{
		boost::mutex::scoped_lock lock(device->debugMutex);

		switch(LOWORD(wparam))
		{
		case IDC_PROCESSOR_BREAK_LIST:
			int selectedItem = (int)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETCURSEL, 0, NULL);
			if(selectedItem != LB_ERR)
			{
				breakpointListIndex = selectedItem;
				Breakpoint* targetBreakpoint = (Breakpoint*)SendMessage(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LIST), LB_GETITEMDATA, breakpointListIndex, 0);
				breakpoint = *targetBreakpoint;
				UpdateBreakpoint(hwnd, breakpoint, device->GetAddressBusCharWidth());
			}
			break;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------------------
//Breakpoint functions
//----------------------------------------------------------------------------------------
void Processor::BreakpointView::UpdateBreakpoint(HWND hwnd, const Breakpoint& breakpoint, unsigned int addressCharWidth)
{
	UpdateDlgItemString(hwnd, IDC_PROCESSOR_BREAK_NAME, breakpoint.GetName());
	CheckDlgButton(hwnd, IDC_PROCESSOR_BREAK_LOG, (breakpoint.GetLogEvent())? BST_CHECKED: BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_PROCESSOR_BREAK_BREAK, (breakpoint.GetBreakEvent())? BST_CHECKED: BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_PROCESSOR_BREAK_BREAKONCOUNTER, (breakpoint.GetBreakOnCounter())? BST_CHECKED: BST_UNCHECKED);
	UpdateDlgItemBin(hwnd, IDC_PROCESSOR_BREAK_BREAKCOUNTER, breakpoint.GetBreakCounter());
	UpdateDlgItemBin(hwnd, IDC_PROCESSOR_BREAK_HITCOUNT, breakpoint.GetHitCounter());

	UpdateDlgItemHex(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA1, addressCharWidth, breakpoint.GetLocationConditionData1());
	UpdateDlgItemHex(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2, addressCharWidth, breakpoint.GetLocationConditionData2());
	CheckDlgButton(hwnd, IDC_PROCESSOR_BREAK_LOCCONDNOT, (breakpoint.GetLocationConditionNot())? BST_CHECKED: BST_UNCHECKED);
	switch(breakpoint.GetLocationCondition())
	{
	case Breakpoint::CONDITION_EQUAL:
		CheckRadioButton(hwnd, IDC_PROCESSOR_BREAK_LOCCOND1, IDC_PROCESSOR_BREAK_LOCCOND4, IDC_PROCESSOR_BREAK_LOCCOND1);
		break;
	case Breakpoint::CONDITION_GREATER:
		CheckRadioButton(hwnd, IDC_PROCESSOR_BREAK_LOCCOND1, IDC_PROCESSOR_BREAK_LOCCOND4, IDC_PROCESSOR_BREAK_LOCCOND2);
		break;
	case Breakpoint::CONDITION_LESS:
		CheckRadioButton(hwnd, IDC_PROCESSOR_BREAK_LOCCOND1, IDC_PROCESSOR_BREAK_LOCCOND4, IDC_PROCESSOR_BREAK_LOCCOND3);
		break;
	case Breakpoint::CONDITION_GREATER_AND_LESS:
		CheckRadioButton(hwnd, IDC_PROCESSOR_BREAK_LOCCOND1, IDC_PROCESSOR_BREAK_LOCCOND4, IDC_PROCESSOR_BREAK_LOCCOND4);
		break;
	}
	if(breakpoint.GetLocationCondition() == Breakpoint::CONDITION_GREATER_AND_LESS)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwnd, IDC_PROCESSOR_BREAK_LOCCONDDATA2), FALSE);
	}
}
