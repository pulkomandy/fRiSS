#include "fr_pref.h"

#include <assert.h>
#include <stdlib.h>

#include <FilePanel.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <Entry.h>
#include <Path.h>

#include "frissconfig.h"
#include "fr_fstringitem.h"


FrissPrefWin::FrissPrefWin(BView* thefv,FrissConfig* conf, XmlNode* xList,
		const char* Title)
	: BWindow(BRect(0, 0, 100, 100), Title, B_MODAL_WINDOW,
		B_FRAME_EVENTS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLook(B_TITLED_WINDOW_LOOK);
	SetFeel(B_MODAL_APP_WINDOW_FEEL);
	
	fv = thefv;		// "parent"
	config = conf;
	theList = xList;
	
	// selected item for editing:
	editi = -1;

	CenterOnScreen();
	
	m_pFileOpenPanel = m_pFileSavePanel = NULL;
	m_pCurrentNode = NULL;
	m_iImpExpMode = 0;
	
	mess = new BMessenger(this);
	
	// Background...
	SetLayout(new BGroupLayout(B_VERTICAL));
	BGroupView* bvx = new BGroupView(B_VERTICAL);
	bvx->GroupLayout()->SetInsets(B_USE_WINDOW_INSETS);
	bvx->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bvx);
	
	// TabView
	tabView = new BTabView("tabView", B_WIDTH_AS_USUAL);
	
	BGroupView *bbFeeds, *bbColor, *bbMisc;
	
	tabFeeds = new BTab();
	tabView->AddTab( bbFeeds = new BGroupView("bbFeeds", B_VERTICAL), tabFeeds );
	tabFeeds->SetLabel(_T("Feeds"));
	
	tabColor = new BTab();
	tabView->AddTab( bbColor = new BGroupView("bbColor", B_VERTICAL), tabColor );
	tabColor->SetLabel(_T("Colours"));
	
	tabMisc = new BTab();
	tabView->AddTab( bbMisc = new BGroupView("bbMisc", B_VERTICAL), tabMisc );
	tabMisc->SetLabel(_T("Misc"));
		
	bvx->AddChild(tabView);
	tabView->SetTabWidth(B_WIDTH_FROM_WIDEST);
	
	/* ------ Feeds ------ */
	{
		bv = new PrefListView(theList, "feedlist", B_SINGLE_SELECTION_LIST);
		bv->BuildView(theList);
	
		BScrollView *sv = new BScrollView("scrollview",bv,B_FRAME_EVENTS,false,true);
		BGroupLayoutBuilder(bbFeeds)
			.SetInsets(5, 5, 5, 5)
			.Add(sv)
			.AddGroup(B_HORIZONTAL)
				.Add( bAdd = new BButton("BAdd",_T("New Item..."),
					new BMessage(CMD_ADD_ITEM) ) )
				.Add( bEdi = new BButton("BEdi",_T("Edit"),
					new BMessage(CMD_EDIT_ITEM)) )
				.Add( bRem = new BButton("BRem",_T("Remove"),
					new BMessage(CMD_REMOVE_ITEM)) );
		
		// Remove and Edit have to be enabled by selecting an entry
		bAdd->SetEnabled(true);
		bEdi->SetEnabled(false);
		bRem->SetEnabled(false);
	}
	
	/* ----- Colour ----- */
	{
		bBack = new BBox("background");
		bBack->SetLabel(_T("Background"));

		BGroupView* group = new BGroupView(B_VERTICAL);
		bBack->AddChild(group);

		BGroupLayoutBuilder(group)
			.SetInsets(5, 5, 5, 5)
			.Add( cColTransparent = new BRadioButton("CBTransp", _T("Transparent background"/*" (replicants only)"*/), new BMessage( MSG_COL_CHANGED ) ) )
			.Add( cColDesktop = new BRadioButton("CBDesktop", _T("Adapt to Desktop background colour"), new BMessage( MSG_COL_CHANGED ) ) )
			.Add( cColDefault = new BRadioButton("CBDefault", _T("Use default colour"), new BMessage( MSG_COL_CHANGED ) ) )
			.Add( cColCustom = new BRadioButton("CBTransp", _T("Custom background colour:"), new BMessage( MSG_COL_CHANGED ) ) )
			.Add( cColor = new BColorControl(B_ORIGIN, B_CELLS_32x8, 7.0f, "colorpicker", new BMessage( MSG_COL_CHANGED ), true) );

		cColor->SetValue( conf->col );
		
		switch (conf->ColBackMode) {
		case ColBackTransparent:
			cColTransparent->SetValue(B_CONTROL_ON);
			break;

		case ColBackDesktop:
			cColDesktop->SetValue(B_CONTROL_ON);
			break;
		
		case ColBackDefault:
			cColDefault->SetValue(B_CONTROL_ON);
			break;
			
		default:
			cColCustom->SetValue(B_CONTROL_ON);
		}
		
#ifndef ALLOW_TRANSP
		cColTransparent->SetEnabled(false);
#endif
		
		/* ---- TEXT COLOR ---- */
		bFore = new BBox("foreground");
		bFore->SetLabel(_T("Text Colour"));

		group = new BGroupView(B_VERTICAL);
		bFore->AddChild(group);
		
		BGroupLayoutBuilder(group)
			.SetInsets(5, 5, 5, 5)
			.Add( cColForeAdapt = new BRadioButton("CFAdapt", _T("Adapt colour to background" /* "(black on bright/white on dark)"*/), new BMessage( MSG_COL_CHANGED ) ) )
			.Add( cColForeCustom = new BRadioButton("CFCustom", _T("Custom text colour:"), new BMessage( MSG_COL_CHANGED ) ) )
			.Add( cColorHigh = new BColorControl(B_ORIGIN, B_CELLS_32x8, 7.0f, "colorpicker_high", new BMessage( MSG_COL_CHANGED ), true) );

		cColorHigh->SetValue( conf->high );
		
		switch(conf->ColForeMode) {
		case ColForeAdapt:
			cColForeAdapt->SetValue(B_CONTROL_ON);
			break;
		default:
			cColForeCustom->SetValue(B_CONTROL_ON);
		}

		BGroupLayoutBuilder(bbColor)
			.SetInsets(5, 5, 5, 5)
			.Add(bBack)
			.Add(bFore)
			.AddGlue();
	}
	
	/* ----- Misc ----- */
	{
		bbMisc->GroupLayout()->SetInsets(5, 5, 5, 5);
		bbMisc->AddChild( tRefrAdv = new BCheckBox("tRefresh", _T("Load next feed instead of current"), NULL) );
		
		BString dummy;
		dummy << (int)(conf->RefreshRate / 120);
		bbMisc->AddChild( tRefresh = new BTextControl("tRefresh", _T("Refresh interval (min)"), dummy.String(), NULL) );
		
		if (config->RefreshAdvances == 1)
			tRefrAdv->SetValue(B_CONTROL_ON);

		/* --- */
#ifdef	OPTIONS_WINDOW_MODE

		BPopUpMenu* men = new BPopUpMenu(_T("WindowMode"));

		men->AddItem( miSimple = new BMenuItem( _T("Simple"),
			new BMessage( MSG_SB_CHANGED ) ) );
		men->AddItem( miPreview = new BMenuItem( _T("Preview"),
			new BMessage( MSG_SB_CHANGED ) ) );
		//men->AddItem( miFull = new BMenuItem( _T("Full"),
		//	new BMessage( MSG_SB_CHANGED ) ) );
		
		bbMisc->AddChild( mfWindowMode=new BMenuField("WindowMode",
			_T("Window mode"), men ) );
		
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
		
		
		boxBrowser = new BBox("browser");
		boxBrowser->SetLabel(_T("Browser"));
		BGroupView* group = new BGroupView(B_VERTICAL);

		boxBrowser->AddChild(group);

		BGroupLayoutBuilder(group)
			.SetInsets(5, 5, 5, 5)
			.Add( cBrowserNetP = new BRadioButton(
				"cBrowserNetP", _T("WebPositive"), NULL) )
			.Add( cBrowserFox = new BRadioButton(
				"cBrowserFox", _T("Mozilla Firefox"), NULL) )
			.Add( cBrowserCustom = new BRadioButton(
				"cBrowserCustom", _T("Custom"), NULL) )
			.Add( tBrowserMime = new BTextControl("tBrowser",
				_T("Browser MIME-Type"), config->BrowserMime.String(), NULL) );

		BGroupLayoutBuilder(bbMisc)
			.Add(boxBrowser)
			.AddGlue();
		
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
				// FPrefEditWindow has updated the item and we now have to
				// invalidate the list to reflect the changes
				
				bv->Invalidate();
			}
			break;
		
		case CMD_ADD_ITEM:
			// Changed behaviour: Add new XmlNode and open Edit for it
			{
				XmlNode* newItem = new XmlNode(theList, "outline");
				EditItem(newItem);
				bv->AddItem(new BStringItem(newItem->Name()));
				theList->AddChild(newItem);
				bv->FullListSortItems(&compare_func);
				break;
			}
			
			//Falltrough : edit the item we just added

		case CMD_EDIT_ITEM:
			{
				if (editi == bv->CurrentSelection()) {
					item = dynamic_cast<XmlNode*>(theList->ItemAt(editi));
					EditItem(item);
				}
			}
			break;

		case CMD_REMOVE_ITEM:
			if ((selected = bv->CurrentSelection())>=0) {
				item = dynamic_cast<XmlNode*>(theList->ItemAt(selected));
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
				XmlNode* node = dynamic_cast<XmlNode*>(bv->ItemAt(idx));
				ItemPopup(node, point);
			}
			break;	
		
		case MSG_COL_CHANGED:
			// Save Color Modes
			if (cColTransparent->Value() == B_CONTROL_ON)
				config->ColBackMode = ColBackTransparent;
			else if (cColDesktop->Value() == B_CONTROL_ON)
				config->ColBackMode = ColBackDesktop;
			else if (cColDefault->Value() == B_CONTROL_ON)
				config->ColBackMode = ColBackDefault;
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
			{
				entry_ref ref;
				BString name;
				msg->FindRef("directory", &ref);
				msg->FindString("name", &name);

				BDirectory dir(&ref);
				BEntry file(&dir, name.String());
				file.GetRef(&ref);

				Export(theList->Parent()->Parent(), &ref);
			}
			break;
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
				if ( type != B_REF_TYPE) {
					return;
				}
				
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
	// TODO : all of this should be checked while the user makes change, not on
	// closing the window...
	config->m_iAnz = bv->CountItems();

	// refresh rate	
	int min = atoi(tRefresh->Text());
	if (tRefrAdv->Value() == B_CONTROL_OFF && min < REFRESH_MIN_NORM) {
		char buffer[1000];
		sprintf(buffer, _T("The refresh rate is set to low.\n\n"
			"Minimum is %i minutes.\nRecommended: %i (~%i hours)"),
			REFRESH_MIN_NORM, REFRESH_REC_NORM, REFRESH_REC_NORM / 60);
		(new BAlert(_T("Warning"),buffer,_T("Ok")))->Go();
		return false;
	}
	if (tRefrAdv->Value() == B_CONTROL_ON && min < REFRESH_MIN_ADVC) {
		char buffer[1000];
		sprintf(buffer, _T("The feed advance rate is set to low.\n\n"
			"Minimum is %i minutes.\nRecommended: %i"), REFRESH_MIN_ADVC,
			REFRESH_REC_ADVC);
		(new BAlert(_T("Warning"),buffer,_T("Ok")))->Go();
		return false;
	}	
	
	// we simply cannot allow an empty list
	if (config->m_iAnz == 0) {
		(new BAlert(_T("Warning"),_T("Your feed list must not be empty!"),
			_T("Ok")))->Go();
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
	
	if (!m_pFileOpenPanel) {
		m_pFileOpenPanel = new BFilePanel(B_OPEN_PANEL, mess, NULL, 0, false,
			NULL, NULL, true, true);
	}
		
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
	
	XmlNode* loader = new XmlNode(NULL, "");
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
	
	if (!m_pFileSavePanel) {
		m_pFileSavePanel = new BFilePanel(B_SAVE_PANEL, mess, NULL, 0, false,
			NULL, NULL, true, true);
	}

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
	BMenuItem *miCollapse = NULL, *miRemove = NULL, *miEdit = NULL,
		*miAddSubfolder = NULL;
	BMenuItem *miDup = NULL, *miAddItem = NULL, *miExport = NULL,
		*miImport = NULL;
	BMenuItem *miSort = NULL;

	if (node) {
		int idx = theList->IndexOf(node);
		bv->Select(idx, false);
		BStringItem* item = dynamic_cast<BStringItem*>(bv->ItemAt(idx));
		
		if (node->Attribute(OPML_URL)==NULL) {
			folder = true;
			
			if (!item->IsExpanded()) {
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
		//popup->AddItem( miAddSubfolder = new BMenuItem( _T("Add new Folder..."), NULL ) );
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
	
	int idx = theList->IndexOf(node);

	if (ret == miEdit)
		EditItem(node);
	else if (ret == miRemove) {
		bv->RemoveItem(idx);
		node->Parent()->RemoveChild(node);
		bv->Invalidate();
	}
	else {
		if (folder) {
			if (ret == miAddItem) {
				XmlNode* newnode;
				
				if (node) {
					newnode = new XmlNode(node->Parent(), "outline");
					node->AddChild(newnode,0);
					bv->AddUnder(bv->FullListItemAt(idx), new BStringItem(newnode->Name()));
				}
				else {
					newnode = new XmlNode(theList, "outline");
					theList->AddChild(newnode);
					bv->AddItem(new BStringItem(newnode->Name()));
				}

				newnode->AddAttribute(OPML_TITLE, _T("New Item"));
				newnode->AddAttribute(OPML_URL, "http://");
				
				bv->Select(theList->IndexOf(newnode),false);
				
				bv->Invalidate();
				
				EditItem(newnode);
			}
			else if (ret==miAddSubfolder) {
				XmlNode* newnode;
				
				if (node) {
					newnode = new XmlNode(node->Parent(), "outline");
					node->Parent()->AddChild(newnode, node->Parent()->IndexOf(node)+1);
					bv->AddItem(new BStringItem(newnode->Name()),
						theList->IndexOf(node)+bv->CountItemsUnder(
							bv->FullListItemAt(idx), false)+1);
				}
				else {
					newnode = new XmlNode(theList, "outline");
					theList->AddChild(newnode);
					bv->AddItem(new BStringItem(newnode->Name()));
				}

				newnode->AddAttribute(OPML_TITLE, _T("New Folder"));
				bv->Select(theList->IndexOf(newnode),false);
				
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
					bv->Expand(bv->FullListItemAt(idx));
				else
					bv->Collapse(bv->FullListItemAt(idx));
				
				bv->Invalidate();				
			}
			else if (ret==miSort)
				bv->Sort(bv->FullListItemAt(idx));
		}
		else { // Items
			if (ret == miDup) {
				XmlNode* newnode = new XmlNode(*node);
				BString s(newnode->Attribute(OPML_TITLE));
				s << " " << _T("(copy)");
				newnode->AddAttribute(OPML_TITLE, s.String());
				node->Parent()->AddChild(newnode, node->Parent()->IndexOf(node)+1);
				bv->AddItem(new BStringItem(newnode->Name()), idx+1);
				bv->Invalidate();
			}
		}
	}
}


void
FrissPrefWin::EditItem(XmlNode* item)
{
	assert(item != NULL);
	(new FPrefEditWindow(this, item, Frame().LeftTop()))->Show();
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

