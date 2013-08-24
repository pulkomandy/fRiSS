#include "fr_def.h"

class FrFeedLoader : public BLooper
{
public:
	FrFeedLoader(BView* parentView);
	
	virtual void	MessageReceived(BMessage *msg);
	
	BString sUrl;
	
	bool Busy();
	
private:
	void Load();
	
	BView*	parent;
	bool	stateLoading;
};
