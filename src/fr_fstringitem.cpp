/* FRiSS RSS feed reader
 * Copyright 2011, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT licence
 */

#include "fr_fstringitem.h"


FStringItem::FStringItem() :
	BStringItem("<empty>", 0, true)
{
	visited = false;
	bAddDesc = false;
}

FStringItem::FStringItem(const char *text, const char *url, uint32 level, bool expanded,bool addDescFlag) :
	BStringItem(text, level, expanded),
	sUrl(url)
{
	visited = false;
	bAddDesc = addDescFlag;
}

FStringItem::FStringItem(FStringItem& fi) : 
	BStringItem( fi.Text(), 0, true ),
	sUrl(fi.Url()),
	sDesc(fi.Desc())
{
	visited = fi.visited;
	bAddDesc = fi.bAddDesc;
}

const char*
FStringItem::Title() const
{
	return Text();
}

const char*
FStringItem::Url() const
{
	return sUrl.String();
}

const char*
FStringItem::Desc() const
{
	return sDesc.String();
}

const char*
FStringItem::Date() const
{
	return sDate.String();
}

bool
FStringItem::Visited() const
{
	return visited;
}

void
FStringItem::SetUrl(const char* url)
{
	sUrl = url;
}

void
FStringItem::SetDesc(const char* desc)
{
	sDesc = desc;
}

void
FStringItem::SetDate(const char* date)
{
	sDate = date;
}

void
FStringItem::SetVisited(bool vis)
{
	visited = vis;
}

bool
FStringItem::IsAddDesc() const
{
	return bAddDesc;
}

void
FStringItem::SetAddDesc(bool add)
{
	bAddDesc = add;
}


void
FStringItem::SetTitleHtml(BString title)
{
	// TODO proper HTML thing, when we get a BHTMLView.
	// For now, parse out the html stuff
	int startMarkup, endMarkup;
	do
	{
		startMarkup = title.FindFirst("<");
		endMarkup = title.FindFirst(">");
		if (endMarkup > startMarkup)
		{
			title = title.Remove(startMarkup, endMarkup - startMarkup + 1);
		} else  if (endMarkup >= 0) {
			// bogus html ?
			title = title.Remove(endMarkup, 1);
		}
	} while (startMarkup >= 0 && endMarkup >= 0);
	SetText(title);	
}

void
FStringItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color saveH = owner->HighColor();
	rgb_color saveL = owner->LowColor();
	rgb_color colorB, colorF;

	if (IsSelected()) {
		colorB = owner->HighColor();
		colorF = owner->ViewColor();
	}
	else {
		colorB = owner->ViewColor();
		colorF = owner->HighColor();
	}

	if (IsSelected() || complete) {
		owner->SetHighColor(colorB);
		owner->FillRect(frame);
	}

	/*
	if (visited) {
		colorF.red = 127;
		colorF.green = 0;
		colorF.blue = 0;
		colorF.alpha = 0;
	}
	*/
	
	owner->MovePenTo(frame.left+4, frame.bottom-2);

	owner->SetHighColor(colorF);
	owner->SetLowColor(colorB);

	float radius = (frame.bottom - frame.top) / 6;
	BPoint pos( frame.left+radius+2, frame.top + (frame.bottom-frame.top) / 2 );
	pattern pat = B_SOLID_HIGH;
	if (!visited)
		owner->FillEllipse(pos, radius, radius, pat);
	//else
	//	owner->StrokeEllipse(pos, radius, radius, pat);
	
	owner->MovePenBy(2*radius+4, 0);
	owner->DrawString(Text());
	
	// restore Highcolor:
	owner->SetHighColor(saveH);
	owner->SetLowColor(saveL);
}


int compare_func(const BListItem* firstArg, const BListItem* secondArg)
{
	FStringItem *a = (FStringItem*) firstArg;
	FStringItem *b = (FStringItem*) secondArg;	
	
	return strcmp(a->Text(), b->Text());
}

int compare_func(const void* firstArg, const void* secondArg)
{
	FStringItem *a = (FStringItem*) firstArg;
	FStringItem *b = (FStringItem*) secondArg;	
	
	return strcmp(a->Text(), b->Text());
}
