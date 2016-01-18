#include "fr_view.h"
#include "load.h"
#include "parser.h"
#include "frissWindow.h"
#include "fr_ftextview.h"
#include "fr_fstringitem.h"

#include <assert.h>
#include <time.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <GridLayoutBuilder.h>
#include <InterfaceDefs.h>
#include <Path.h>
#include <StorageKit.h>
#include <UrlProtocolAsynchronousListener.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

// Helper window informs us about workspaces changes etc:
#ifdef OPTIONS_USE_HELPERWINDOW
	#include "fr_helperwindow.inc"
#endif


#include <stdlib.h>


FrissView::FrissView(FrissConfig* newconf, XmlNode* x_root)
	: BGridView("fRiSS")
	, pop(NULL)
	, miGo(NULL)
	, tvTextView(new FTextView())
	, sbTextView(new BScrollView("sbTextView", tvTextView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_PLAIN_BORDER))
	, listview(new FListView("listview", B_SINGLE_SELECTION_LIST))
	, config(newconf)
	, theRoot(x_root)
	, theList(theRoot->FindChild("body", NULL, true))
{
	listview->Hide();
	sbTextView->Hide();
	tvTextView->MakeSelectable(false);

	BGridLayoutBuilder(this)
		.SetInsets(7, 0, 0, 0)
		.Add(new BScrollView("scroll", listview, 0, false, true), 0, 0)
		.Add(sbTextView, 0, 1);

	// Only transparency for replicants:
	replicant = false;
}


FrissView::FrissView(BMessage *archive) :
	BGridView(archive)
{
	replicant = true;

	BMessage msg;
	archive->FindMessage("frissconfig", &msg);

	config = new FrissConfig(&msg);

	listview = dynamic_cast<FListView*>(FindView("listview"));
	sbTextView = dynamic_cast<BScrollView*>(FindView("sbTextView"));
	tvTextView = dynamic_cast<FTextView*>(FindView("Feed item text view"));

	// Reassemble feed list:
	BMessage list;
	archive->FindMessage("feeds", &list);
	theRoot = new XmlNode(&list);
	theList = theRoot->FindChild("body", NULL, true);
}

FrissView::~FrissView()
{
	delete tlist;	tlist = NULL;
}


void Notify(const char* msg)
{
	BAlert *alert = new BAlert( "Vorsicht!", msg, "Mist!" );
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}

status_t
FrissView::Archive(BMessage *data, bool deep) const
{
	BGridView::Archive(data, deep);

	if (deep) {
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

	data->AddString("add_on", app_signature);

	return B_OK;
}

BArchivable *
FrissView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "FrissView"))
		return NULL;
	return new FrissView(data);
}


class FeedLoadListener: public BUrlProtocolAsynchronousListener
{
	public:
					FeedLoadListener(BView* view)
						: BUrlProtocolAsynchronousListener(true) {
			fView = view;
			fCurrentRequest = NULL;
		}

		virtual		~FeedLoadListener() {};

		void		ConnectionOpened(BUrlRequest* request) {
			if (fCurrentRequest)
				fCurrentRequest->Stop();
			fCurrentRequest = request;
			result.SetSize(0);
		}

		void		DataReceived(BUrlRequest* request, const char* data, off_t position,
						ssize_t size) {
			if (request != fCurrentRequest) {
				delete request;
			}
			result.WriteAt(position, data, size);
		}

		void		RequestCompleted(BUrlRequest* request, bool success) {
			if (request != fCurrentRequest)
				return;
			BMessage* msg = new BMessage(success ? MSG_LOAD_DONE : MSG_LOAD_FAIL);
			msg->AddPointer("data", result.Buffer());
			fView->Looper()->PostMessage(msg, fView);

			fCurrentRequest = NULL;
			delete request;
		}

		BUrlRequest* fCurrentRequest;
		BView* fView;
		BMallocIO result;
};


