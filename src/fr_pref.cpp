#include "fr_pref.h"
#include "fr_fstringitem.h"

#include <be/storage/FilePanel.h>
#include <be/storage/Entry.h>
#include <be/storage/Path.h>


#include <stdlib.h>
#include "frissconfig.h"


FrissPrefWin::FrissPrefWin(BView* thefv,FrissConfig* conf, XmlNode* xList, BRect frame, const char* Title)
	: BWindow(frame, Title, B_MODAL_WINDOW, B_NOT_RESIZABLE | B_WILL_DRAW | B_FRAME_EVENTS)
{
	SetLook(B_TITLED_WINDOW_LOOK);
	SetFeel(B_MODAL_APP_WINDOW_FEEL);
	
	fv = thefv;		// "parent"
	config = conf;
	theList = xList;
	
	// selected item for editing:
	editi = -1;
	
	int width = 400, height = 350;
	MoveTo(1024/2-width/2,768/2-height/2);
	ResizeTo(width,height);

	BRect r;
	
	m_pFileOpenPanel = m_pFileSavePanel = NULL;
	m_pCurrentNode = NULL;
	m_iImpExpMode = 0;
	
	mess = new BMessenger(this);
	
	// Background...
	r = Bounds();
	BBox* bvx = new BBox(r, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW, B_NO_BORDER);
	bvx->SetViewColor(216,216,216);
	AddChild(bvx);
	
	// TabView
	r = Bounds();
	r.InsetBy(5,5);
	tabView = new BTabView( r, "tabView", B_WIDTH_AS_USUAL, B_FOLLOW_ALL_SIDES );
	tabView->SetViewColor(216,216,216,0);
	
	r = tabView->Bounds();
	//r.InsetBy(5,5);
	r.bottom -= tabView->TabHeight();
	
	BBox *bbFeeds, *bbColor, *bbMisc;
	
	tabFeeds = new BTab();
	tabView->AddTab( bbFeeds = new BBox( r, "bbFeeds", 0, B_WILL_DRAW, B_NO_BORDER ), tabFeeds );
	tabFeeds->SetLabel(_T("Feeds"));
	
	tabColor = new BTab();
	tabView->AddTab( bbColor = new BBox( r, "bbColor", 0, B_WILL_DRAW, B_NO_BORDER ), tabColor );
	tabColor->SetLabel(_T("Colours"));
	
	tabMisc = new BTab();
	tabView->AddTab( bbMisc = new BBox( r, "bbMisc", 0, B_WILL_DRAW, B_NO_BORDER ), tabMisc );
	tabMisc->SetLabel(_T("Misc"));
		
	bvx->AddChild(tabView);
	tabView->SetTabWidth(B_WIDTH_FROM_WIDEST);
	
	/* ------ Feeds ------ */
	{
		//bbFeeds->AddChild( bEdi = new BButton(br,"BEdi",_T("Edit"),new BMessage(CMD_EDIT_ITEM)) );
			
		const int buttonsize = 60;
		
		BRect wr = bbFeeds->Bounds();
		wr.InsetBy(5,5); wr.top += 5;
		BRect br(5, wr.bottom-30, 0, 0/*wr.bottom-10*/);
		br.right = br.left + buttonsize;
		
		bbFeeds->AddChild( bAdd = new BButton(br,"BAdd",_T("New Item..."),new BMessage(CMD_ADD_ITEM) ) );
		bAdd->ResizeToPreferred();
		
		br.OffsetBy(bAdd->Bounds().Width()+10,0);
		bbFeeds->AddChild( bEdi = new BButton(br,"BEdi",_T("Edit"),new BMessage(CMD_EDIT_ITEM)) );
		bEdi->ResizeToPreferred();
		
		br.OffsetBy(bEdi->Bounds().Width()+10,0);
		bbFeeds->AddChild( bRem = new BButton(br,"BRem",_T("Remove"),new BMessage(CMD_REMOVE_ITEM)) );
		bRem->ResizeToPreferred();
		
		// Remove and Edit have to be enabled by selecting an entry
		bAdd->SetEnabled(true);
		bEdi->SetEnabled(false);
		bRem->SetEnabled(false);
		
		wr.bottom = br.top - 5;
		
		BRect fr = wr;
		fr.InsetBy(5,5);
		fr.right -= B_V_SCROLL_BAR_WIDTH;
		
		bv = new PrefListView(theList, fr, "feedlist", B_SINGLE_SELECTION_LIST);
		bv->BuildView(theList);
	
		BScrollView *sv = new BScrollView("scrollview",bv,B_FOLLOW_RIGHT|B_FOLLOW_TOP|B_FOLLOW_BOTTOM,B_FRAME_EVENTS,false,true);
		//sv->Scrollbar(B_HORIZONTAL)->SetRange(
		bbFeeds->AddChild(sv);

	}
	
	/* ----- Colour ----- */
	{
		BRect ccb = bbColor->Bounds();
		ccb.InsetBy(5,5);
		
		const float backsize = 160.0f;
		
		BRect b = ccb;
		bBack = new BBox(BRect(b.left,b.top,b.right,backsize));
		bBack->SetLabel(_T("Background"));
		bbColor->AddChild(bBack);
		
		b = bBack->Bounds();		
		b.InsetBy(5,5);
		b.top += 10;
		b.bottom = b.top + 20;		
		bBack->AddChild( cColTransparent = new BRadioButton( b, "CBTransp", _T("Transparent background"/*" (replicants only)"*/), new BMessage( MSG_COL_CHANGED ) ) );
		b.top += 20;
		bBack->AddChild( cColDesktop = new BRadioButton( b, "CBDesktop", _T("Adapt to Desktop background colour"), new BMessage( MSG_COL_CHANGED ) ) );
		b.top += 30;
		bBack->AddChild( cColCustom = new BRadioButton( b, "CBTransp", _T("Custom background colour:"), new BMessage( MSG_COL_CHANGED ) ) );
		b.top += 20;
		BPoint p(5,b.top);
		bBack->AddChild( cColor = new BColorControl(p, B_CELLS_32x8, 7.0f, "colorpicker", new BMessage( MSG_COL_CHANGED ), true) );
		cColor->SetValue( conf->col );
		
		switch (conf->ColBackMode) {
		case ColBackTransparent:
			cColTransparent->SetValue(B_CONTROL_ON);
			break;

		case ColBackDesktop:
			cColDesktop->SetValue(B_CONTROL_ON);
			break;
			
		default:
			cColCustom->SetValue(B_CONTROL_ON);
		}
		
#ifndef ALLOW_TRANSP
		cColTransparent->SetEnabled(false);
#endif
		
		/* ---- TEXT COLOR ---- */
		b = ccb;
		bFore = new BBox(BRect(b.left,backsize+10.0f,b.right,b.bottom));
		bFore->SetLabel(_T("Text Colour"));
		bbColor->AddChild(bFore);
		
		b = bFore->Bounds();
		b.InsetBy(5,5);
		b.top += 10;
		b.bottom = b.top + 20;

		bFore->AddChild( cColForeAdapt = new BRadioButton( b, "CFAdapt", _T("Adapt colour to background" /* "(black on bright/white on dark)"*/), new BMessage( MSG_COL_CHANGED ) ) );
		b.top += 30;
		bFore->AddChild( cColForeCustom = new BRadioButton( b, "CFCustom", _T("Custom text colour:"), new BMessage( MSG_COL_CHANGED ) ) );
		b.top += 20;
		BPoint p2(5,b.top);
		bFore->AddChild( cColorHigh = new BColorControl(p2, B_CELLS_32x8, 7.0f, "colorpicker_high", new BMessage( MSG_COL_CHANGED ), true) );
		cColorHigh->SetValue( conf->high );
		
		switch(conf->ColForeMode) {
		case ColForeAdapt:
			cColForeAdapt->SetValue(B_CONTROL_ON);
			break;
		default:
			cColForeCustom->SetValue(B_CONTROL_ON);
		}
	}
	
	/* ----- Misc ----- */
	{
		BRect br = bbMisc->Bounds();
		br.InsetBy(5,5); br.top += 5;
		
		br.bottom = br.top + 20;
		bbMisc->AddChild( tRefrAdv = new BCheckBox(br, "tRefresh", _T("Load next feed instead of current"), NULL) );
		br.OffsetBy(0,30);
		
		BString dummy;
		dummy << (int)(conf->RefreshRate / 120);
		bbMisc->AddChild( tRefresh = new BTextControl(br, "tRefresh", _T("Refresh interval (min)"), dummy.String(), NULL) );
		
		if (config->RefreshAdvances == 1)
			tRefrAdv->SetValue(B_CONTROL_ON);

		br.OffsetBy(0,80);
		
		BPopUpMenu* men = new BPopUpMenu(_T("ScrollbarMode"));

		men->AddItem( miAu = new BMenuItem( _T("Automatic"), new BMessage( MSG_SB_CHANGED ) ) );
		men->AddItem( miOn = new BMenuItem( _T("Show always"), new BMessage( MSG_SB_CHANGED ) ) );
		men->AddItem( miOf = new BMenuItem( _T("Hide always"), new BMessage( MSG_SB_CHANGED ) ) );
		
		bbMisc->AddChild( mfScrollbar=new BMenuField( br, "ScrollbarMode", _T("Show scrollbar"), men ) );
		
		if (config->ScrollbarMode == ScrollbarModeOn)
			miOn->SetMarked(true);
		else if (config->ScrollbarMode == ScrollbarModeOff)
			miOf->SetMarked(true);
		else
			miAu->SetMarked(true);

		men = NULL;
		
		/* --- */
		br.OffsetBy(0,30);
		
#ifdef	OPTIONS_WINDOW_MODE

		men = new BPopUpMenu(_T("WindowMode"));

		men->AddItem( miSimple = new BMenuItem( _T("Simple"), new BMessage( MSG_SB_CHANGED ) ) );
		men->AddItem( miPreview = new BMenuItem( _T("Preview"), new BMessage( MSG_SB_CHANGED ) ) );
		//men->AddItem( miFull = new BMenuItem( _T("Full"), new BMessage( MSG_SB_CHANGED ) ) );
		
		bbMisc->AddChild( mfWindowMode=new BMenuField( br, "WindowMode", _T("Window mode"), men ) );
		
		/*
		if (config->WindowMode == WindowModeFull)
			miFull->SetMarked(true);
		else */if (config->WindowMode == WindowModePreview)
			miPreview->SetMarked(true);
		else
			miSimple->SetMarked(true);
#else
		mfWindowMode = NULL;
		miSimple = miPreview = miFull = NULL;
#endif
		/* --- */
		
		
		BRect bBbox = bbMisc->Bounds();
		bBbox.InsetBy(5,5);
		bBbox.top = bBbox.bottom-110;
		
		boxBrowser = new BBox( bBbox );
		boxBrowser->SetLabel(_T("Browser"));
		bbMisc->AddChild(boxBrowser);
		
		br = boxBrowser->Bounds();
		br.top += 10; br.InsetBy(5,5);
		br.bottom = 20;
		boxBrowser->AddChild( cBrowserNetP = new BRadioButton(br, "cBrowserNetP", _T("NetPositive"), NULL) );
		br.OffsetBy(0,20);
		boxBrowser->AddChild( cBrowserFox = new BRadioButton(br, "cBrowserFox", _T("Mozilla Firefox"), NULL) );
		br.OffsetBy(0,30);
		boxBrowser->AddChild( cBrowserCustom = new BRadioButton(br, "cBrowserCustom", _T("Custom"), NULL) );
		br.OffsetBy(0,20);
		boxBrowser->AddChild( tBrowserMime = new BTextControl(br, "tBrowser", _T("Browser MIME-Type"), config->BrowserMime.String(), NULL) );	
		
		switch (config->BrowserType) {
			case BrowserCustom:
				cBrowserCustom->SetValue(B_CONTROL_ON);
				break;
				
			case BrowserFirefox:
				cBrowserFox->SetValue(B_CONTROL_ON);
				break;
				
			case BrowserNetpositive:
			default:
				cBrowserNetP->SetValue(B_CONTROL_ON);
				break;
		}
	}
}

