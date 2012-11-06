/* Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence */

#include "feedlistview.h"

#include <NetAddress.h>
#include <NetEndpoint.h>
#include <TranslationUtils.h>

#ifdef TRACE_LISTVIEW
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif

char* getFavicon(BString st, size_t& size)
{
	size = 0;

	BNetAddress *addr;
	try {
		addr = new BNetAddress(st.String(),80);
		if (addr->InitCheck() != B_OK) {
			puts("...");
			return NULL;
		}
	}
	catch(...) {
		FPRINT(("Exception?!?"));
		return NULL;
	}
	
	BNetEndpoint sock;
	if (sock.InitCheck() != B_OK)
		return NULL;

	sock.SetTimeout( 2000000 );		
    if (sock.Connect(*addr) != B_OK) {
    	sock.Close();
    	return NULL;
    }

    char cmd[1000];

	sprintf(
		cmd,
		"GET /favicon.ico HTTP/1.1\r\nHost: %s\r\nUser-Agent: friss/0.7\r\n"
		"Accept: */*\r\nConnection: Close\r\n\r\n",
		st.String()
	);

	sock.SetNonBlocking(true);
    if (sock.Send(cmd, strlen(cmd), 0) == B_ERROR) {
    	puts(sock.ErrorStr());
    	sock.Close();
		return NULL;
    }
    
    if (!sock.IsDataPending(5000000)) {
    	FPRINT(("Error: Server did not respond."));
    	return NULL;
    }
  
	size_t bufsize = 40000;
    BNetBuffer buffy(bufsize);
 
 	sock.SetNonBlocking(false);
 	sock.SetTimeout( 5000000 );
 	
    int read = 0, nr=0;
    while ((read = sock.Receive(buffy, bufsize-1)) > 0) {
    	++nr;
    }

   	//printf("Reading: %s\n", sock.ErrorStr() );
	sock.Close();
	
	size = buffy.Size();
	char* buf = (char*)malloc(size);
	if (size>bufsize)
		size = bufsize-1;
    buffy.RemoveData(buf,size);
    
	// Do basic HTTP parsing:
	BString result;
	result.Append(buf, 12).Remove(0,9);
	int http_return_code = atoi(result.String());
	
	switch (http_return_code)
	{
		case 200: // OK
		{
			// remove http headers
			BString answer(buf);
			int start = answer.FindFirst("\r\n\r\n");
			size -= start;
			return buf+start+4;
		}
		
		default:
			printf("LoadFeedNet() failed, Server response was '%d'\n", http_return_code);
			FPRINT(( "DATA: %d bytesn==n\n%s\nn==n", size, buf));
	}
	
	return NULL;

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
	textFrame.left += 16 + 2 * 5;
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
