/* Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence */

#pragma once

#include "xmlnode.h"

class FeedListItem: public BStringItem
{
	public:
		FeedListItem(XmlNode*);
		void DrawItem(BView* owner, BRect frame, bool complete);

	private:
		BBitmap* fIcon;
};