FrissPrefWin::~FrissPrefWin()
{
	SAFE_DELETE( m_pFileOpenPanel );
	SAFE_DELETE( m_pFileSavePanel );

	delete mess; // clean up the mess
}

void
FrissPrefWin::MessageReceived(BMessage *msg)
{
	int selected;
	XmlNode *item;
			
	switch(msg->what) {
	
		case MSG_P_ITEM_UPDATED:
			{
				// FPrefEditWindow has updated the item and we now have to invalidate the
				// list to reflect the changes
				
				bv->Invalidate();
			}
			break;
		
		case CMD_ADD_ITEM:
			// Changed behaviour: Add new XmlNode and open Edit for it
			{
				XmlNode* newItem = new XmlNode("outline");
				EditItem(newItem);
				bv->AddItem(newItem);
				theList->AddChild(newItem);
				bv->FullListSortItems(&compare_func);
				break;
			}
			
			//Falltrough : edit the item we just added

		case CMD_EDIT_ITEM:
			{
				if (editi == bv->CurrentSelection()) {
					item = (XmlNode*)bv->ItemAt(editi);
					EditItem(item);
				}
			}
			break;

		case CMD_REMOVE_ITEM:
			if ((selected = bv->CurrentSelection())>=0) {
				item = (XmlNode*)bv->ItemAt(selected);
				bv->RemoveItem(selected);
				delete item;
				editi = -1;
			}
			break;
			
		case CMD_IMPORT_LIST:
			{
				OpenImportFileDialog();
			}
			break;
			
		case CMD_ADD_FOLDER:
			{
			}
			break;
			
		case MSG_PREF_DONE:
			bv->Invalidate();
			UpdateIfNeeded();
			break;
		
		case MSG_LIST_CHANGED:
			if ((selected = bv->CurrentSelection())>=0) {
				item = (XmlNode*)bv->ItemAt(selected);
				
				editi = selected;
				
				bEdi->SetEnabled(true);
				bRem->SetEnabled(true);
			}	
			else {
				bEdi->SetEnabled(false);
				bRem->SetEnabled(false);
				editi = -1;
			}
			bAdd->SetEnabled(true);
			break;
			
			
		case MSG_LIST_POPUP:
			{
				BPoint point;
				msg->FindPoint("point", &point);
				int32 idx;
				msg->FindInt32("index", &idx);
				XmlNode* node = (XmlNode*)bv->ItemAt(idx);
				ItemPopup(node, point);
			}
			break;	
		
		case MSG_COL_CHANGED:
			// Save Color Modes
			if (cColTransparent->Value() == B_CONTROL_ON)
				config->ColBackMode = ColBackTransparent;
			else if (cColDesktop->Value() == B_CONTROL_ON)
				config->ColBackMode = ColBackDesktop;
			else
				config->ColBackMode = ColBackCustom;
				
			if (cColForeAdapt->Value() == B_CONTROL_ON)
				config->ColForeMode = ColForeAdapt;
			else
				config->ColForeMode = ColForeCustom;

			config->col = cColor->ValueAsColor();
			config->high = cColorHigh->ValueAsColor();

			// Invoke update:			
			fv->Window()->PostMessage(MSG_COL_CHANGED, fv);
			break;
			
		case MSG_SB_CHANGED:
			// Save ScrollBarMode:			
			if (miAu->IsMarked())
				config->ScrollbarMode = ScrollbarModeAuto;
			else if (miOn->IsMarked())
				config->ScrollbarMode = ScrollbarModeOn;
			else
				config->ScrollbarMode = ScrollbarModeOff;
			
			if (mfWindowMode) {
				/*if (miFull->IsMarked())
					config->WindowMode = WindowModePreview;//WindowModeFull; //todo
				else*/ if (miPreview->IsMarked())
					config->WindowMode = WindowModePreview;
				else
					config->WindowMode = WindowModeSimple;			
			}
				
			// Invoke update:			
			fv->Window()->PostMessage(MSG_SB_CHANGED, fv);		
			break;
			
		case B_SAVE_REQUESTED:
		case B_REFS_RECEIVED:
			{
				//printf("\nPrefWin: REFS RECEIVED\n");
				XmlNode* p;
				if ((selected = bv->CurrentSelection())>=0)
					p = (XmlNode*)bv->ItemAt(selected);
				else
					p = theList;

				uint32		type;
				int32		count;
				entry_ref	ref;
				
				msg->GetInfo( "refs", &type, &count );
				if ( type != B_REF_TYPE)
					return;
				
				//printf("There are %d refs to import!\n", count);
				
				for ( long i = --count; i >= 0; i-- ) {
					if ( msg->FindRef("refs", i, &ref) == B_OK ) {
						if (m_iImpExpMode == 1)
				  			Import(p, &ref);
				  		else if (m_iImpExpMode == 2)
				  			Import(p, &ref);
				  		else {
				  			// Huh?
				  		}
				  			
					}
				}
				
				bv->MakeEmpty();
				bv->BuildView(theList);				
				
				// reset and clean up:
				m_iImpExpMode = 0;

				//puts("done");
			}
			break;
			
		case B_LANGUAGE_CHANGED:
			ReLabel();
			break;
			
		default:
			BWindow::MessageReceived(msg);
	}
}


