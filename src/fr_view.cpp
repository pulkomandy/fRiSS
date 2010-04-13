#include "fr_view.h"
#include "load.h"
#include "parser.h"
#include <time.h>
#include <be/StorageKit.h>

// Helper window informs us about workspaces changes etc:
#ifdef OPTIONS_USE_HELPERWINDOW
	#include "fr_helperwindow.inc"
#endif


#include <stdlib.h>


FrissView::FrissView(FrissConfig* newconf, XmlNode* x_root, BRect frame) :
	BBox(frame, "fRiSS", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS, B_FANCY_BORDER)
{
	// Important
	config = newconf;
	theRoot = x_root;
	theList = theRoot->FindChild("body", NULL, true);
	
	screen = NULL;
	
	if ((mutex_loader = create_sem(1, "MutexLoader")) < B_OK) {
		puts("Error: Could not create semaphore");
	}
	
	/* Nun noch den Dragger einbauen.
	 * B_ORIGIN scheint unten rechts zu sein, also noch entsprechend
	 * die linke obere Ecke setzen (ich wusste vorher auch nicht, dass
	 * der Dragger 7x7 Pixel groß ist :-)
	 */
	BRect r = Bounds();
	r.OffsetTo(B_ORIGIN);
	r.top = r.bottom-7;
	r.left = r.right - 7;		
	myd = new BDragger( r, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW );
	AddChild(myd);
	
	// Only transparency for replicants:
	replicant = false;	
	
	// rest of initialisation will happen in AllAttached()
	
	#if (__ZETA || OPTIONS_USE_NLANG)
	entry_ref ref; 
	if ( B_OK == be_roster->FindApp(APP_SIGNATURE, &ref) ) {
		BPath path(&ref);
		BString appname = path.Leaf();
		path.GetParent(&path);
		path.Append("Language/Dictionaries");
		#ifdef __ZETA__
			path.Append(appname.String());
			be_locale.LoadLanguageFile(path.Path());
		#else
			strDebug = config->Lang;
			
			no_locale.Init(path.Path());
			if (config->Lang.Length()>0)
				no_locale.LoadFileID(config->Lang.String());
		#endif
	}
	else 
		strDebug = "Path not found";
	#else
		strDebug = "No language support";
	#endif	
}


FrissView::FrissView(BMessage *archive) :
	BBox(archive)
{
	myd = NULL;
	replicant = true;
	
	// 
	screen = NULL;


	if ((mutex_loader = create_sem(1, "MutexLoader")) < B_OK) {
		puts("Error: Could not create semaphore");
	}
	
	BMessage msg;
	archive->FindMessage("frissconfig", &msg);
	
	config = new FrissConfig(&msg);


	// Reassemble feed list:	
	BMessage list;
	archive->FindMessage("feeds", &list);
	theRoot = new XmlNode(&list);
	theList = theRoot->FindChild("body", NULL, true);

	
	#if (__ZETA || OPTIONS_USE_NLANG)
	entry_ref ref; 
	if ( B_OK == be_roster->FindApp(APP_SIGNATURE, &ref) ) {
		BPath path(&ref);
		BString appname = path.Leaf();
		path.GetParent(&path);
		path.Append("Language/Dictionaries");
		#ifdef __ZETA__
			path.Append(appname.String());
			be_locale.LoadLanguageFile(path.Path());
		#else
			strDebug = config->Lang;

			no_locale.Init(path.Path());
			
			if (config->Lang.Length()>0)
				no_locale.LoadFileID(config->Lang.String());
		#endif
	}
	else 
		strDebug = "Path not found";
	#else
		strDebug = "No language support";
	#endif
	
	// don't resize any more (this sucks in Shelfer)
	SetResizingMode(ResizingMode() & !B_FOLLOW_RIGHT & !B_FOLLOW_BOTTOM);	
	
	BRect r = Bounds();
	r.OffsetTo(B_ORIGIN);
	r.top = r.bottom-7;
	r.left = r.right - 7;		
	myd = new BDragger( r, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
	AddChild(myd);		
}

FrissView::~FrissView()
{
	free(buf);		buf = NULL;
	delete tlist;	tlist = NULL;
	delete screen;	screen = NULL;
	
	//Window()->PostMessage(B_QUIT_REQUESTED, feedloader);
	BMessenger(feedloader).SendMessage(B_QUIT_REQUESTED);
}


void Notify(const char* msg)
{
	BAlert *alert = new BAlert( "Vorsicht!", msg, "Mist!" );
	alert->SetShortcut(0, B_ESCAPE);
	alert->ResizeTo(400,300);
	alert->Go();
}

status_t
FrissView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, false);
	data->AddString("add_on", app_signature);
	
	if (deep) {
		// manually add the dragger
		
	
		// save configuration
		BMessage msg;
		if (config->Archive(&msg, deep) == B_OK)
			data->AddMessage("frissconfig", &msg);
		else {
			Notify("FrissView::Archive() : ERROR - Could not archive config");
			return B_ERROR;
		}
		
		// save feed tree
		BMessage list;
		if (theRoot->Archive(&list, deep) == B_OK)
			data->AddMessage("feeds", &list);
		else {
			Notify("FrissView::Archive() : ERROR - Could not archive feeds");
			return B_ERROR;			
		}
	}
		
	return B_OK;
}

