#include "fr_flistview.h"
#include "fr_view.h"

FListView::FListView(const char *name,
	list_view_type type, uint32 flags) :
	BListView(name, type, flags)
{
	transparent = false;	
}


FListView::FListView(BMessage* archive)
	: BListView(archive)
{
	transparent = false;
}


void
FListView::SelectionChanged()
{
	BListView::SelectionChanged();
	FrissView* fv = (FrissView*)Parent()->Parent();
	
	if (m_buttons == 0x1) {
		int32 idx = CurrentSelection();
	
		if (idx!=B_ERROR) {
			FStringItem *fi = (FStringItem*) ItemAt( idx );
			if (fi) {
				fv->ItemSelected( fi );
			}
		}
	}

}


void
FListView::MouseDown(BPoint point)
{
	BPoint cursor;
	GetMouse(&cursor,&m_buttons);
	FrissView* fv = (FrissView*)Parent()->Parent();

	if ((m_buttons & 0x1) == 0x1)
		BListView::MouseDown(point);
	else if ((m_buttons & 0x2) == 0x2) {
		// right mouse button is for popup only :-)
		BPoint m_point = point;
		ConvertToScreen(&m_point);
		
		fv->StartPopup( m_point );
	}
	else if ((m_buttons & 0x4) == 0x4) {
		fv->LoadNext();
	}
}


status_t
FListView::Archive(BMessage* archive, bool deep) const
{
	status_t result = BListView::Archive(archive, deep);
	archive->AddString("add_on", app_signature);
	return result;
}


/*static*/ BArchivable*
FListView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "FListView"))
		return NULL;
	return new FListView(archive);
}