bool
FrissPrefWin::QuitRequested()
{
	//TODO : all of this should be checked while the user makes change, not on closing the window...
	config->m_iAnz = bv->CountItems();

	// refresh rate	
	int min = atoi(tRefresh->Text());
	if (tRefrAdv->Value() == B_CONTROL_OFF && min < REFRESH_MIN_NORM) {
		char buffer[1000];
		sprintf(buffer, _T("The refresh rate is set to low.\n\nMinimum is %i minutes.\nRecommended: %i (~%i hours)"), REFRESH_MIN_NORM, REFRESH_REC_NORM, REFRESH_REC_NORM / 60);
		(new BAlert(_T("Warning"),buffer,_T("Ok")))->Go();
		return false;
	}
	if (tRefrAdv->Value() == B_CONTROL_ON && min < REFRESH_MIN_ADVC) {
		char buffer[1000];
		sprintf(buffer, _T("The feed advance rate is set to low.\n\nMinimum is %i minutes.\nRecommended: %i"), REFRESH_MIN_ADVC, REFRESH_REC_ADVC);
		(new BAlert(_T("Warning"),buffer,_T("Ok")))->Go();
		return false;
	}	
	
	// we simply cannot allow an empty list
	if (config->m_iAnz == 0) {
		(new BAlert(_T("Warning"),_T("Your feed list must not be empty!"),_T("Ok")))->Go();
		return false;
	}
	
	if (config->Index() >= config->m_iAnz)
		config->SetIndex(0);
	
	// Save Color
	// no need to do that, as we have live preview
	//config->col = cColor->ValueAsColor();

	// Save refresh rate
	config->RefreshRate = min * 120;
	config->RefreshAdvances = (tRefrAdv->Value() == B_CONTROL_ON ? 1 : 0);

	// ScrollbarMode -> COL_CHANGED


	// Browser:
	if (cBrowserNetP->Value() == B_CONTROL_ON)
		config->BrowserType = BrowserNetpositive;
	else if (cBrowserFox->Value() == B_CONTROL_ON)
		config->BrowserType = BrowserFirefox;
	else
		config->BrowserType = BrowserCustom;
	
	config->BrowserMime = tBrowserMime->Text();
	
	// Notify FrissView so that it can apply the changes
	fv->Window()->PostMessage(MSG_PREF_DONE, fv);
	
	// do whatever you want:
	return BWindow::QuitRequested();
}


