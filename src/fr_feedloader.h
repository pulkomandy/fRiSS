#include "fr_def.h"

class FrFeedLoader : public BLooper
{
public:
	FrFeedLoader(BView* parentView, char* buf);
	
	virtual void	MessageReceived(BMessage *msg);
	
	BString sUrl;
	
	bool Busy();
	
private:
	void Load();
	
	BView*	parent;
	char*	pBuf;
	bool	stateLoading;
};