BArchivable *
FrissView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "FrissView"))
		return NULL;
	return new FrissView(data);
}


void
FrissView::AllAttached()
{
	BBox::AllAttached();
	
	

	status_t stat;
	messenger = new BMessenger(this, NULL, &stat);
	
	BEntry entry("/boot/home/config/settings/NetPositive/History");
	node_ref nref;
	entry.GetNodeRef(&nref);
	
	watch_node(&nref, B_WATCH_ALL, *messenger);
	
	
	// Screen is useful for getting workspace color etc.	
	screen = new BScreen(B_MAIN_SCREEN_ID);
	if (replicant) {
	}

	// Pulse/Reload timer:
	last_reload = 0;
	pulses = 0;	
	pulsing = true;
	inv = false;
	
	currentFeed = NULL;
	
	// "Global" Buffer for data and items:
	buf = (char*)calloc(BUFSIZE,1);
	tlist = new BList();
	
	// Feedloader loads files from disk and net
	feedloader = new FrFeedLoader(this, buf);
	feedid = feedloader->Run();

	// Views:
	pop = NULL;
	
	
	BRect br = Bounds();
	br.InsetBy(10,10);
	br.top += 5;
	br.right -= 1; //B_V_SCROLL_BAR_WIDTH;
	listview = new FListView(this,br,"listview",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL_SIDES);
	AddChild(listview);

	BRect sbr = Bounds();
	sbr.InsetBy(10,10);
	sbr.top += 5;	
	sbr.left = sbr.right - B_V_SCROLL_BAR_WIDTH;
	sb = new BScrollBar(sbr, "scrollbar", listview, 0, 100, B_VERTICAL);
	AddChild(sb);
	sb_hidden = true;
	sb->Hide();
	
	listview->Hide();
		
	
	BRect s(0,0,br.Width()-1,br.Height()-1);
	tvTextView = new FTextView(this, br,"stview",s,B_FOLLOW_ALL_SIDES,B_WILL_DRAW | B_FRAME_EVENTS);
	tvTextView->MakeSelectable(false);
	//AddChild(tvTextView);
	
	sbTextView = new BScrollView("sbTextView", tvTextView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_PLAIN_BORDER);
	AddChild(sbTextView);
	
	sbTextView->Hide();
	//ShowPreviewArea(false);
	
	
	
	// Hack/Workspace change notifying helper window:
	#ifdef OPTIONS_USE_HELPERWINDOW
		MyDummyWindow *x = new MyDummyWindow(this);
		x->Show();
		x->Hide();
	#endif
	
	
	Load(config->Index());
	
	
	#ifdef OPTIONS_USE_NLANG
		if (config->Lang.Length() != 0)
			no_locale.LoadFileID(config->Lang.String());
	#endif
	
	BString ts;	
	
	pop = new BPopUpMenu("popup",false,false,B_ITEMS_IN_COLUMN);
	
	pop->AddItem( miGo = new BMenuItem( _T("Go"), NULL ) );	
	
	ts << _T("Information") << "...";
	pop->AddItem( miInfo = new BMenuItem( ts.String(), NULL ) );
	//miInfo->SetShortcut('I', B_COMMAND_KEY);
	miInfo->SetEnabled(false);
	pop->AddSeparatorItem();
	pop->AddItem( miRefr = new BMenuItem( _T("Refresh"), NULL /*,'R', B_CONTROL_KEY*/ ) );
	//miRefr->SetShortcut('R', B_COMMAND_KEY);
	pop->AddSeparatorItem();
	pop->AddItem( miNext = new BMenuItem( _T("Next Feed"), NULL /*, 'N', B_CONTROL_KEY*/ ) );
	//miNext->SetShortcut('N', B_COMMAND_KEY);
	pop->AddItem( miPrev = new BMenuItem( _T("Previous Feed"), NULL /*, 'N', B_CONTROL_KEY | B_SHIFT_KEY*/ ) );
	//miPrev->SetShortcut('R', B_COMMAND_KEY);
	
	pop->AddItem( mList = new BMenu( _T("Select Feed") ) );
	pop->AddSeparatorItem();
	
	ts = _T("Preferences"); ts << "...";
	pop->AddItem( miOpts = new BMenuItem( ts.String(), NULL /*,'P', B_CONTROL_KEY*/ ) );
	#ifdef OPTIONS_USE_NLANG
		pop->AddItem( mLang = new BMenu( _T("Language") ) );
		no_locale.BuildLangMenu(mLang, config->Lang.String());
	#endif	
	pop->AddSeparatorItem();
	BString version(_T("About"));
	version << " " << VERSION_;
	pop->AddItem( miAbout = new BMenuItem( version.String(), NULL /*,'A', B_CONTROL_KEY*/ ) );
#ifdef N3S_DEBUG
	if (replicant) {
		pop->AddSeparatorItem();
		pop->AddItem( miDebug = new BMenuItem( _T("Debug: Remove Replicant"), NULL /*,'Q', B_CONTROL_KEY*/ ) );
	}
	else {
		pop->AddSeparatorItem();
		pop->AddItem( miDebug = new BMenuItem( _T("Debug: Nuthing"), NULL /*,'Q', B_CONTROL_KEY*/ ) );
	}
#else
	if (replicant) {
		pop->AddSeparatorItem();
		pop->AddItem( miDebug = new BMenuItem( _T("Debug: Remove Replicant"), NULL /*,'Q', B_CONTROL_KEY*/ ) );
	}
	else
		miDebug = NULL;
#endif

	config->m_iAnz = BuildPopup(theList, mList);
	
	UpdateColors();
	currentWindowMode = WindowMode_NONE;
	UpdateWindowMode();
}