void
FrissPrefWin::OpenImportFileDialog()
{
	m_iImpExpMode = 1;
	
	if (!m_pFileOpenPanel)
		m_pFileOpenPanel = new BFilePanel(B_OPEN_PANEL, mess, NULL, 0, false, NULL, NULL, true, true);
		
	m_pFileOpenPanel->Show();
}

void
FrissPrefWin::Import(XmlNode* parent, entry_ref* ref)
{
	BEntry e(ref, true);
	if (e.InitCheck() != B_OK)
		return;
	
	BPath path;
	e.GetPath(&path);
	
	XmlNode* loader = new XmlNode("");
	loader->LoadFile(path.Path());
	
	XmlNode* opml = loader->FindChild("opml", NULL, true);
	if (!opml) {
		delete loader;
		puts("Error: no valid OPML file!");
	}
		
	XmlNode* body = opml->FindChild("body", NULL, true);
	
	if (body) {
		int anz = body->Children();
		for (int i=0; i<anz; i++) {
			XmlNode* c = body->DetachChild(0);
			parent->AddChild(c);
		}	
	}
	else
		puts("Error: no valid OPML file!");
		
	delete loader;
}

void
FrissPrefWin::OpenExportFileDialog()
{
	m_iImpExpMode = 2;
	
	if (!m_pFileSavePanel)
		m_pFileSavePanel = new BFilePanel(B_SAVE_PANEL, mess, NULL, 0, false, NULL, NULL, true, true);
		
	m_pFileSavePanel->Show();
}

