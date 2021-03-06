#ifndef __DOCKINGWINDOW_H__
#define __DOCKINGWINDOW_H__
#include "IDockingWindow.h"
#include <list>
#include <vector>
#include <map>

//##TODO## Consider eliminating the use of the common controls tab control for the content
// window tabs, and drawing the tabs ourselves. This will allow us to stack the tabs on
// multiple rows rather than have them go into an overflow area, but without changing the
// order of the tabs when they're selected like they do on the common tab control. It would
// also allow us to clip the text of the window title for each tab if there's not enough
// space to hold the full title of each one, and collapse the size of each tab down
// progressively until we reach a minimum size, like is done in tab-based web browsers. The
// behaviour of Internet Explorer is particularly nice, with scroll buttons appearing at
// each edge of the tab area. I would make the currently selected tab display at its full
// width if possible though, with tabs being shifted to the right to accommodate it, but
// the tab list never being scrolled to the left automatically. We could also add a close
// button on each tab itself if desired, like in web browsers.
//##FIX## We need a way to handle infinite nesting of docking windows within docking
// windows, where the placement targets can directly overlap. Perhaps some kind of ancestor
// navigator windows that appear in drop target mode? I'm not sure what the best way to
// handle this problem is.
class DockingWindow :public IDockingWindow
{
public:
	// Enumerations
	enum class WindowMessages :unsigned int;
	enum class ContentWindowInfo :unsigned int;
	enum class DockedWindowInfo :unsigned int;

	// Structures
	struct AddContentWindowParams;
	struct ModifyContentWindowParams;
	struct AddDockedWindowParams;
	struct GetContentWindowInfo;
	struct GetDockedWindowInfo;

	// Constants
	static const wchar_t* WindowClassName;

public:
	// Constructors
	DockingWindow(HINSTANCE moduleHandle, HWND hwnd);
	virtual ~DockingWindow();

	// Class registration
	static bool RegisterWindowClass(HINSTANCE moduleHandle);
	static bool UnregisterWindowClass(HINSTANCE moduleHandle);

	// Message handlers
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// Structures
	struct ChildContainerEntry;
	struct ContentEntry;
	struct AutoHideDockTab;
	struct AutoHideDockTabGroup;
	struct AutoHideDockInfo;
	struct TabRenderInfo;
	struct DockingWindowDropTargetInfo;