void
FrissView::AttachedToWindow()
{
	BBox::AttachedToWindow();
	#ifdef __ZETA__
		//fNotify.SetTo((BView*)this);
	#endif
}

void
FrissView::MouseDown(BPoint point)
{
	if (!pop)
		return;

	BPoint	cursor;
	unsigned long	buttons;
	
	GetMouse(&cursor,&buttons);
	
	if (buttons & 0x2) {
		// Rechte Maustaste
		ConvertToScreen(&point);
		StartPopup(point);
	}
}

void
FrissView::UpdateColors()
{	
	rgb_color colback, collow, colfore;
	
	if (replicant && config->ColBackMode == ColBackTransparent) {
		listview->transparent = true;	
		//colback = config.col;
		//colback.alpha = 255
		
		colback = B_TRANSPARENT_COLOR;
		collow = screen->DesktopColor();
	}
	else if (config->ColBackMode == ColBackDesktop) {
		colback = screen->DesktopColor();
		collow = colback;
	}
	else {
		colback = config->col;
		collow = colback;
	}
	
	if (config->ColForeMode == ColForeAdapt) {
		float len = sqrt( collow.red * collow.red + collow.green * collow.green + collow.blue * collow.blue);
		
		if (len < 127.0) {
			colfore.red = 255;
			colfore.green = 255;
			colfore.blue = 255;			
		}
		else {
			colfore.red = 0;
			colfore.green = 0;
			colfore.blue = 0;
		}		
		colfore.alpha = 0;
	}
	else {
		colfore = config->high;	
	}
	
	
	SetViewColor(colback);	// View background
	SetHighColor(colfore);	// Textcolor
	SetLowColor(collow);	// Text background =!= ViewColor

	listview->SetViewColor(colback);	//
	listview->SetHighColor(colfore);	// Text
	listview->SetLowColor(collow);		// Textbackground
	
	tvTextView->SetViewColor(colback);
	//tvTextView->SetHighColor(colfore);
	tvTextView->SetLowColor(collow);
	
	tvTextView->SetFontAndColor( be_plain_font, B_FONT_ALL, &colfore );

	//puts("Invalidate?");		

	Invalidate();
	if (!listview->IsHidden())
		listview->Invalidate();
	
	if (!sbTextView->IsHidden())
		tvTextView->Invalidate();

	if (config->ColBackMode == ColBackTransparent) {
		Window()->UpdateIfNeeded();
	}
}

void
FrissView::MessageReceived(BMessage *msg)
{
	int32		opcode;
	//dev_t		device;
	//ino_t		directory;
	//ino_t		node;
	const char*	name;
	entry_ref	ref;
	
	switch(msg->what) {
		case MSG_PREF_DONE:
			// signaled by FrissPrefWindow, saying it's done
			//inv = true;

			theRoot->SaveToFile( config->Feedlist.String() );
			pulsing = true;
			
			UpdateColors();
			ReBuildPopup(theList, mList);		
			break;
			
		case MSG_LOAD_DONE:
			LoadDone();
			break;
			
		case MSG_LOAD_FAIL:
			pulsing = true;
			pulses = 0;
			Error(_T("An error occured while loading this feed."));
			break;
			
		case MSG_COL_CHANGED:
			UpdateColors();
			UpdateScrollbar();
			break;

		case MSG_SB_CHANGED:
			UpdateWindowMode();
			UpdateScrollbar();
			break;			
		
		case MSG_WORKSPACE:
			//puts("DUMMY DUMMY!");
		case B_SCREEN_CHANGED:
		case B_WORKSPACES_CHANGED:
		case B_WORKSPACE_ACTIVATED:
			OnWorkspaceChanged();
			break;

		case B_LANGUAGE_CHANGED:
			{
				ReBuildPopup(theList, mList);
				BString ts;

				miGo->SetLabel( _T("Go") );
				ts = _T("Information"); ts << "...";
				miInfo->SetLabel( ts.String() );
				miRefr->SetLabel( _T("Refresh") );
				miNext->SetLabel( _T("Next Feed") );
				miPrev->SetLabel( _T("Previous Feed") );
				pop->ItemAt(6)->SetLabel(_T("Select Feed"));
				//mList->SetLabel( _T("Select Feed") );
				ts = _T("Preferences"); ts << "...";
				miOpts->SetLabel( _T(ts.String()) );
				#ifdef OPTIONS_USE_NLANG
				pop->ItemAt(9)->SetLabel( _T("Language") );
				#endif
				BString version(_T("About"));
				version << " " << VERSION_;
				miAbout->SetLabel( version.String() );
			}
			break;

		case B_NODE_MONITOR:
			if (msg->FindInt32("opcode", &opcode) == B_OK) {
				switch(opcode) {
				case B_ENTRY_CREATED:
					{
						//puts("Create");
						msg->FindInt32("device", &ref.device);
						msg->FindInt64("directory", &ref.directory);
						//msg->FindInt64("node", &node);
						msg->FindString("name", &name);
						ref.set_name(name);

						VisitRef(&ref);
					}
					break;
				
				case B_ENTRY_MOVED:
					{
						//puts("Moved");
						msg->FindInt32("device", &ref.device);
						msg->FindInt64("to directory", &ref.directory);
						//msg->FindInt64("node", &node);
						msg->FindString("name", &name);
						ref.set_name(name);
						
						VisitRef(&ref);
					}					
					break;
				
				default:
					// andere Nachricht
					//puts("Else");
					break;
				}
			}
			break;
			
		case CmdLoad:
			{
			}
			break;
			
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		
		default:
			BBox::MessageReceived(msg);
	}
}

