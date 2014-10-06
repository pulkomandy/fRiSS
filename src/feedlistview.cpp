/* Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence */

#include "feedlistview.h"

#include <TranslationUtils.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlSynchronousRequest.h>

#ifdef TRACE_LISTVIEW
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif

class SynchronousListener: public BUrlProtocolListener
{
	public:
		virtual	~SynchronousListener() {};
			void	DataReceived(BUrlRequest*, const char* data, off_t position,
					ssize_t size) {
			result.WriteAt(position, data, size);
		}
		BMallocIO result;
};


char* getFavicon(BString host, size_t& size)
{
	BUrl url;
	url.SetProtocol("http");
	url.SetHost(host);
	url.SetPort(80);
	url.SetPath("/favicon.ico");

	SynchronousListener listener;
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, &listener);
	request->Run();
	while(request->IsRunning()) snooze(1000);

	// FIXME the constness of the result prevents us from reading from it !

	size = listener.result.BufferLength();
	char* buf = (char*)malloc(size);
	memcpy(buf, listener.result.Buffer(), size);

	delete request;

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
