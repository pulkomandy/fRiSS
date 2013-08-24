#include "fr_flistview.h"
#include "fr_view.h"

FListView::FListView(BView* thefv, BRect f, const char *name,
	list_view_type type, uint32 resizingMode, uint32 flags) :
	BListView(f, name, type, resizingMode, flags)
{
	fv = thefv;
	transparent = false;	

	f.left = f.right - 8;
	f.top = f.bottom - 8;
}


void
FListView::SelectionChanged()
{
	BListView::SelectionChanged();
	
	if (m_buttons == 0x1) {
		int32 idx = CurrentSelection();
	
		if (idx!=B_ERROR) {
			FStringItem *fi = (FStringItem*) ItemAt( idx );
			if (fi) {
				((FrissView*)fv)->ItemSelected( fi );
			}
		}
	}

}


void
FListView::MouseDown(BPoint point)
{
	BPoint cursor;
	GetMouse(&cursor,&m_buttons);

	if ((m_buttons & 0x1) == 0x1)
		BListView::MouseDown(point);
	else if ((m_buttons & 0x2) == 0x2) {
		// right mouse button is for popup only :-)
		BPoint m_point = point;
		ConvertToScreen(&m_point);
		
		((FrissView*)fv)->StartPopup( m_point );
	}
	else if ((m_buttons & 0x4) == 0x4) {
		((FrissView*)fv)->LoadNext();
	}
}
