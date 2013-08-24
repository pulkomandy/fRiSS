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
	myd = new BDragger(f, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
	AddChild(myd);		
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
	else {
		BString text;
		
		text << _T("The mouse button you just pressed is not yet supported in friss.") << "\n\n";
		text << _T("Please send a mail to beos@herzig-net.de and notify me about the following mouse button code: ") << m_buttons <<"\n\n";
		text << _T("Thank you");
		(new BAlert(_T("Unexpected Event"), text.String(), _T("Ok")))->Go();
	}
}

void
FListView::Draw(BRect frame)
{
#if ALLOW_TRANSP
	if (!transparent) {
		SetDrawingMode(B_OP_COPY);
		BListView::Draw(frame);
	}
	else {
		FrissView* f = (FrissView*) fv;
		if (f->bitmap) {
			BRect t(Bounds());
			ConvertToParent(&t);
			DrawBitmap(f->bitmap, t, Bounds());
		}
		SetViewColor(B_TRANSPARENT_COLOR);
		BListView::Draw(frame);
	}
#else
	BListView::Draw(frame);
#endif
}

void
FListView::TranspSetUp(bool transp)
{
	transparent = transp;
}
