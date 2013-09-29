/* Copyright 2010-2013, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under the terms of the MIT Licence */

#ifndef __FEEDLISTVIEW
#define __FEEDLISTVIEW

#include "xmlnode.h"

class FeedListItem: public BStringItem
{
	public:
		FeedListItem(XmlNode*);
		void DrawItem(BView* owner, BRect frame, bool complete);
		void Update(BView* owner, const BFont* font);

	private:
		static status_t GetIcon(void* cookie);

		BBitmap* fIcon;
		BView* fParent;
};

#endif
