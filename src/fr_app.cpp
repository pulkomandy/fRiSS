#include "fr_def.h"
#include "fr_view.h"
#include <FindDirectory.h>
#include <Path.h>
#include <unistd.h>

const char *app_signature = APP_SIGNATURE;

class MyWindow : public BWindow
{
private:
	FrissView	*myf;

public:
	MyWindow(FrissConfig* config, XmlNode* theList, BRect frame, const char* Title) : 
		BWindow(frame, Title, B_TITLED_WINDOW, B_FRAME_EVENTS)
	{	
		BRect brect = Bounds();
		myf = new FrissView(config, theList, brect);
		AddChild(myf);
	}
	
	bool QuitRequested()
	{
		Save(myf->Config(), myf->GetFeedTree() );
		be_app->PostMessage(B_QUIT_REQUESTED);
		return(TRUE);
	}
	
	
	void Save(FrissConfig* conf, XmlNode* root)
	{
		BPath	path;		
		
		if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
			path.Append("friss_settings.xml");
			
			conf->SetWindowRect( Frame() );
			conf->Save( path.Path() );
		}	
	}
	
};

class MyApplication : public BApplication
{
private:
	MyWindow *theWindow;

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
		
		printf("Ok, loading feed list: %s\n", config->Feedlist.String());
		if (!x_root->LoadFile( config->Feedlist.String() )) {
			config->m_iAnz = 4;
			config->SetIndex(0);
			FailsafeFeeds(x_root);
		}	

		/*puts("--------");
		x_root->Display();
		puts("--------");*/
		
		theWindow = new MyWindow(config,x_root,windowRect,VERSION_);
		theWindow->Show();
	}
	
	void FailsafeFeeds(XmlNode* root)
	{
		puts("failsafe: could not load feed list, using hardcoded ones");
		
		//root->CreateChild("opml");
		//root->CreateChild("opml/head");
		
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
