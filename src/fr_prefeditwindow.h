#ifndef FPREFEDITWINDOW
#define FPREFEDITWINDOW

#include "fr_def.h"

#define CMD_SAVE			'Isav'
#define CMD_REVERT			'Irev'


#define CMD_CHECK			'CHK!'

class FPrefEditWindow : public BWindow
{
public:
	FPrefEditWindow(BWindow* parent, XmlNode* itemNode, BPoint point, bool item=true);
	virtual 		~FPrefEditWindow();
	
	virtual void	MessageReceived(BMessage* msg);
	virtual bool	QuitRequested();
	
private:
	// saves the values from the UI to the XmlNode
	void			Save();
	
	// copies the values from the XmlNode to the UI
	void			UpdateData();
	
	// returns true, if tName and tUrl are both not empty
	bool			CheckFields();

	// -------------------------------------
	BWindow*		mParent;
	XmlNode*		item;
	
	BBox*			bbox;
	
	BButton*		bSave;
	BButton*		bCancel;
	BButton*		bRevert;

	BTextControl*	tName;
	BTextControl*	tUrl;
	BTextControl*	tDesc;
	BCheckBox*		cItemAddDesc;		

	BMenuItem 		*miBDef, *miBNpos, *miBFox, *miBCust;
	BMenuField*		mfBrowser;
	
	
	bool			isItem;
};


#endif
