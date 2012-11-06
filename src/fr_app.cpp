#include "fr_def.h"
#include "fr_view.h"
#include "feedlistview.h"
#include "frissWindow.h"

#include <FindDirectory.h>
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
			BPoint p(30,30);
			p = ConvertToScreen(p);
			(new FPrefEditWindow(this, Xfeeds->ItemAt(feedList->CurrentSelection()), p, true))->Show();
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}

FrissWindow::FrissWindow(FrissConfig* config, XmlNode* theList, BRect frame,
	const char* Title) : 
	BWindow(frame, Title, B_TITLED_WINDOW, B_FRAME_EVENTS)
{	
	BRect brect = Bounds();

	BView* background = new BView(brect, "background",
			B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);

	brect.InsetBy(5,5);
	brect.left += 150;
	myf = new FrissView(config, theList, brect);
	background->AddChild(myf);

	brect = Bounds();
	brect.InsetBy(5, 5);
	brect.right = brect.left + 150 - 5 - B_V_SCROLL_BAR_WIDTH;

	feedList = new BListView(brect, "feedlist", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL_SIDES);

	background->AddChild(new BScrollView("scroll", feedList,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0, false, true));

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
	for (int i = 0; i < Xfeeds->Children(); i++) {
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
	private:
		FrissWindow *theWindow;

	public:
		MyApplication(const char *signature) :
			BApplication(signature) {

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

		theWindow = new FrissWindow(config,x_root,windowRect,VERSION_);
		theWindow->Show();
	}
	
	void FailsafeFeeds(XmlNode* root)
	{
		time_t now = time(NULL);
		BString d(ctime(&now));
		d.RemoveSet("\t\r\n");
		root->CreateChild("opml/head/dateCreated", d.String());
			
		XmlNode *body = root->CreateChild("opml/body");
		
		{
			XmlNode *node = new XmlNode("outline");
			node->AddAttribute("text", "Fortune");
			node->AddAttribute("xmlURL", "fortune");
			body->AddChild(node);
		}
		{
			XmlNode *node = new XmlNode("outline");
			node->AddAttribute("text", "LOCAL mini");
			node->AddAttribute("xmlURL", "file:///boot/projects/friss/samples_xml/mini.xml");
			body->AddChild(node);
		}		
		/*{
			XmlNode* bex = new XmlNode("outline");
			bex->AddAttribute("text", "BeOS");
			body->AddChild(bex);
			
			{			
				XmlNode *node = new XmlNode("outline");
				node->AddAttribute("text", "bebits");
				node->AddAttribute("xmlURL", "http://www.bebits.com/backend/recent.rdf");
				bex->AddChild(node);
			}
			{
				XmlNode *node = new XmlNode("outline");
				node->AddAttribute("text", "isComputerOn");
				node->AddAttribute("xmlURL", "http://www.iscomputeron.com/index/backend.php");
				bex->AddChild(node);
			}
		}
		{
			XmlNode* bex = new XmlNode("outline");
			bex->AddAttribute("text", "Media");
			body->AddChild(bex);
			{			
				XmlNode *node = new XmlNode("outline");
				node->AddAttribute("text", "ORF1");
				node->AddAttribute("xmlURL", "http://rss.orf.at/orf1.xml");
				node->AddAttribute("x-addDesc", "true");
				bex->AddChild(node);
			}
			{
				XmlNode *node = new XmlNode("outline");
				node->AddAttribute("text", "ORF2");
				node->AddAttribute("xmlURL", "http://rss.orf.at/orf2.xml");
				node->AddAttribute("x-addDesc", "true");
				bex->AddChild(node);
			}
		}*/
		{
			XmlNode* bex = new XmlNode("outline");
			bex->AddAttribute("text", "EmptyGroup");
			body->AddChild(bex);	
		}
			/*
			config.list->AddItem( new FStringItem("Fortune","fortune") );		
			config.list->AddItem( new FStringItem("bebits","http://www.bebits.com/backend/recent.rdf") );
			config.list->AddItem( new FStringItem("isComputerOn","http://www.iscomputeron.com/index/backend.php") );
			config.list->AddItem( new FStringItem("BeosJournal","http://www.beosjournal.org/recent.rdf") );
			config.list->AddItem( new FStringItem("OSNews", "http://www.osnews.com/files/recent.rdf") );
			//config.list->AddItem( new FStringItem("Heise Newsticker","http://www.heise.de/newsticker/heise.rdf") );
			*/

	}
	
	virtual void MessageReceived(BMessage *msg)
	{
		BApplication::MessageReceived(msg);
	}
};

int main()
{
	new MyApplication(app_signature);
	be_app->Run();

	delete be_app;
	return 0;
}