	// Constants
	static const wchar_t* TabTrayWindowClassName;
	static const wchar_t* PlacementTargetWindowClassName;
	static const wchar_t* PlacementShadowWindowClassName;

private:
	// Message handlers
	LRESULT WndProcPrivate(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_CREATE(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_DESTROY(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_SIZING(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_SIZE(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_MOVING(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_TIMER(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_ENTERSIZEMOVE(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_EXITSIZEMOVE(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_WINDOWPOSCHANGING(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_NCHITTEST(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_NCLBUTTONUP(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_SETCURSOR(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_GETFONT(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_SETFONT(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_PAINT(WPARAM wParam, LPARAM lParam);
	LRESULT msgWM_BOUNCE(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_ADDCONTENTWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_REMOVECONTENTWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_MODIFYCONTENTWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_ADDDOCKEDWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_REMOVEDOCKEDWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETCONTENTWINDOWINDEXFROMHANDLE(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETDOCKEDWINDOWINDEXFROMHANDLE(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETCONTENTWINDOWCOUNT(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETCONTENTWINDOWINFO(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETDOCKEDWINDOWCOUNT(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETDOCKEDWINDOWINFO(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_GETACTIVECONTENTWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_SETACTIVECONTENTWINDOW(WPARAM wParam, LPARAM lParam);
	LRESULT msgDOCKWIN_PERFORMTABHITTEST(WPARAM wParam, LPARAM lParam);

	// Tab tray message handlers
	static LRESULT CALLBACK TabTrayWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT TabTrayWndProcPrivate(HWND tabTrayHwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT msgTabTrayWM_SIZE(HWND tabTrayHwnd, WPARAM wParam, LPARAM lParam);
	LRESULT msgTabTrayWM_TIMER(HWND tabTrayHwnd, WPARAM wParam, LPARAM lParam);
	LRESULT msgTabTrayWM_MOUSEMOVE(HWND tabTrayHwnd, WPARAM wParam, LPARAM lParam);
	LRESULT msgTabTrayWM_PAINT(HWND tabTrayHwnd, WPARAM wParam, LPARAM lParam);
	LRESULT msgTabTrayWM_PRINTCLIENT(HWND tabTrayHwnd, WPARAM wParam, LPARAM lParam);

	// Placement target message handlers
	static LRESULT CALLBACK PlacementTargetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT PlacementTargetWndProcPrivate(HWND placementTargetHwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT msgPlacementTargetWM_PAINT(HWND placementTargetHwnd, WPARAM wParam, LPARAM lParam);
	LRESULT msgPlacementTargetWM_PRINTCLIENT(HWND placementTargetHwnd, WPARAM wParam, LPARAM lParam);

	// Placement shadow message handlers
	static LRESULT CALLBACK PlacementShadowWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT PlacementShadowWndProcPrivate(HWND placementShadowHwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT msgPlacementShadowWM_PAINT(HWND placementShadowHwnd, WPARAM wParam, LPARAM lParam);

	// Helper methods
	static IDockingWindow* GetDockingWindowFromHWND(HWND hwnd);
	static BOOL CALLBACK EnumDockingWindowsProc(HWND hwnd, LPARAM lParam);
	static void AddDockingWindowToWindowList(HWND hwnd, std::list<IDockingWindow*>& dockingWindowList);
	static HCURSOR LoadCursorFile(const unsigned char* cursorData, size_t cursorDataSize);

	// Window handle methods
	virtual HWND GetWindowHandle() const;

	// Placement content methods
	virtual void HideDropTargets(IDockingWindow* callingDockingWindow);
	void HideDropTargets(IDockingWindow* callingDockingWindow, bool hideChildWindowDropTargets);
	void ShowDropTargets(bool allowCenterDocking);
	virtual void ShowDropTargets(IDockingWindow* callingDockingWindow, int dockWindowWidth, int dockWindowHeight, int cursorPosX, int cursorPosY);
	virtual bool HitTestDropTargets(IDockingWindow* callingDockingWindow, int cursorPosX, int cursorPosY, IDockingWindowDropTargetInfo*& dropTargetInfo) const;

	// Tab control update methods
	void HandleContentTabChanged(int newTabIndex);
	void TabHitTest(HWND tabTrayHwnd, int cursorPosX, int cursorPosY);
	void UpdateAutoHideWindowTabSizes();

	// Child container methods
	virtual void AddChildContainer(IDockingWindow* childContainer, const IDockingWindowDropTargetInfo* dropTargetInfo);
	virtual void RemoveChildContainer(IDockingWindow* childContainer);
	virtual void NotifyChildContainerContentChanged(IDockingWindow* childContainer);
	virtual bool HasNestedChildDockingWindows() const;
	virtual Marshal::Ret<std::list<IDockingWindow*>> GetNestedChildDockingWindowList() const;
	virtual bool CanResizeChildContainerWindowEdge(IDockingWindow* childContainer, WindowEdge windowEdge) const;
	virtual void UpdateDesiredChildContainerSize(IDockingWindow* childContainer, int desiredWidth, int desiredHeight);
	virtual HCURSOR ParentOverrideCursorForChildContainer(IDockingWindow* childContainer, int cursorPosX, int cursorPosY) const;
	virtual bool ParentBorderClickForChildContainer(IDockingWindow* childContainer, int cursorPosX, int cursorPosY);
	void ExpandHiddenDockPanel(IDockingWindow* childContainer, HWND targetContentWindow, int newWindowPosX, int newWindowPosY, int newWindowWidth, int newWindowHeight);
	void CollapseHiddenDockPanel();

	// Hosted content methods
	void AddHostedContent(HWND contentWindow, const std::wstring& contentTitle);
	void RemoveHostedContent(HWND contentWindow);
	virtual unsigned int GetHostedContentCount() const;
	virtual unsigned int GetSortedContentEntryNo(unsigned int sortedContentEntryIndex) const;
	virtual bool GetHostedContentIndexFromWindow(HWND contentWindow, unsigned int& contentEntryNo) const;
	unsigned int GetActiveContent() const;
	virtual void SetActiveContent(unsigned int contentEntryNo);
	virtual Marshal::Ret<std::wstring> GetHostedContentTitle(unsigned int contentEntryNo) const;
	void SetHostedContentTitle(unsigned int contentEntryNo, const std::wstring& newContentTitle);
	virtual HWND GetHostedContentWindow(unsigned int contentEntryNo) const;
	void SetHostedContentWindow(unsigned int contentEntryNo, HWND newContentWindow);
	bool AlwaysShowContentWindowTabs() const;

	// Parent docking window methods
	virtual IDockingWindow* GetParentDockingWindow() const;
	virtual void NotifyAddedToParent(IDockingWindow* newParentDockingWindow);
	virtual void NotifyRemovedFromParent();

	// Sizing methods
	void HandleSizeChanged(int newWidth, int newHeight);

private:
	// Window handles
	HINSTANCE _moduleHandle;
	HWND _hwnd;
	HWND _tabDockPanel;
	HWND _dockPanel;

	// Size info
	int _currentControlWidth;
	int _currentControlHeight;

	// Font info
	HFONT _controlFont;
	HFONT _controlFontVertical;
	int _controlFontHeight;
	int _controlFontDescent;

	// Parent docking window info
	IDockingWindow* _parentDockingWindow;

	// Child content info
	std::list<ChildContainerEntry> _childContainers;
	std::vector<ContentEntry> _hostedContent;
	std::map<int, size_t> _tabIndexToHostedContentNo;
	HWND _contentTabControl;
	int _currentSelectedTabIndex;
	int _dockingPanelTabMarginSize;
	int _dockingPanelTabGroupSeparatorSize;

	// Autohide child containers
	std::map<WindowEdge, AutoHideDockInfo> _autoHideDocks;
	std::map<WindowEdge, HCURSOR> _enableAutoHideCursor;
	std::map<WindowEdge, HCURSOR> _disableAutoHideCursor;
	bool _autoHidePanelVisible;
	IDockingWindow* _currentAutoHidePanel;
	HWND _currentAutoHidePanelContent;
	int _tabExtendedHitRegionX;
	int _tabExtendedHitRegionY;
	int _tabExtendedHitRegionWidth;
	int _tabExtendedHitRegionHeight;

	// Drag-info
	bool _windowDragInProgress;
	IDockingWindow* _dockingWindowUnderDragPos;
	RECT _windowSizeMoveInitialPos;
	int _lastDragCursorPosX;
	int _lastDragCursorPosY;
	bool _leftMouseButtonDown;
	bool _ignoreNextSizeAndMove;
	bool _tabDragActive;
	bool _tabDragIgnoreTabIndex;
	int _tabDragIndexToIgnore;
	int _dragTabIndex;
	int _tabWithDeadZoneForDrag;
	int _tabDeadZoneStartX;
	int _tabDeadZoneEndX;

	// Placement target info
	std::map<WindowEdge, HWND> _placementTargets;
	std::map<WindowEdge, HWND> _placementTargetsForceTop;
	IDockingWindow* _placementTargetsChildDockingWindow;
	bool _placementTargetsVisible;
	bool _placementTargetCenterEnabled;
	HWND _placementTargetCenter;
	bool _placementShadowVisible;
	HWND _placementShadow;
};

#include "DockingWindow.inl"
#endif