void
FrissView::Pulse()
{
	if ((!pulsing) && (!inv))
		return;

	pulses++;
	
	//printf("PULSE %i / %i\n",pulses, config.RefreshRate);
	
	if (inv==true || pulses >= config->RefreshRate) {
		if (inv==false && config->RefreshAdvances == 1) {
			config->m_iIndex++;
		}
		inv = false;
	
		//puts("Time's up! Reloading...");
		Load(config->Index());
		pulses = 0;
		pulsing = true;
	}
	
	// check for workspace change
	#ifndef OPTIONS_USE_HELPERWINDOW
		static int workspace = -1;
		const int cw = current_workspace();
		if (workspace != cw) {
			workspace = cw;
			OnWorkspaceChanged();
			//messenger->SendMessage(MSG_WORKSPACE);
		}
	#endif
}

XmlNode*
FrissView::FindItem(int index, int& is, XmlNode* node)
{
	int anz = node->Children();
	
	for (int i=0; i<anz; i++) {
		XmlNode* c = node->ItemAt(i);

		if (c->Children()>0) {
			//puts("  recursing");
			XmlNode* d = FindItem(index, is, c);
			if (d)
				return d;
		}
		else {
			if (index == is)
				return c;
			
			is++;
		}
	}
	
	return NULL;
}

void
FrissView::Load(int idx, XmlNode* direct)
{
	if (!theList) {
		Error(_T("Error: we do not have a list!"));
		return;
	}
	if (theList->Children() == 0) {
		Error(_T("Error: Please add some feeds in the Preferences Window!"));
		return;
	}
	
	// Load
	pulsing = false;
	
	//printf("Suche Item #%d\n", idx);


	int x = 0;
	XmlNode* fi;
	
	if (!direct) {
		if (idx<0)
			idx = config->m_iAnz - 1;
		else if (idx >= config->m_iAnz)
			idx = 0;

		fi = FindItem(idx, x, theList);
		if (!fi) {
			x = 0;
			fi = FindItem(0,x,theList);
			idx = 0;
		}
		if (!fi) {
			Error("DEBUG: FV::Load() fails because a NULL pointer is detected");
			return;
		}
	}
	else {
		fi = direct;
	}
	
	config->SetIndex(idx);
	
	//printf("Loading %d / %d\n", config.index, config.anz);

	// Get some mem
	int bufsize = BUFSIZE;
	
	BString title(fi->Attribute("text"));
	BString url(fi->Attribute("xmlURL"));
	
	SetLabel(title.String());


	// run external program
	if (url.Compare("run://", 6)==0) {
		if (!listview->IsHidden())
			listview->Hide();
			
		if (config->WindowMode == WindowModeSimple)
			ShowPreviewArea(true);
		
		url.Remove(0,6);
		
		memset( buf, 0, bufsize );
			
		if ( LoadFile(buf, bufsize, url.String()) > 0 )
			tvTextView->SetText( buf );
		else {
			BString text(_T("Error: Could not execute program."));
			text << "\n" << url.String();
			Error( text.String() );
		}

		currentFeed = fi;

		// Necessary?
		pulses = 0;
		pulsing = true;
		
		Invalidate();
		tvTextView->Invalidate();
			
		return;	
	}
	
	// load from net
	if (url.Compare("http://", 7)==0 || url.Compare("file://", 7)==0) {
		if (feedloader->Busy()) {
			Error(_T("Warning: FeedLoader is still busy..."));
			return;
		}
		
		currentFeed = fi;

		if (!listview->IsHidden())
			listview->Hide();
		
		if (config->WindowMode == WindowModeSimple)
			ShowPreviewArea(true);
		
		tvTextView->SetText(_T("Loading..."));

		// Liste leeren:
		int anz = listview->CountItems();
		for (int32 i = 0; i < anz; i++ )
			delete listview->ItemAt(i);
		listview->MakeEmpty();
		
		pulsing = false;

		memset( buf, 0, bufsize );
		
		if (acquire_sem(mutex_loader) == B_NO_ERROR) {
			feedloader->sUrl = url;
			//feedloader->PostMessage('LOAD');
			BMessenger(feedloader).SendMessage('LOAD');
			
			release_sem(mutex_loader);
		}
		else {
			puts("Error: could not enter mutex!");
		}
		
		Invalidate();
		tvTextView->Invalidate();
		return;
	}
	
	// unsupported protocol
	BString errf;
	errf = "Unsupported protocol: ";
	errf << url.String();
	errf << "\n\nValid protocols are:\n\thttp://\n\tfile://\n\trun://\n" ;

	puts( errf.String() );
	Error( errf.String() );
}

