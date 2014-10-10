#include "fr_def.h"

class FrFeedLoader : public BLooper
{
public:
	FrFeedLoader(BView* parentView);
	
	virtual void	MessageReceived(BMessage *msg);
	
	bool Busy();
	
private:
	void Load(BString url);
	
	BView*	parent;
	bool	stateLoading;
};
