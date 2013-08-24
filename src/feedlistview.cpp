/* Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence */

#include "feedlistview.h"

#include <TranslationUtils.h>
#include <Url.h>
#include <UrlSynchronousRequest.h>

#ifdef TRACE_LISTVIEW
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif

char* getFavicon(BString host, size_t& size)
{
	BUrl url;
	url.SetProtocol("http");
	url.SetHost(host);
	url.SetPort(80);
	url.SetPath("/favicon.ico");

	BUrlSynchronousRequest request(url);
	request.Perform();
	request.WaitUntilCompletion();
	const BUrlResult& result = request.Result();

	// FIXME the constness of the result prevents us from reading from it !
	const BMallocIO& io = result.RawData();

	size = io.BufferLength();
	char* buf = (char*)malloc(size);
	memcpy(buf, io.Buffer(), size);
	return buf;
}


typedef struct {
	FeedListItem* item;
	XmlNode* node;
} Cookie;


/* static */
status_t FeedListItem::GetIcon(void* cookie)
{
	Cookie* data = (Cookie*)cookie;
	FeedListItem* item = data->item;

	// Extract hostname from full feed URL
	BString st(data->node->Attribute("xmlUrl"));
	st.RemoveFirst("http://");
	st.Truncate( st.FindFirst('/') );
	
	size_t icoSize;
	char* icon = getFavicon(st, icoSize);
	if (icoSize > 0 && icon != NULL) {
		BMemoryIO m(icon, icoSize);
		item->fIcon = BTranslationUtils::GetBitmap(&m);
		if (item->fParent) {
			item->fParent->LockLooper();
			item->fParent->Invalidate();
			item->fParent->UnlockLooper();
		}
	}

	delete data;
	return B_OK;
}


FeedListItem::FeedListItem(XmlNode* n)
	: BStringItem(n->Attribute("text"))
	, fIcon(NULL)
	, fParent(NULL)
{
	Cookie* cookie = new Cookie();
	cookie->item = this;
	cookie->node = n;

	thread_id thread = spawn_thread(GetIcon, "iconGetter", B_LOW_PRIORITY,
		cookie);
	resume_thread(thread);
}


void FeedListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BRect textFrame = frame;
	textFrame.left += 16 + 5;
	BStringItem::DrawItem(owner, textFrame, complete);

	// draw the icon
	frame.left += 4;
	frame.right = frame.left + 16;

	if (fIcon != NULL && fIcon->IsValid()) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, frame);
	}
}


void FeedListItem::Update(BView* owner, const BFont* font)
{
	fParent = owner;
	BStringItem::Update(owner, font);
}
