#ifndef FR_FLISTVIEW
#define FR_FLISTVIEW

#include "fr_def.h"

class FListView : public BListView
{
public:
	FListView(const char *name,
		list_view_type type = B_SINGLE_SELECTION_LIST,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	FListView(BMessage* archive);

	void SelectionChanged();
	void MouseDown(BPoint point);

	int	compare(const void* a, const void* b);

	status_t Archive(BMessage* archive, bool deep = true) const;
	static BArchivable* Instantiate(BMessage* archive);

	// Transparency:
	bool		transparent;

private:
	unsigned long		m_buttons;
};

#endif
