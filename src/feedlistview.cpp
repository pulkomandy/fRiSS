/* Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence */

#include "feedlistview.h"

#include <NetAddress.h>
#include <NetEndpoint.h>
#include <TranslationUtils.h>

#define FPRINT(x) printf x

char* getFavicon(BString st, size_t& size)
{
	size = 0;

	printf("getting favicon from %s\n", st.String());

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

FeedListItem::FeedListItem(XmlNode* n)
	: BStringItem(n->Attribute("text"))
	, fIcon(NULL)
{
	// Extract hostname from full feed URL
	BString st(n->Attribute("xmlURL"));
	st.RemoveFirst("http://");
	st.Truncate( st.FindFirst('/') );
	
	size_t icoSize;
	char* icon = getFavicon(st, icoSize);
	if (icoSize > 0 && icon != NULL) {
		BMemoryIO m(icon, icoSize);
		fIcon = BTranslationUtils::GetBitmap(&m);
	}
}


void FeedListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BStringItem::DrawItem(owner, frame, complete);

	// draw the icon
	frame.left = frame.right - 16;

	if (fIcon != NULL && fIcon->IsValid()) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, frame);
	}
}
