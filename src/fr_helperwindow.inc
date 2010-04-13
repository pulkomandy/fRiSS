// Included from fr_view.cpp

class MyDummyWindow : public BWindow
{
public:
	MyDummyWindow(FrissView* p) : 
		BWindow(BRect(-50,-50,-10,-10), "buddy of friss", B_TITLED_WINDOW, B_FRAME_EVENTS)
	{	
		parent = p;
		SetWorkspaces(B_ALL_WORKSPACES);
	}
	
	bool QuitRequested()
	{
		return(TRUE);
	}
	
	void ScreenChanged(BRect frame, color_space mode)
	{
		parent->Window()->PostMessage(MSG_WORKSPACE, parent);

		BWindow::ScreenChanged(frame, mode);	
	}
	
	void WorkspacesChanged(uint32 oldWs, uint32 newWs)
	{
		parent->Window()->PostMessage(MSG_WORKSPACE, parent);

		BWindow::WorkspacesChanged(oldWs, newWs);	
	}
		
	void WorkspaceActivated(int32 ws, bool state)
	{
		if (state && parent) {
			parent->Window()->PostMessage(MSG_WORKSPACE, parent);
		}
		BWindow::WorkspaceActivated(ws,state);
	}
	
private:
	FrissView* parent;
};

