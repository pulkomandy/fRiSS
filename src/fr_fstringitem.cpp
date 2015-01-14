/* FRiSS RSS feed reader
 * Copyright 2011, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT licence
 */

#include "fr_fstringitem.h"


FStringItem::FStringItem()
	: BStringItem("<empty>", 0, true)
	, sDesc(NULL)
{
	visited = false;
	bAddDesc = false;
}

FStringItem::FStringItem(const char *text, const char *url, uint32 level,
		bool expanded, bool addDescFlag) :
	BStringItem(text, level, expanded),
	sUrl(url),
	sDesc(NULL)
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

const XmlNode* const
FStringItem::Desc() const
{
	return sDesc;
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
FStringItem::SetDesc(const XmlNode* const desc)
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
