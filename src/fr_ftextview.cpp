#include "fr_ftextview.h"
#include "fr_view.h"

FTextView::FTextView(BView* parentView, BRect br,const char* name,BRect innerRect,int resMode,int flags) :
	BTextView(br,name,innerRect,resMode,flags)
{
	parent = parentView;
	linkoffset = 999999;
}

void
FTextView::MouseDown(BPoint point)
{
	BPoint	cursor;
	ulong	m_buttons;
	
	GetMouse(&cursor,&m_buttons);

	if (m_buttons & 0x2){
		// right mouse button is for popup only :-)
		BPoint m_point = point;
		ConvertToScreen(&m_point);
		
		((FrissView*)parent)->StartPopup( m_point );
	}
	else if (m_buttons & 0x4) {
		((FrissView*)parent)->LoadNext();
	}		
	else {
		int32 offset = OffsetAt(point);
		
		if (offset >= linkoffset) {
			((FrissView*)parent)->NodeLaunch(fi);
			//Insert(offset,"X", 1);
		}
		else {
			BTextView::MouseDown(point);
		}
	}
}


void
FTextView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
/*
	uint32 offset = OffsetAt(point);
	
	if (offset>= linkoffset && offset < linkoffset+linklen) {
		be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT, true);
	}
	else*/
		BTextView::MouseMoved(point, transit, message);
}