void
FrissPrefWin::Export(XmlNode* parent, entry_ref* ref)
{
	BEntry e(ref, true);
	if (e.InitCheck() != B_OK)
		return;
	
	BPath path;
	e.GetPath(&path);
	printf("Saving feeds to %s\n",path.Path());

	parent->SaveToFile(path.Path());
	/*
	time_t now = time(NULL);
	BString d(ctime(&now));
	d.RemoveSet("\r\n\t");		
	theRoot->CreateChild("opml/head/dateModified", d.String());

	theRoot->SaveToFile("/boot/home/Desktop/feeds.opml");
	*/
	//...
}
				

void
FrissPrefWin::ItemPopup(XmlNode* node, BPoint point)
{
	bool folder = false, collapsed = false;
	
	m_iImpExpMode = 0;

	BPopUpMenu*	popup = new BPopUpMenu("popup",false,false,B_ITEMS_IN_COLUMN);
	BMenuItem *miCollapse = NULL, *miRemove = NULL, *miEdit = NULL, *miAddSubfolder = NULL;
	BMenuItem *miDup = NULL, *miAddItem = NULL, *miExport = NULL, *miImport = NULL;
	BMenuItem *miSort = NULL;

	if (node) {
		bv->Select(bv->IndexOf(node), false);
		
		if (node->Attribute(OPML_URL)==NULL) {
			folder = true;
			
			if (!node->IsExpanded()) {
				popup->AddItem( miCollapse = new BMenuItem( _T("Expand"), NULL ) );
				collapsed = true;
			}
			else
				popup->AddItem( miCollapse = new BMenuItem( _T("Collapse"), NULL ) );
			
			popup->AddSeparatorItem();
		}

		if (folder)
			popup->AddItem( miEdit = new BMenuItem( _T("Rename"), NULL ) );
		else {
			popup->AddItem( miEdit = new BMenuItem( _T("Edit..."), NULL ) );
			popup->AddItem( miDup = new BMenuItem( _T("Duplicate"), NULL ) );
		}
		
		popup->AddItem( miRemove = new BMenuItem( _T("Remove"), NULL ) );
		
		//miInfo->SetEnabled(false);
		if (folder) {
			popup->AddSeparatorItem();
			popup->AddItem( miAddItem = new BMenuItem( _T("Add new Item..."), NULL ) );
			popup->AddItem( miAddSubfolder = new BMenuItem( _T("Add Subfolder..."), NULL ) );
		}
	}
	else {
		folder = true;
		popup->AddItem( miAddItem = new BMenuItem( _T("Add new Item..."), NULL ) );
		popup->AddItem( miAddSubfolder = new BMenuItem( _T("Add new Folder..."), NULL ) );
	}
	
	if (folder) {
		popup->AddSeparatorItem();
		popup->AddItem( miImport = new BMenuItem( _T("Import..."), NULL ) );
		popup->AddItem( miExport = new BMenuItem( _T("Export..."), NULL ) );
		popup->AddSeparatorItem();
		popup->AddItem( miSort = new BMenuItem( _T("Sort by Name"), NULL ) );
	}
		
	bv->ConvertToScreen(&point);
	point.x -= 5;
	point.y -= 5;
	BMenuItem *ret = popup->Go(point);
	if (!ret)
		return;
	
	if (ret == miEdit)
		EditItem(node);
	else if (ret == miRemove) {
		bv->RemoveItem(node);
		node->Parent()->RemoveChild(node);
		bv->Invalidate();
	}
	else {
		if (folder) {
			if (ret == miAddItem) {
				XmlNode* newnode;
				
				if (node) {
					newnode = new XmlNode(node->Parent(), "outline", node->OutlineLevel()+1, true);
					node->AddChild(newnode,0);
					bv->AddUnder(newnode, node);
				}
				else {
					newnode = new XmlNode(theList, "outline");
					theList->AddChild(newnode);
					bv->AddItem(newnode);
				}

				BString s(_T("New Item"));
				newnode->AddAttribute(OPML_TITLE, s.String());
				newnode->SetText(s.String());
				newnode->AddAttribute(OPML_URL, "http://");
				
				bv->Select(bv->IndexOf(newnode),false);
				
				bv->Invalidate();
				
				EditItem(newnode);
			}
			else if (ret==miAddSubfolder) {
				XmlNode* newnode;
				
				if (node) {
					newnode = new XmlNode(node->Parent(), "outline", node->OutlineLevel()+1, true);
					node->Parent()->AddChild(newnode, node->Parent()->IndexOf(node)+1);
					bv->AddItem(newnode, bv->IndexOf(node)+bv->CountItemsUnder(node, false)+1);
				}
				else {
					newnode = new XmlNode(theList, "outline");
					theList->AddChild(newnode);
					bv->AddItem(newnode);
				}

				newnode->SetMarked();
				BString s(_T("New Folder"));
				newnode->AddAttribute(OPML_TITLE, s.String());
				newnode->SetText(s.String());				
				bv->Select(bv->IndexOf(newnode),false);
				
				bv->Invalidate();
				
				EditItem(newnode);
			}
			else if (ret==miImport) {
				if (!node)
					node = theList;
					
				m_pCurrentNode = node;
				OpenImportFileDialog();
				// the rest is done after we received the file refs
			}
			else if (ret==miExport) {
				if (!node)
					node = theList;

				m_pCurrentNode = node;
				OpenExportFileDialog();
				// the rest is done after we received the file refs
			}			
			else if (ret==miCollapse) {
				if (collapsed)
					bv->Expand(node);
				else
					bv->Collapse(node);
				
				bv->Invalidate();				
			}
			else if (ret==miSort)
				bv->Sort(node);
		}
		else { // Items
			if (ret == miDup) {
				XmlNode* newnode = node->Duplicate();
				BString s(newnode->Attribute(OPML_TITLE));
				s << " " << _T("(copy)");
				newnode->AddAttribute(OPML_TITLE, s.String());
				newnode->SetText(s.String());
				node->Parent()->AddChild(newnode, node->Parent()->IndexOf(node)+1);
				bv->AddItem(newnode, bv->IndexOf(node)+1);
				bv->Invalidate();
			}
		}
	}
}


