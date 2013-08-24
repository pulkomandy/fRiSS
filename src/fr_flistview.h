#ifndef FR_FLISTVIEW
#define FR_FLISTVIEW

#include "fr_def.h"

class FListView : public BListView
{
public:
	FListView(BView* thefv, BRect frame, const char *name,
		list_view_type type = B_SINGLE_SELECTION_LIST,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	
	virtual void SelectionChanged();
	virtual void MouseDown(BPoint point);
	
	// Transparency:
	virtual void Draw(BRect frame);
	void TranspSetUp(bool transp);
	bool		transparent;

private:
	BView*		fv;
	unsigned long		m_buttons;

	// Only for window mode:
	BDragger* 		myd;
};

#endif
