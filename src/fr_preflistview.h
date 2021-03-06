#include "fr_def.h"

class PrefListView : public BOutlineListView
{
public:
	PrefListView(XmlNode* listroot, const char *name,
		list_view_type type = B_SINGLE_SELECTION_LIST,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);

	virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
	
	virtual void SelectionChanged();
	
	virtual void MessageReceived(BMessage* msg);
	
	void BuildView(XmlNode *node, int level=0, BListItem* parent = NULL);
	
	virtual void MouseDown(BPoint point);
	
	void Sort(BListItem* sort);

private:
	XmlNode*	root;
};
