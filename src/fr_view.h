#ifndef FR_VIEW
#define FR_VIEW

#include "fr_def.h"

#include <GridView.h>
#include <String.h>

#include "frissconfig.h"
#include "fr_pref.h"
#include "fr_flistview.h"
#include "fr_fstringitem.h"
#include "xmlnode.h"

extern const char *app_signature;
class FTextView;
class FeedLoadListener;

class FrissView : public BGridView
{
public:
	FrissView(FrissConfig* newconf, XmlNode* x_root);
	FrissView(BMessage* msg);
	~FrissView();
	
	static	BArchivable	*Instantiate(BMessage *data);
	virtual	status_t	Archive(BMessage *data, bool deep = false) const;	

	// Initialise this object
	virtual void AllAttached();
	
	virtual void Pulse();
	
	// message handling:	
	virtual	void MessageReceived(BMessage *msg);

	// for context menu / popup
	virtual	void MouseDown(BPoint point);

	
	// Load or Re-Load or Invoke the loading of a feed
	// Parameter idx is the index in config->list
	void Load(uint32 idx, XmlNode* direct = NULL);
	void LoadNext();
	void LoadPrev();
	
	void StartPopup(BPoint point);
	
	void ItemSelected(FStringItem* fi);

	void Launch(FStringItem* fi);
	// TODO move this method to somewhere where it makes more sense
	void OpenURL(BString url);
	
	virtual void FrameResized(float width, float height);

	// return our config to parent:	
	FrissConfig*	Config();
	XmlNode*		GetFeedTree();
	
	void			UpdateVisitedLink(const char* url);
	bool			InitialVisitedLink(const char* pfad);
	void			VisitRef(entry_ref* ref);
	
	void			NodeViewInformation(FStringItem* node);
		
private:
	// display error message
	void Error(const char* err);
	
	// Updates the window, like hiding/unhiding the preview pane etc
	void UpdateWindowMode();
	
	// Changes all colors
	void UpdateColors();
	
	// Build feed list in popup submenu
	int		BuildPopup(XmlNode *node, BMenu* menu, int nr=0);
	void	DeletePopup(BMenu* menu);
	void	ReBuildPopup(XmlNode *node, BMenu* menu);

	// Teil2 des Loaders
	void LoadDone(char* data);
	
	void OnWorkspaceChanged();
	
	// display tvTextView
	void	ShowPreviewArea(bool show=true);


	// get the nth item in tree
	XmlNode*	FindItem(int n, int& is, XmlNode* node);

	// Some member controls
	FrissPrefWin*	pref;
	
	BPopUpMenu*		pop;			// popup menu
	BMenuItem*		miGo;			// "Go"
	BMenuItem*		miInfo;			// information on this feed
	BMenuItem*		miRefr;			// "Refresh"
	BMenuItem*		miNext;			// "Next Feed"
	BMenuItem*		miPrev;			// "Previous Feed"
	BMenu*			mList;			// the feed list popup
	BMenuItem*		miOpts;			// "Options"
	BMenuItem*		miAbout;		// "About Friss"
	BMenuItem*		miDebug;		// debug item
	
	BMenuItem*		miActiveFeed;	// Workaround since FindMarked is not recursive :-/
	
	FTextView*		tvTextView;
	BScrollView*	sbTextView;
	
	bool			sb_hidden;
	FListView*		listview;

	// we just store a pointer to the config struct because
	// FrissMasterView likes to have it on Shutdown in order to
	// properly save it to disk/archive
	FrissConfig*	config;
	
	// do some time measurement for refreshes
	bool			pulsing;
	unsigned int	pulses;
	unsigned int	last_reload;
	
	// inv=true means that the PrefWindow wishes us to reload and
	// invalidate
	bool			inv;
	
	// Feedlist
	XmlNode*		theRoot;			// the file
	XmlNode*		theList;			// container of all <item>s
	XmlNode*		currentFeed;		// must be a subitem of theRoot/theList
	
	// List for Output
	BObjectList<FStringItem>* tlist;

	FeedLoadListener* fLoadListener;
	
	// Running as replicant?
	bool replicant;
	
	// WindowMode
	WindowMode_t	currentWindowMode;
	
	BMessenger*		messenger;
	
	BString			strDebug;
};

#endif