void
FrissView::AllAttached()
{
	BGridView::AllAttached();

	fLoadListener = new FeedLoadListener(this);

	status_t stat;
	messenger = new BMessenger(this, NULL, &stat);

	// Pulse/Reload timer:
	last_reload = 0;
	pulses = 0;
	pulsing = true;
	inv = false;

	currentFeed = NULL;

	// "Global" Buffer for data and items:
	tlist = new BObjectList<FStringItem>();
	loadList = new BObjectList<FStringItem>();

	// Views:
	Load(config->Index());

	BString ts;

	pop = new BPopUpMenu("popup",false,false,B_ITEMS_IN_COLUMN);

	pop->AddItem( miGo = new BMenuItem( _T("Go"), NULL ) );

	ts << _T("Information") << B_UTF8_ELLIPSIS;
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

	ts = _T("Preferences"); ts << B_UTF8_ELLIPSIS;
	pop->AddItem( miOpts = new BMenuItem( ts.String(), NULL /*,'P', B_CONTROL_KEY*/ ) );
	pop->AddSeparatorItem();
	pop->AddItem(miAbout = new BMenuItem(_T("About fRiSS" B_UTF8_ELLIPSIS),
		NULL /*,'A', B_CONTROL_KEY*/ ) );
	if (replicant) {
		pop->AddSeparatorItem();
		pop->AddItem( miDebug = new BMenuItem( _T("Remove Replicant"), NULL /*,'Q', B_CONTROL_KEY*/ ) );
	}
	else
		miDebug = NULL;

	config->m_iAnz = BuildPopup(theList, mList);

	UpdateColors();
	currentWindowMode = WindowMode_NONE;
	UpdateWindowMode();
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
	rgb_color desktopColor = BScreen(B_MAIN_SCREEN_ID).DesktopColor();


	if (replicant && config->ColBackMode == ColBackTransparent) {
		listview->transparent = true;
		//colback = config.col;
		//colback.alpha = 255

		colback = B_TRANSPARENT_COLOR;
		collow = desktopColor;
	}
	else if (config->ColBackMode == ColBackDesktop) {
		colback = desktopColor;
		collow = colback;
	} else if (config->ColBackMode == ColBackCustom) {
		colback = config->col;
		collow = colback;
	} else {
		colback = ui_color(B_PANEL_BACKGROUND_COLOR);
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

	tvTextView->SetFontAndColor( be_plain_font, B_FONT_ALL, &colfore );

	//puts("Invalidate?");

	Invalidate();
	if (!sbTextView->IsHidden())
		tvTextView->Invalidate();

	if (config->ColBackMode == ColBackTransparent) {
		Window()->UpdateIfNeeded();
	}
}

void
FrissView::MessageReceived(BMessage *msg)
{
	entry_ref	ref;

	switch(msg->what) {
		case MSG_PREF_DONE:
			// signaled by FrissPrefWindow, saying it's done
			//inv = true;

			theRoot->SaveToFile( config->Feedlist.String() );
			pulsing = true;

			UpdateColors();
			ReBuildPopup(theList, mList);

			if (!replicant)
				((FrissWindow*)Window())->PopulateFeeds(theRoot);
			break;

		case MSG_LOAD_DONE:
		{
			void* data = NULL;
			msg->FindPointer("data", &data);
			LoadDone((char*)data);
			break;
		}

		case MSG_LOAD_FAIL:
			pulsing = true;
			pulses = 0;
			Error(_T("An error occured while loading this feed."));
			break;

		case MSG_COL_CHANGED:
			UpdateColors();
			break;

		case MSG_SB_CHANGED:
			UpdateWindowMode();
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
				miAbout->SetLabel(_T("About fRiSS"B_UTF8_ELLIPSIS));
			}
			break;

		case CmdLoad:
			{
			}
			break;

		case B_ABOUT_REQUESTED:
			be_app->AboutRequested();
			break;

		default:
			BGridView::MessageReceived(msg);
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
FrissView::Load(uint32 idx, XmlNode* direct)
{
	assert(theList != NULL);
	if (theList->Children() == 0) {
		Error(_T("Please add some feeds in the Preferences Window!"));
		return;
	}

	// Load
	pulsing = false;

	int x = 0;
	XmlNode* fi;

	if (!direct) {
		if (idx >= config->m_iAnz)
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

	// Get some mem
	BString title(fi->Attribute("text"));
	BString url(fi->Attribute("xmlURL"));

	// FIXME SetLabel(title.String());

	// run external program
	if (url.Compare("run://", 6)==0) {
		if (!listview->IsHidden())
			listview->Hide();

		if (config->WindowMode == WindowModeSimple)
			ShowPreviewArea(true);

		url.Remove(0,6);

		char buf[BUFSIZE];
		if ( LoadFile(buf, BUFSIZE, url.String()) > 0 )
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
	currentFeed = fi;

	if (!listview->IsHidden())
		listview->Hide();

	if (config->WindowMode == WindowModeSimple)
		ShowPreviewArea(true);

	tvTextView->SetText(_T("Loading..."));

	// Clear the previous items
	int anz = listview->CountItems();
	for (int i = anz - 1; i > 0; i-- )
		delete listview->RemoveItem(i);

	pulsing = false;

	BUrl urlReq(url);
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(urlReq,
		fLoadListener->SynchronousListener());
	if (request == NULL) {
		// unsupported protocol
		BString errf;
		errf = "Unsupported protocol: ";
		errf << url.String();
		errf << "\n\nValid protocols are:\n\thttp://\n\tfile://\n\trun://\n" ;

		Error( errf.String() );
		return;
	}

	request->Run();
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
FrissView::LoadDone(char* buf)
{
	BString	status;
	int		anz = 0;
	bool	addDesc = false;
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
				anz = Parse_ics(buf, tlist, status);
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

				anz = Parse_rss(root, tlist, status, addDesc);
			}
			else {
				x = root->FindChild("feed");

				if (x) {
					#ifndef OPTIONS_NO_ATOM
						anz = Parse_atom(root, tlist, status, addDesc);
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

		// FIXME SetLabel( title.String() );
	}

	tvTextView->SetText("");

	const char* feed_title = currentFeed->Attribute("text");
	const char* feed_url = currentFeed->Attribute("xmlUrl");

	BPath friss_path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &friss_path, true);
	friss_path.Append("fRiSS");

	BPath feed_path(friss_path.Path());
	feed_path.Append(feed_title);

	BDirectory current_dir(friss_path.Path());
	BDirectory feed_dir(feed_path.Path());

	listview->MakeEmpty();
	if (feed_dir.InitCheck() != B_OK) {
		current_dir.CreateDirectory(feed_title, &feed_dir );
		feed_dir.WriteAttr("url", B_STRING_TYPE, 0, feed_url,
			strlen(feed_url));
	} else {
		entry_ref ref;
		while (1) {
			status_t status = feed_dir.GetNextRef(&ref);

			if (status != B_OK) {
				if (status == B_ENTRY_NOT_FOUND)
					break;
				else
					continue;
			}

			BString loaded_title("");
			BString loaded_url("");
			BString loaded_date("");
			BString file_status;
			BString file_contents;
			bool isRead = false;

			BFile file(&ref, B_READ_ONLY);
			off_t size = 0;
			file.GetSize(&size);

			char* buf = file_contents.LockBuffer(size);
			file.Read(buf, size);
			file_contents.UnlockBuffer(size);

			file.ReadAttrString("title", &loaded_title);
			file.ReadAttrString("url", &loaded_url);
			file.ReadAttrString("date", &loaded_date);
			file.ReadAttr("read", B_BOOL_TYPE, 0, &isRead, sizeof(isRead));

			BPath filename(feed_path.Path());
			filename.Append(loaded_title.String());

			FStringItem* loaded_article = new FStringItem(
				loaded_title.String(),
				loaded_url.String());
			loaded_article->SetDate(loaded_date.String());
			loaded_article->SetVisited(isRead);

			XmlNode* root = new XmlNode(file_contents, NULL);
			root->LoadFile(filename.Path());
			loaded_article->SetDesc(root);

			listview->AddItem(dynamic_cast<BListItem*>(loaded_article));
		}
	}

	int a = tlist->CountItems();
	int listview_size = listview->CountItems();
	for (int i=0;i<a;i++) {
		FStringItem* current_item = tlist->ItemAt(i);
		const char* title = current_item->Title();
		const char* url = current_item->Url();
		const char* date = current_item->Date();

		bool addItem = true;
		for (int list_index=0;list_index<listview_size;list_index++)
		{
			FStringItem* current_list_item =
				dynamic_cast<FStringItem*>(listview->ItemAt(list_index));
			const char* list_title = current_list_item->Title();
			const char* list_url = current_list_item->Url();

			if ((strcmp(title, list_title) == 0) &&
				(strcmp(url, list_url) == 0)) {
				addItem = false;
			}
		}

		if (addItem) {
			printf("SAVING ITEM...\n");

			int defaultValue = 0;

			listview->AddItem(dynamic_cast<BListItem*>(current_item));

			BString file_name(feed_path.Path());
			file_name.Append("/");
			file_name.Append(title);
			current_item->Desc()->SaveToFile(file_name);

			BFile file;
			file.SetTo(file_name, B_READ_WRITE);

			file.WriteAttr("title", B_STRING_TYPE, 0, title, strlen(title));
			file.WriteAttr("read", B_BOOL_TYPE, 0, &defaultValue,
				sizeof(defaultValue));
			file.WriteAttr("url", B_STRING_TYPE, 0, url, strlen(url));
			file.WriteAttr("date", B_STRING_TYPE, 0, date, strlen(date));
		}
	}

	if (config->WindowMode == WindowModeSimple)
		ShowPreviewArea(false);

	if (listview->IsHidden())
		listview->Show();

	tlist->MakeEmpty();

	// Necessary?
	pulses = 0;
	pulsing = true;

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
		Launch( (FStringItem*)listview->ItemAt(idx) );
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
		FrissPrefWin* pf = new FrissPrefWin(this, config, theList, _T("Preferences"));
		pf->Show();
		// now we're "blocked" until the pref window sends 'PREF' to us
	}
	else if (mi == miAbout) {
		be_app->AboutRequested();
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

		BString url(fi->Url());
		int list_size = listview->CountItems();
		for (int i=0; i<list_size; i++) {
			FStringItem* fi = (FStringItem*) listview->ItemAt(i);
			if (strcmp(url.String(), fi->Url())==0) {
				fi->SetVisited();

				const char* feed_title = currentFeed->Attribute("text");
				int selected_link = 1;

				BPath file_path;
				find_directory(B_USER_SETTINGS_DIRECTORY, &file_path, true);
				file_path.Append("fRiSS");
				file_path.Append(feed_title);
				file_path.Append(fi->Title());

				BFile file(file_path.Path(), B_READ_WRITE);
				file.WriteAttr("read", B_BOOL_TYPE, 0, &selected_link,
					sizeof(selected_link));

				break;
			}
		}
	}
}


void FrissView::OpenURL(BString url)
{
	char *argv[2];
	BString app;
	int BrowserType = config->BrowserType;

	switch (BrowserType) {
		case BrowserCustom:
			app = config->BrowserMime;
			break;

		case BrowserFirefox:
			app = BROWSER_MIME_FIREFOX;
			break;

		case BrowserNetpositive:
		default:
			app = BROWSER_MIME_WEBPOSITIVE;
	}

	argv[0] = (char*)url.String();
	argv[1] = 0;

	status_t status = be_roster->Launch( app.String(), 1, argv );
	//printf("Result is: %i\n", (int)status);

	if (status == B_OK || status == B_ALREADY_RUNNING) {
		listview->DeselectAll();
	} else {
		BString message = _T("Error: The browser could not be started :-(\n");
		message << strerror(status);
		(new BAlert( _T("Error"), message, _T("Ok") ))->Go();
	}
}


void
FrissView::Launch(FStringItem* fi)
{
	if (!fi) {
		Error("DEBUG: FV::Launch() fails because a NULL pointer is detected");
		return;
	}

	OpenURL(fi->Url());
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
FrissView::FrameResized(float width, float height)
{
	BGridView::FrameResized(width,height);

	UpdateWindowMode();
}


void
FrissView::UpdateWindowMode()
{
#ifdef OPTIONS_WINDOW_MODE
	if (config->WindowMode == WindowModePreview) {
		ShowPreviewArea(true);
		currentWindowMode = WindowModePreview;
		return;
	}

	// else: SimpleMode
	{
		ShowPreviewArea(false);
		currentWindowMode = WindowModeSimple;
		return;
	}

#endif
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
FrissView::NodeViewInformation(FStringItem* node)
{
	if (config->WindowMode == WindowModeSimple) {
		// FIXME we are better off making our own window with an FTextView.
		// There is no way to render the (x)html content of feeds inside a
		// BAlert...
		BString d(_T("No information available"));

		if (node) {
			d = "";
			BString title( node->Title() );
			BString link( node->Url() );

			if (link.Length()==0)
				link = _T("<no link available>");

			d << _T("Title") << ":\n" << title.String() << "\n\n";
			d << _T("Link") << ":\n";
			d << link.String();
		}

		BAlert *alert = new BAlert(_T("Information"), d.String(), _T("Ok"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->ResizeTo(400,300);
		alert->Go();
	}
	else if(node) {
		tvTextView->SetContents(node->Title(), *node->Desc(), node->Url());
	} else {
		tvTextView->SetText(_T("No information available"));
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