void
FrissView::LoadNext()
{
	Load(config->Index()+1);
}

void
FrissView::LoadPrev()
{
	Load(config->Index()-1);
}


void
FrissView::LoadDone()
{
	BString	status;
	int		anz = 0;
	bool	addDesc = false;
	bool	updateFeedList = false;
	BString currentType;

	if (!currentFeed) {
		Error("Kein CurrentFeed?!");
		return;
	}
	
	if (*buf == 0) {
		status = _T("Error: Feed is empty");
		anz = -1;
	} else {
	
		addDesc = (currentFeed->Attribute("x-addDesc") != NULL);
		currentType = currentFeed->Attribute("xmlURL");
		currentType.Remove(0, currentType.FindLast('.'));
		
		if (currentType.ICompare(".ICS")==0) {
			// iCal / Sunbird ICS
			#ifndef OPTIONS_NO_ICS
				anz = Parse_ics(buf, tlist, status, updateFeedList);	
			#else
				anz = -1;
				status = _T("ICS is not supported in this build");
			#endif
		}
		else {
			// default is RSS/RDF/XML
	
			// build tree from loaded file:
			XmlNode* root = new XmlNode(buf, NULL);
				
			XmlNode* x = root->FindChild("channel",NULL,true);
			if (x) {
				// assume it's RSS or RDF Xml style
				
				anz = Parse_rss(root, tlist, status, updateFeedList, addDesc);
			}
			else {
				x = root->FindChild("feed");
				
				if (x) {
					#ifndef OPTIONS_NO_ATOM
						anz = Parse_atom(root, tlist, status, updateFeedList, addDesc);	
					#else
						anz = -1;
						status = _T("ATOM is not supported in this build");
					#endif		
				}
				else {
					// unknown XML type
					status = _T("Error: Unrecognized format");
					anz = -1;
				}
			}					
		}
	}
	//printf("\nNACH DEM PARSEN: anz=%i Status='%s'\n\n", anz, status.String() );
	
	if (anz<=0 || tlist->CountItems()==0) {
		Error(status.String());
		
		pulses = 0;
		pulsing = true;		
		Invalidate();
		return;
	}
	//puts("x2");

	// Add current time to Box-label
	if (1) {
		// get current time:
		time_t now = time(NULL);
		tm* localtm = localtime( &now );
		
		BString title(currentFeed->Attribute("text"));
		
		title << " (" << localtm->tm_hour << ":";
		if (localtm->tm_min<10) title << "0";
		title << localtm->tm_min << ")";
		
		SetLabel( title.String() );	
	}	
	
	if (updateFeedList) {
		/*FStringItem* fi =*/ (FStringItem*)tlist->RemoveItem((int32)0);
		// Update 
		// well, can't do that in OPML...
		/*
		XmlNode* li = (FStringItem*)config.list->ItemAt(config.index);
		//printf("LoadDone:\n\tName:\t%s\n\tUrl:\t%s\n\tDesc:\t%s\n\n", fi->Text(), fi->Url(), fi->Desc() );
		li->SetDesc( fi->Desc() );
		delete fi;
		*/
	}
		
	tvTextView->SetText("");
	
	listview->MakeEmpty();
	int a = tlist->CountItems();
	for (int i=0;i<a;i++) {
		listview->AddItem( (BListItem*)tlist->ItemAt(i) );
	}	
	//listview->AddList(tlist);
	
	// set visited:
	InitialVisitedLink("/boot/home/config/settings/NetPositive/History");

	if (config->WindowMode == WindowModeSimple)
		ShowPreviewArea(false);
	
	if (listview->IsHidden())
		listview->Show();
		
	tlist->MakeEmpty();
	
	// Necessary?
	pulses = 0;
	pulsing = true;
	
	UpdateScrollbar();
	Invalidate();
	listview->Invalidate();
}


void
FrissView::ReBuildPopup(XmlNode *node, BMenu* menu)
{
	//puts("Rebuilding the Feed popup...");
	//puts("   del");
	DeletePopup(menu);
	
	//puts("   build");
	if (node)
		config->m_iAnz = BuildPopup(node, menu, 0) + 1;
		
	//puts("   ok");
}

void
FrissView::DeletePopup(BMenu* menu)
{
	int anz = menu->CountItems();
	
	for (int i=0;i<anz;i++) {
		BMenuItem* mi = menu->ItemAt(0);
		BMenu* sub = mi->Submenu();
		if (sub) {
			DeletePopup(sub);
			menu->RemoveItem(sub);
			delete sub;
		}	
		else {
			menu->RemoveItem(mi);
			delete mi;
		}
	}	
}