void
FrissPrefWin::EditItem(XmlNode* item)
{
	if (!item)
		return;

	BRect br( bv->ItemFrame(bv->IndexOf(item)));
	bv->ConvertToScreen(&br);
	
	FPrefEditWindow* pf = pf = new FPrefEditWindow(this, item, BPoint(br.left, br.top), !item->Marked());
	pf->Show();
}


void
FrissPrefWin::ReLabel()
{
	// Window
		SetTitle(	_T("Preferences") );
	
	// Tabs
		tabFeeds->SetLabel(	_T("Feeds") );
		tabColor->SetLabel(	_T("Colours") );
		tabMisc->SetLabel(	_T("Misc" ));
	
	// tabFeeds
		bAdd->SetLabel(	_T("New Item...") );
		bEdi->SetLabel(	_T("Edit") );
		bRem->SetLabel(	_T("Remove") );
	
	// tabColor
		bBack->SetLabel(			_T("Background") );
		cColTransparent->SetLabel(	_T("Transparent background") );
		cColDesktop->SetLabel(		_T("Adapt to Desktop background colour") );
		cColCustom->SetLabel(		_T("Custom background colour:") );
		bFore->SetLabel(			_T("Text Colour"));
		cColForeAdapt->SetLabel(	_T("Adapt colour to background")  );//(black on bright/white on dark)") );
		cColForeCustom->SetLabel(	_T("Custom text colour:") );
	
	// tabMisc
		tRefrAdv->SetLabel(		_T("Load next feed instead of current") );
		tRefresh->SetLabel(		_T("Refresh interval (min)") );
		mfScrollbar->SetLabel(	_T("Show scrollbar") );
		miAu->SetLabel(			_T("Automatic") );
		miOn->SetLabel(			_T("Show always") );
		miOf->SetLabel(			_T("Hide always") );
		
		boxBrowser->SetLabel(		_T("Browser") );
		cBrowserCustom->SetLabel(	_T("Custom") );
		tBrowserMime->SetLabel(		_T("Browser MIME-Type") );	
		
	// Force Redraw:
	tabView->Invalidate();
	UpdateIfNeeded();
}

