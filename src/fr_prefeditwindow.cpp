#include "fr_prefeditwindow.h"
#include "frissconfig.h"
#include <stdlib.h>

#define WIN_WIDTH	400
#define WIN_HEIGHT	200


FPrefEditWindow::FPrefEditWindow(BWindow* parent, XmlNode* itemNode, BPoint point, bool bItem)
	: BWindow(BRect(point.x, point.y, point.x+WIN_WIDTH, point.y+WIN_HEIGHT), "Edit Item", B_MODAL_WINDOW, B_NOT_RESIZABLE | B_WILL_DRAW | B_FRAME_EVENTS)
{
	SetLook(B_TITLED_WINDOW_LOOK);
	SetFeel(B_MODAL_APP_WINDOW_FEEL);
	
	AddChild( bbox = new BBox( Bounds(), "bbox", B_FOLLOW_ALL_SIDES, B_WILL_DRAW, B_NO_BORDER ) );

	item = itemNode;
	mParent = parent;
	
	isItem = bItem;
	
	BRect tr = Bounds();
	tr.InsetBy(5,5);
	tr.bottom = tr.top + 20;


	// Text fields	
	bbox->AddChild( tName = new BTextControl(tr, "tName", _T("Name"), "", new BMessage(CMD_CHECK)) );
	tName->SetDivider(50);

	if (isItem) {		
		tr.OffsetBy(0,25);
		
		bbox->AddChild( tUrl = new BTextControl(tr, "tUrl", _T("URL"), "http://", new BMessage(CMD_CHECK)) );
		tUrl->SetDivider(50);
		
		tr.OffsetBy(0,25);
		bbox->AddChild( cItemAddDesc = new BCheckBox(tr, "cAddDesc", _T("Add Description to title"), NULL) );
		
		tr.OffsetBy(0,45);
		BPopUpMenu* men = new BPopUpMenu(_T("BrowserSelect_"));
		{
			men->AddItem( miBDef = new BMenuItem( _T("Default"), NULL ) );
			men->AddItem( miBNpos = new BMenuItem( _T("Netpositive"), NULL ) );
			men->AddItem( miBFox = new BMenuItem( _T("Firefox"), NULL) );
			men->AddItem( miBCust = new BMenuItem( _T("Custom"), NULL ) );
		}
		bbox->AddChild( mfBrowser=new BMenuField( tr, "BrowserSelect", _T("Browser for this feed"), men ) );
	}
	else {
		tUrl			= NULL;
		cItemAddDesc	= NULL;
		miBDef			= NULL;
		miBNpos			= NULL;
		miBFox			= NULL;
		miBCust			= NULL;
		mfBrowser		= NULL;
	}

	{ // Buttons		
		BRect wr = Bounds();
		wr.InsetBy(5,5);
		wr.top += 5;
		
		BRect br(wr.left, wr.bottom-30, 0, 0);
		
		bbox->AddChild( bRevert = new BButton(br,"BRevert",_T("Revert to Original"),new BMessage(CMD_REVERT)) );
		bRevert->ResizeToPreferred();
		br.OffsetBy(bRevert->Bounds().Width()+10,0);
		
		/*
		bbox->AddChild( bCancel = new BButton(br,"BSave",_T("Cancel"),new BMessage(CMD_CANCEL) ) );
		bCancel->ResizeToPreferred();
		*/
		
		bbox->AddChild( bSave = new BButton(br,"BSave",_T("Save and Close"),new BMessage(CMD_SAVE) ) );
		bSave->ResizeToPreferred();
	}

	UpdateData();
}


FPrefEditWindow::~FPrefEditWindow()
{
}


void
FPrefEditWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	
	case CMD_SAVE:
		if (CheckFields()) {
			Save();
			PostMessage(B_QUIT_REQUESTED);
		}
		break;
		
	case CMD_REVERT:
		UpdateData();
		CheckFields();
		break;
	
	case CMD_CHECK:
		CheckFields();
		break;
		
	default:
		BWindow::MessageReceived(msg);
	}
}


bool
FPrefEditWindow::QuitRequested()
{
	BMessenger(mParent).SendMessage(MSG_PREF_DONE);
	return BWindow::QuitRequested();
}

void
FPrefEditWindow::Save()
{
	// Save name and update Label:
	item->AddAttribute(OPML_TITLE, tName->Text());
	item->SetText(tName->Text());
	
	if (isItem) {
		item->AddAttribute(OPML_URL, tUrl->Text());
		
		if (cItemAddDesc->Value() == B_CONTROL_ON)
			item->AddAttribute(OPML_ADD_DESCRIPTION, "true");
		else
			item->RemoveAttribute(OPML_ADD_DESCRIPTION);
		
		if (miBNpos->IsMarked())
			item->AddAttribute(OPML_OVERRIDE_BROWSER, BrowserNetpositiveStr);
		else if (miBFox->IsMarked())
			item->AddAttribute(OPML_OVERRIDE_BROWSER, BrowserFirefoxStr);
		else if (miBCust->IsMarked())
			item->AddAttribute(OPML_OVERRIDE_BROWSER, BrowserCustomStr);
		else
			item->RemoveAttribute(OPML_OVERRIDE_BROWSER);
	}
}

void
FPrefEditWindow::UpdateData()
{
	// Edit fields:
	tName->SetText(item->Attribute(OPML_TITLE));
	
	if (isItem) {
		tUrl->SetText(item->Attribute(OPML_URL));
		
		// Checkbox for "Add Descrition"
		bool bad = false;
		const char* bs = item->Attribute(OPML_ADD_DESCRIPTION);
	
		if (bs) {
			BString addD( bs );
			if (addD.ICompare("true")==0)
				bad = true;
		}
		if (bad)
			cItemAddDesc->SetValue( B_CONTROL_ON );
		else
			cItemAddDesc->SetValue( B_CONTROL_OFF );
			
			
		// Browser Settings for this node:
		BrowserType_t a = BrowserDefault;
		const char* browser = item->Attribute(OPML_OVERRIDE_BROWSER);
		if (browser)
			a = (BrowserType_t)atoi(browser);
	
		switch(a) {
			case BrowserNetpositive:
				miBNpos->SetMarked(true);
				break;
			
			case BrowserFirefox:
				miBFox->SetMarked(true);
				break;
			
			case BrowserCustom:
				miBCust->SetMarked(true);	
				break;
				
			case BrowserDefault:
			default:
				miBDef->SetMarked(true);			
		}
	}	
}

bool
FPrefEditWindow::CheckFields()
{
	bool ok = true;
	if (strlen(tName->Text())==0)
		ok = false;
	
	if (isItem && strlen(tUrl->Text())==0)
		ok = false;
	
	if (ok)
		bSave->SetEnabled(true);
	else
		bSave->SetEnabled(false);
				
	return ok;
}