int
FrissView::BuildPopup(XmlNode *node, BMenu* menu, int nr)
{
	if (!node)
		return nr;
		
	miActiveFeed = NULL;

	int anz = node->Children();
	
	menu->SetRadioMode(false);

	if (anz>0) {
		for (int i=0;i<anz;i++) {
			XmlNode* c = (XmlNode*)node->ItemAt(i);
			const char* t = c->Attribute("text");
			
			if (c->Children()>0) {
				BMenu*  sub = new BMenu( _T( t ) );
				menu->AddItem(sub);

				nr = BuildPopup(c, sub, nr);
			}
			else {
				BMessage* msg = new BMessage(CmdLoad);
				msg->AddInt32("node", (int)c);
				msg->AddInt32("nr", nr);
				menu->AddItem( new BMenuItem( t, msg ) );
				
				nr++;
			}
		}
	}
	else {
		puts("Tja...");
		// node->Attribute("xmlURL")
	}
	
	return nr;
}

void
FrissView::StartPopup(BPoint point)
{
	#ifdef N3S_DEBUG
		BString l(miDebug->Label());
		l << " " << strDebug.String();
		
		miDebug->SetLabel(l.String());
	#endif

	miGo->SetEnabled(false);
	miInfo->SetEnabled(false);
	int idx = -1;
	if (!listview->IsHidden()) {
		BPoint p(point);
		ConvertFromScreen(&p);
				
		if (listview->Frame().Contains(p)) {
			p = point;
			listview->ConvertFromScreen(&p);
			idx = listview->IndexOf(p);
			//printf("Index ist %d\n", idx);
		}
		else {
			// maybe inside the text field?	
			if ((!sbTextView->IsHidden()) && (sbTextView->Frame().Contains(p)))
				idx = listview->CurrentSelection();
		}
		
		if (idx>=0) {
			miGo->SetEnabled(true);
			miInfo->SetEnabled(true);
		}
	}

	point.x -= 5;
	point.y -= 5;		
	BMenuItem* mi = pop->Go(point);
	
	if (!mi)
		return;

	if (miGo!=NULL && mi==miGo) {
		NodeLaunch( (FStringItem*)listview->ItemAt(idx) );
	}
	else if (mi == miInfo) {
		NodeViewInformation( (FStringItem*)listview->ItemAt(idx) );
	}	
	else if (mi == miNext)
		LoadNext();
	else if (mi == miPrev)
		LoadPrev();
	else if (mi == miRefr)
		Load(config->Index());
	else if (mi == miOpts) {
		inv = false;
		pulsing = false;
		FrissPrefWin* pf = new FrissPrefWin(this, config, theList, BRect(0,0,0,0), _T("Preferences"));
		pf->Show();
		// now we're "blocked" until the pref window sends 'PREF' to us
	}
	else if (mi == miAbout) {
		AboutRequested();
	}
//#//ifdef N3S_DEBUG	
	else if (miDebug!=NULL && mi == miDebug) {
		Parent()->RemoveChild(this);		
	}
//#//endif
	else if (mi) {
		// perhaps something from the feed list?
	
		BMessage *msg = mi->Message();
		if (msg) {
			int32 idx;
			XmlNode *node = NULL;
			if ((msg->FindInt32("nr", (int32*)&idx)==B_OK)
			&& (msg->FindInt32("node", (int32*)&node)==B_OK)) {
				if (node) {
					//printf("Popup: found node #%d (%s)\n", idx, node->Attribute("text"));
					
					if (miActiveFeed)
						miActiveFeed->SetMarked(false);
					miActiveFeed = mi;
						
					mi->SetMarked(true);
					Load( idx, node );
				}
			}
			else {
				#ifdef OPTIONS_USE_NLANG
				BString file;
				if (msg->FindString("filename", &file) == B_OK) {
					printf("Trying to load language file '%s'\n", file.String());
					no_locale.LoadFile(file.String());
					file.Remove(0,file.Length()-4);
					config->Lang = file.String();
					DeletePopup(mLang);
					no_locale.BuildLangMenu(mLang, file.String());
					BMessenger(this).SendMessage(B_LANGUAGE_CHANGED);
				}		
				#endif
			}
		}
	}
}

void
FrissView::ItemSelected(FStringItem* fi)
{
	if (!fi)
		return;
		
	if (config->WindowMode == WindowModeSimple)
		Launch(fi);
	else {
		NodeViewInformation(fi);
	}
}

void
FrissView::Launch(FStringItem* fi)
{
	if (!fi) {
		Error("DEBUG: FV::Launch() fails because a NULL pointer is detected");
		return;
	}

	char *argv[2];
	
	BString app;
	
	int BrowserType = config->BrowserType;
	
	//
	if (currentFeed) {
		const char* bb = currentFeed->Attribute(OPML_OVERRIDE_BROWSER);
		if (bb)
			BrowserType = atoi(bb);
	}
	
	switch (BrowserType) {
		case BrowserCustom:
			app = config->BrowserMime;
			break;
		
		case BrowserFirefox:
			app = BROWSER_MIME_FIREFOX;
			break;
	
		case BrowserNetpositive:
		default:
			app = BROWSER_MIME_NETPOSITIVE;
	}
	
	BString url(fi->Url());
	argv[0] = (char*)url.String();
	argv[1] = 0;
	
	printf("URL ist '%s'\n", argv[0]);
	
	status_t status = be_roster->Launch( app.String(), 1, argv );
	//printf("Result is: %i\n", (int)status);
	
	if (status == B_OK || status == B_ALREADY_RUNNING) {
		// seems to be fine
		
		// mark item as visited:
		//fi->SetVisited();
		//listview->Invalidate();
		listview->DeselectAll();
				
		// add URL to visitedList:
		// TODO:
	}
	else {
		(new BAlert( _T("Error"), _T("Error: The browser could not be started :-("), _T("Ok") ))->Go();
	}
}

