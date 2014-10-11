#include "fr_def.h"
#include "fr_view.h"
#include "feedlistview.h"
#include "frissWindow.h"

#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <GroupView.h>
#include <Path.h>
#include <unistd.h>

const char *app_signature = APP_SIGNATURE;

const int msg_SelFeed = 'slfd';
const int msg_EditFeed = 'edfd';

void FrissWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case msg_SelFeed:
			myf->Load(feedList->CurrentSelection());
			break;
		case msg_EditFeed:
		{
			int item = feedList->CurrentSelection();
			if (item < 0)
				break;

			BPoint p(30,30);
			p = ConvertToScreen(p);
			(new FPrefEditWindow(this, Xfeeds->ItemAt(item), p, true))->Show();
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}

FrissWindow::FrissWindow(FrissConfig* config, XmlNode* theList, BRect frame,
	const char* Title) : 
	BWindow(frame, Title, B_TITLED_WINDOW,
		B_FRAME_EVENTS | B_AUTO_UPDATE_SIZE_LIMITS)
{	
	SetLayout(new BGroupLayout(B_VERTICAL));
	BGridView* background = new BGridView("background");
	background->GridLayout()->SetInsets(7, 7, 0, 0);
	background->GridLayout()->SetSpacing(0, 0);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);

	feedList = new BListView("feedlist", B_SINGLE_SELECTION_LIST);
	background->GridLayout()->AddView(new BScrollView("scroll", feedList,
		0, false, true), 0, 0);

	myf = new FrissView(config, theList);
	background->GridLayout()->AddView(myf,1,0);

	/* Nun noch den Dragger einbauen.
	 * B_ORIGIN scheint unten rechts zu sein, also noch entsprechend
	 * die linke obere Ecke setzen (ich wusste vorher auch nicht, dass
	 * der Dragger 7x7 Pixel groß ist :-)
	 */
	BDragger* myd = new BDragger(myf, B_WILL_DRAW );
	myd->SetExplicitMaxSize(BSize(7, 7));
	background->GridLayout()->AddView(myd, 2, 1);

	BMessage* m = new BMessage(msg_SelFeed);
	feedList->SetSelectionMessage(m);

	m = new BMessage(msg_EditFeed);
	feedList->SetInvocationMessage(m);

	PopulateFeeds(theList);
}

void FrissWindow::PopulateFeeds(XmlNode* theList)
{
	// TODO we should be caching the icons for each feed...
	for(int i = feedList->CountItems(); --i>=0;)
		delete feedList->ItemAt(i);
	feedList->MakeEmpty();

	Xfeeds = theList->FindChild("body", NULL, true);
	for (uint32 i = 0; i < Xfeeds->Children(); i++) {
		FeedListItem* it = new FeedListItem(Xfeeds->ItemAt(i));
		feedList->AddItem(it);
	}
}

bool FrissWindow::QuitRequested()
{
	Save(myf->Config(), myf->GetFeedTree() );
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}


void FrissWindow::Save(FrissConfig* conf, XmlNode* root)
{
	BPath	path;		

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append("friss_settings.xml");

		conf->SetWindowRect( Frame() );
		conf->Save( path.Path() );
	}	
}


class MyApplication : public BApplication
{
	public:
		MyApplication(const char *signature) :
			BApplication(signature)
		{}

		void ReadyToRun() {
			// Creates the window and sets the title with the application name. 
			BRect		windowRect;
			BPath		path;
			FrissConfig*	config = new FrissConfig();
			//config_t	config;
			XmlNode*	x_root = new XmlNode( NULL, "" );
			bool		ok = false;

			if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
				path.Append("friss_settings.xml");

				ok = config->Load( path.Path() );
			}

			if (!ok) { // Defaults:
				puts("Reverting to failsafe.");

				config->Defaults();
			}

			windowRect = config->GetWindowRect();

			if (!x_root->LoadFile( config->Feedlist.String() )) {
				config->m_iAnz = 4;
				config->SetIndex(0);
				FailsafeFeeds(x_root);
			}	

			BString title("fRiSS ");
			title << version();
			theWindow = new FrissWindow(config,x_root,windowRect, title.String());
			theWindow->Show();
		}

		virtual void MessageReceived(BMessage *msg)
		{
			BApplication::MessageReceived(msg);
		}

		void AboutRequested()
		{
			// FIXME use BAboutBox
			BString text("FRiSS Version ");
			text << version();
			text <<	"\n";

			text << "\xC2\xA9""2010-2013 Adrien Destugues (PulkoMandy)\n";
			text << "\tpulkomandy@pulkomandy.tk\n\n";

			text << "\xC2\xA9""2004 Andreas Herzig (N3S)\n";
			text << "\tbeos@herzig-net.de\n\n";

			text << "Original idea:\n\t0033\n\n";

			// FIXME translation credits
#if 0
			if (config!= NULL && config->Lang.Compare("enDE") != 0) {
				text << _T("Language") << ": " << _T("FL:Language") << "\n";
				text << "\t" << _T("FL:Translator") << "\n";
			}
#endif

			BAlert* alert = new BAlert("About fRiSS", text.String(), _T("Ok"));
			alert->SetShortcut( 0, B_ESCAPE );
			alert->Go();
		}	

	private:
		void FailsafeFeeds(XmlNode* root)
		{
			// Just create an empty feed list
			time_t now = time(NULL);
			BString d(ctime(&now));
			d.RemoveSet("\t\r\n");
			root->CreateChild("opml/head/dateCreated", d.String());
			root->CreateChild("opml/body");
		}

		BString version() {
			app_info appInfo;
			BFile file;
			BAppFileInfo appFileInfo;

			be_app->GetAppInfo(&appInfo);
			file.SetTo(&appInfo.ref, B_READ_WRITE);
			appFileInfo.SetTo(&file);

			BString version;

			version_info info;
			appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND);
			version << info.major;
			version << '.';
			version << info.middle;
			version << '.';
			version << info.minor;

			switch(info.variety)
			{
				case B_BETA_VERSION:
					version << "\xCE\xB2";
			}

			if(info.internal > 0) {
				version << info.internal;
			}

			return version;
		}	

	private:
		FrissWindow *theWindow;
};

int main()
{
	new MyApplication(app_signature);
	be_app->Run();

	delete be_app;
	return 0;
}
