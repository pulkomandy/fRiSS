#include "fr_feedloader.h"
#include "load.h"

//#define TRACE_FEEDLOADER

#ifdef TRACE_FEEDLOADER
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif

FrFeedLoader::FrFeedLoader(BView* parentView, char* buf) :
	BLooper("feedloader", B_NORMAL_PRIORITY, B_LOOPER_PORT_DEFAULT_CAPACITY)
{
	parent = parentView;
	pBuf = buf;
	stateLoading = false;
}


void
FrFeedLoader::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'LOAD':
			Load();
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
FrFeedLoader::Load()
{
	if (stateLoading) {
		FPRINT(("FL: still busy\n"));
		return;
	}
		
	stateLoading = true;
	FPRINT(("FL: getting busy\n"));
	
	int bufsize = BUFSIZE;
	int n;
	
	if (sUrl.Compare("file://",7)==0)
		n = LoadFeedFile(sUrl.String(),pBuf,bufsize-1);
	else if (sUrl.Compare("http://",7)==0)
		n = LoadFeedNet(sUrl.String(),pBuf,bufsize-1);
		
	FPRINT(("FL: loading done: %i\n", n));
	
	stateLoading = false;
	
	if (n > 0) {
		pBuf[n]=0;
		parent->Window()->PostMessage(MSG_LOAD_DONE,parent);
	}
	else
		parent->Window()->PostMessage(MSG_LOAD_FAIL,parent);
	
	FPRINT(("FL: notified, done.\n"));
}