void
FrissView::Error(const char* err)
{
	if (!listview->IsHidden())
		listview->Hide();
	ShowPreviewArea();
	
	// Dump to console
	printf("%s\n",err);

	tvTextView->SetText(err);
	tvTextView->Invalidate();	
}

void
FrissView::Draw(BRect frame)
{
	//puts("Draw()");

#ifdef ALLOW_TRANSP
	if (replicant && config.ColBackMode == ColBackTransparent) {
		//SetDrawingMode(B_OP_ALPHA);
		if (bitmap)
			DrawBitmap(bitmap, Bounds());
		
		BBox::Draw(frame);
	}
	else
#endif	
	{
		// Ganz normal:
		BBox::Draw(frame);
	}	

//	if (myd)
//		myd->Invalidate();
}

void
FrissView::FrameResized(float width, float height)
{
	BBox::FrameResized(width,height);
	
	UpdateWindowMode();
	UpdateScrollbar();
}


void
FrissView::UpdateWindowMode()
{
/*
	if (replicant)
		return;
*/

/*
	if (currentWindowMode == config->WindowMode)
		return;
*/
				
#ifdef OPTIONS_WINDOW_MODE
	BRect br = Bounds();

	/*
	if (config->WindowMode == WindowModeFull) {
		// do stuff
		currentWindowMode = WindowModeFull;
		return;
	}
	*/	
	
	if (config->WindowMode == WindowModePreview) {
		sbTextView->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT );
		listview->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT );
		sb->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT );	

		// max visible area:
		br.InsetBy(10,10);
		br.top += 5;
		br.right -= 1; //B_V_SCROLL_BAR_WIDTH;    x1x

		float height = br.Height() / 2 - 5;
		sbTextView->MoveTo(br.left, br.top+height+10);
		sbTextView->ResizeTo(br.Width(), height);
		
		ShowPreviewArea(true);
		
		if (!sb_hidden)
			br.right -= B_V_SCROLL_BAR_WIDTH;
		
		listview->MoveTo(br.left,br.top);
		listview->ResizeTo(br.Width(), height);
		
		sb->ResizeTo(B_V_SCROLL_BAR_WIDTH, height);
		
		currentWindowMode = WindowModePreview;
		
		sbTextView->SetResizingMode( B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT );
		listview->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT );
		sb->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_RIGHT );
		
		return;
	}
	
	// else: SimpleMode
	{
		sbTextView->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT );
		listview->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT );
		sb->SetResizingMode( B_FOLLOW_TOP | B_FOLLOW_LEFT );
	
		ShowPreviewArea(false);
		
		br.InsetBy(10,10);
		br.top += 5;
		br.right -= 1; //B_V_SCROLL_BAR_WIDTH;
		
		sbTextView->MoveTo(br.left,br.top);
		sbTextView->ResizeTo(br.Width(), br.Height() );
				
		if (!sb_hidden)
			br.right -= B_V_SCROLL_BAR_WIDTH;
		
		listview->MoveTo(br.left,br.top);
		listview->ResizeTo(br.Width(), br.Height() );
		

		
		sb->ResizeTo(B_V_SCROLL_BAR_WIDTH, br.Height() );
			
		currentWindowMode = WindowModeSimple;
		
		sbTextView->SetResizingMode( B_FOLLOW_ALL_SIDES );
		listview->SetResizingMode( B_FOLLOW_ALL_SIDES );
		sb->SetResizingMode( B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT );		
		return;	
	}

#endif
}

void
FrissView::UpdateScrollbar()
{
	float min,max;
	if (!sb) {
		//puts("sb ist tot!");
		return;
	}
	
	if (config->ScrollbarMode == ScrollbarModeOn) {
		if (sb_hidden) {
			sb->Show();
			sb_hidden = false;
			
			listview->ResizeBy( -B_V_SCROLL_BAR_WIDTH, 0 );
		}
	}
	else if (config->ScrollbarMode == ScrollbarModeOff) {
		if (!sb_hidden) {
			sb->Hide();
			sb_hidden = true;
			
			listview->ResizeBy( B_V_SCROLL_BAR_WIDTH, 0 );
		}
	}
	else {
		// Automatic
		sb->GetRange(&min, &max);
		
		//printf("SB:  Range: %f %f\n", min, max);
		if (min == 0.0f && max == 0.0f) {
			if (!sb_hidden) {
				sb->Hide();
				sb_hidden = true;
				
				listview->ResizeBy( B_V_SCROLL_BAR_WIDTH, 0 );
			}
		}
		else {
			if (sb_hidden) {
				sb->Show();
				sb_hidden = false;
				
				listview->ResizeBy( -B_V_SCROLL_BAR_WIDTH, 0 );
			}
		}
	}
}

