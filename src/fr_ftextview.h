#ifndef FR_FTEXTVIEW
#define FR_FTEXTVIEW

#include "fr_def.h"
#include "fr_fstringitem.h"

class FTextView : public BTextView
{
public:
	FTextView(BView* parentView, BRect br,const char* name,BRect innerRect,int resMode,int flags);
	
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	
	int32 linkoffset, linklen;
	FStringItem* fi;
	
private:
	BView*	parent;
};


#endif
