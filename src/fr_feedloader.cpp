#include "fr_feedloader.h"
#include "load.h"

//#define TRACE_FEEDLOADER

#ifdef TRACE_FEEDLOADER
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif

FrFeedLoader::FrFeedLoader(BView* parentView) :
	BLooper("feedloader", B_NORMAL_PRIORITY, B_LOOPER_PORT_DEFAULT_CAPACITY)
{
	parent = parentView;
	stateLoading = false;
}


void
FrFeedLoader::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'LOAD':
			Load(msg->FindString("url"));
			break;
		default:
			BLooper::MessageReceived(msg);
	}
}

bool
FrFeedLoader::Busy()
{
	return stateLoading;
}

void
FrFeedLoader::Load(BString url)
{
	if (stateLoading) {
		FPRINT(("FL: still busy\n"));
		return;
	}
		
	stateLoading = true;
	FPRINT(("FL: getting busy\n"));
	
	size_t bufsize;
	char* buf = LoadFeedNet(url.String(),bufsize);
		
	FPRINT(("FL: loading done: %i\n", n));
	
	stateLoading = false;
	
	if (buf) {
		BMessage* msg = new BMessage(MSG_LOAD_DONE);
		msg->AddPointer("data", buf);
		parent->Window()->PostMessage(msg, parent);
	}
	else
		parent->Window()->PostMessage(MSG_LOAD_FAIL,parent);
	
	FPRINT(("FL: notified, done.\n"));
}