FrissConfig*
FrissView::Config()
{
	return config;
}

XmlNode*
FrissView::GetFeedTree()
{
	return theRoot;
}


void
FrissView::UpdateVisitedLink(const char* url)
{
	// find url in listview
	//printf("Update url: %s\n", url);
	
	int anz = listview->CountItems();
	
	for (int i=0; i<anz; i++) {
		FStringItem* fi = (FStringItem*) listview->ItemAt(i);
		if (strcmp(url, fi->Url())==0) {
			//puts("found! :-D");
			fi->SetVisited();
			
			listview->Invalidate();
			return;
		}
	}
	
	//puts("not in list... maybe that wasn't invoked by us :-/");	
}

bool
FrissView::InitialVisitedLink(const char* pfad)
{
	BDirectory dir(pfad);
	if (dir.InitCheck() != B_OK) {
		return false;
	}
	
	//printf("Searching '%s'\n", pfad);
	
	BEntry entry;
	entry_ref ref;
	
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path(&entry);
		
		if ( !InitialVisitedLink(path.Path()) ) {
			entry.GetRef(&ref);
			VisitRef(&ref);
		}
	}
	
	return true;	
}

void
FrissView::VisitRef(entry_ref* ref)
{
	BNode bn(ref);
	if (bn.InitCheck() == B_OK) {
		char *url = (char*)calloc(URL_BUFSIZE, 1);
		bn.ReadAttr("META:url", B_STRING_TYPE, 0, url, URL_BUFSIZE);
		
		UpdateVisitedLink(url);
		free(url);
	}
}


void
FrissView::OnWorkspaceChanged()
{
	/*if (config->ColBackMode == ColBackTransparent) {
		UpdateBitmap();
		UpdateColors();
		Invalidate();
	}
	else*/
	
	UpdateColors();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void 
FrissView::AboutRequested()
{
	BString text("FRiSS Version " VER_ACTUAL "\n");
	text << VER_ADD"\n";
	
	text << "(C)2004 Andreas Herzig (N3S)\n";
	text << "\tbeos@herzig-net.de\n\n";
	
	text << "Original idea:\n\t0033\n\n";
	
#if (!OPTIONS_USE_NLANG && !__ZETA__)
	text << "R5 version without language support\n";
#else	
	if (config!= NULL && config->Lang.Compare("enDE") != 0) {
		text << _T("Language") << ": " << _T("FL:Language") << "\n";
		text << "\t" << _T("FL:Translator") << "\n";
		//text << "\tVersion " << _T("FL:Version") << "\n";
	}
#endif
	
	BAlert* alert = new BAlert("About fRiSS", text.String(), _T("Ok"));
	alert->SetShortcut( 0, B_ESCAPE );
	alert->Go();
}		


void
FrissView::NodeLaunch(FStringItem* node)
{
	Launch(node);
}

void
FrissView::NodeViewInformation(FStringItem* node)
{
	BString d(_T("No information available"));
	
	int32 linkoffset = 999999;
	int32 linklen = 0;
	
	if (node) {
		d = "";
		BString title( node->Title() );
		BString desc( node->Desc() );
		BString link( node->Url() );
		
		if (desc.Length()==0)
			desc = _T("<no description available>");
		if (link.Length()==0)
			link = _T("<no link available>");

		d << _T("Title") << ":\n" << title.String() << "\n\n";
		d << _T("Desc") << ":\n" << desc.String() << "\n\n";
		d << _T("Link") << ":\n";
		linkoffset = d.Length();
		linklen = link.Length();
		d << link.String();
	}
	
	
	if (config->WindowMode == WindowModeSimple) {
	
		BAlert *alert = new BAlert(_T("Information"), d.String(), _T("Ok"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->ResizeTo(400,300);
		alert->Go();
	}
	else {	
		rgb_color linkcol;
		linkcol.red = 0; linkcol.green = 0; linkcol.blue=255; linkcol.alpha = 0;
		
		BFont linkfont(be_plain_font);
		linkfont.SetFace(B_UNDERSCORE_FACE | B_NEGATIVE_FACE);
		
		tvTextView->SetText( d.String() );
		tvTextView->linkoffset = linkoffset;
		tvTextView->linklen = linklen;		
		tvTextView->fi = node;
		tvTextView->SetStylable(true);
		tvTextView->SetFontAndColor(linkoffset,linkoffset+linklen, &linkfont, B_FONT_ALL, &linkcol );
		
		//printf("-> %d %d\n", linkoffset, linklen);
	}	
}


void
FrissView::ShowPreviewArea(bool show)
{
	if (show) {
		if (tvTextView->IsHidden())
			tvTextView->Show();
		if (sbTextView->IsHidden())
			sbTextView->Show();
	}
	else {
		if (!sbTextView->IsHidden())
			sbTextView->Hide();
		if (!tvTextView->IsHidden())
			tvTextView->Hide();
	}
	Invalidate();
}
