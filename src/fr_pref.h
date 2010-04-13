#ifndef FR_PREF
#define FR_PREF

#include <be/InterfaceKit.h>

#include "fr_def.h"
#include "fr_preflistview.h"
#include "fr_prefeditwindow.h"
#include "frissconfig.h"
#include "xmlnode.h"


#define CMD_ADD_ITEM		'Iadd'
#define CMD_EDIT_ITEM		'Iedi'
#define CMD_REMOVE_ITEM		'Irem'

#define CMD_IMPORT_LIST		'Limp'
#define CMD_EXPORT_LIST		'Lexp'

#define	CMD_ADD_FOLDER		'Fadd'
#define	CMD_REMOVE_FOLDER	'Frem'


class FrissPrefWin : public BWindow
{
public:
	FrissPrefWin(BView* thefv,FrissConfig* conf,XmlNode* xList, BRect frame, const char* Title); 
	virtual ~FrissPrefWin();
	virtual	void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	
	
protected:
	void	OpenImportFileDialog();
	void	Import(XmlNode* parent, entry_ref* ref);

	void	OpenExportFileDialog();
	void	Export(XmlNode* parent, entry_ref* ref);
	
	void	ItemPopup(XmlNode* node, BPoint point);
	void	EditItem(XmlNode* node);

	void	ReLabel();
	
private:
	BMessenger*		mess;
	
	BTabView		*tabView;
	BTab			*tabFeeds, *tabColor, *tabMisc;
	
	// Controls:
	PrefListView*	bv;
		
	BButton*		bAdd;
	BButton*		bRem;
	BButton*		bEdi;
	
	BButton*		bImp;
	BFilePanel*		m_pFileOpenPanel;
	BFilePanel*		m_pFileSavePanel;
	
	BButton*		bAddFolder;
	

	// TAB: Colors
	BBox			*bBack, *bFore;
	BRadioButton	*cColTransparent, *cColDesktop, *cColCustom;
	
	BColorControl*	cColor;
	
	BRadioButton	*cColForeAdapt, *cColForeCustom;
	BColorControl*	cColorHigh;
	
	// TAB: Misc	
	BTextControl*	tRefresh;
	BCheckBox*		tRefrAdv;
	
	BMenuField*		mfScrollbar;
	BMenuItem		*miAu, *miOn, *miOf;
	
	BMenuField*		mfWindowMode;
	BMenuItem		*miSimple, *miPreview, *miFull;
	
	BBox* 			boxBrowser;
	BRadioButton	*cBrowserNetP, *cBrowserFox, *cBrowserCustom;
	BTextControl*	tBrowserMime;

	
	// we need to remember which item was selected for editing
	int	editi;
	// also save node for import/export
	XmlNode* 		m_pCurrentNode;
	int				m_iImpExpMode;
	
	// Config and messaging
	BView*			fv;
	FrissConfig*	config;
	XmlNode*		theList;
};


#endif
