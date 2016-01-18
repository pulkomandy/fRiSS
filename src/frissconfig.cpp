#include "frissconfig.h"
#include "xmlnode.h"
#include <StorageKit.h>

FrissConfig::FrissConfig()
{
	Defaults();
}

FrissConfig::FrissConfig(BMessage* archive)
{
	Defaults();

	archive->FindInt32("version", (int32*)&m_iVersion);

	archive->FindInt32("RefreshRate", (int32*)&RefreshRate);

	archive->FindBool("RefreshAdvances", &RefreshAdvances);
	archive->FindString("Lang", &Lang);

	archive->FindInt32("m_iIndex", (int32*)&m_iIndex);
	archive->FindInt32("m_iAnz", (int32*)&m_iAnz);
	archive->FindString("Feedlist", &Feedlist);

	archive->FindRect("windowrect", &m_rWindow);

	archive->FindInt32("ColBackMode", (int32*)&ColBackMode);
	archive->FindInt32("ColForeMode", (int32*)&ColForeMode);

	const void* pColor = NULL;
	ssize_t iSize = 0;
	if (archive->FindData("ColBack", B_RGB_COLOR_TYPE, &pColor, &iSize) == B_OK)
		memcpy(&col, pColor, iSize);
	if (archive->FindData("ColFore", B_RGB_COLOR_TYPE, &pColor, &iSize) == B_OK)
		memcpy(&high, pColor, iSize);

	archive->FindInt32("BrowserType", (int32*)&BrowserType);
	archive->FindString("BrowserMime", &BrowserMime);

}

status_t
FrissConfig::Archive(BMessage *msg, bool deep) const
{
	msg->AddInt32("version", m_iVersion);
	msg->AddInt32("RefreshRate", RefreshRate);
	msg->AddBool("RefreshAdvances", RefreshAdvances);
	msg->AddString("Lang", Lang);

	msg->AddInt32("m_iIndex", m_iIndex);
	msg->AddInt32("m_iAnz", m_iAnz);
	msg->AddString("Feedlist", Feedlist);

	msg->AddRect("windowrect", m_rWindow);

	msg->AddInt32("ColBackMode", (int)ColBackMode);
	msg->AddInt32("ColForeMode", (int)ColForeMode);
	msg->AddData("ColBack", B_RGB_COLOR_TYPE, &col, sizeof(col), true, 1);
	msg->AddData("ColFore", B_RGB_COLOR_TYPE, &high, sizeof(high), true, 1);

	msg->AddInt32("BrowserType", BrowserType);
	msg->AddString("BrowserMime", BrowserMime);

	return B_OK;
}

BArchivable *
FrissConfig::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "FrissConfig"))
		return NULL;
	return new FrissConfig(data);
}

FrissConfig::~FrissConfig()
{
}

#define GET_NODE(key)				f=root->GetChild(key);if(f)
#define GET_VALUE_STR(key,value)	GET_NODE(key) value=f->Value()
#define GET_VALUE_INT(key,value)	GET_NODE(key) value=f->ValueAsInt()
#define GET_ATTR_STR(key,attr,value)	GET_NODE(key) value=f->Attribute(attr)
#define GET_ATTR_INT(key,attr,value)	GET_NODE(key) value=f->AttributeAsInt(attr)
#define GET_ATTR_BOOL(key,attr,value)	GET_NODE(key) value=f->AttributeAsBool(attr)

bool
FrissConfig::Load( const char* path )
{
	BString temp;
	temp << "Loading settings file : " << path << "\n";
	fprintf(stderr, temp.String());

	XmlNode* root = new XmlNode( NULL, "" );
	if (!root->LoadFile( path )) {
		delete root;
		return false;
	}

	XmlNode* f = NULL;

	GET_ATTR_INT( "settings", "version", m_iVersion );

	GET_ATTR_INT( "settings/generic/feeds", "a", m_iIndex );
	GET_ATTR_INT( "settings/generic/feeds", "b", m_iAnz );

	GET_ATTR_INT( "settings/generic/refresh", "rate", RefreshRate );
	GET_ATTR_BOOL( "settings/generic/refresh", "advance", RefreshAdvances );

	GET_VALUE_STR( "settings/generic/language", Lang );
	GET_VALUE_STR( "settings/generic/feedfile", Feedlist );

	f=root->GetChild("settings/window/position");
	if(f) {
	 	m_rWindow.left = f->AttributeAsInt("x");
 		m_rWindow.top = f->AttributeAsInt("y");
	 	m_rWindow.right = m_rWindow.left + f->AttributeAsInt("width");
 		m_rWindow.bottom = m_rWindow.top + f->AttributeAsInt("height");
 	}

	GET_NODE( "settings/window/background/mode") ColBackMode = (ColBack_t)f->ValueAsInt();
	f=root->GetChild("settings/window/background/colour");
	if(f) {
	 	col.red = f->AttributeAsInt("red");
 		col.green = f->AttributeAsInt("green");
	 	col.blue = f->AttributeAsInt("blue");
	}

	GET_NODE( "settings/window/foreground/mode") ColForeMode = (ColFore_t)f->ValueAsInt();
	f=root->GetChild("settings/window/foreground/colour");
	if(f) {
	 	high.red = f->AttributeAsInt("red");
 		high.green = f->AttributeAsInt("green");
	 	high.blue = f->AttributeAsInt("blue");
	}

	if ((f = root->GetChild("settings/window/windowmode")) != NULL)
		WindowMode = (WindowMode_t) f->ValueAsInt();

	if ((f = root->GetChild("settings/browser/mode")) != NULL)
		BrowserType = (BrowserType_t) f->ValueAsInt();

	if ((f = root->GetChild("settings/browser/mime")) != NULL)
		BrowserMime = f->Value();


	delete root;
	return true;
}

bool
FrissConfig::Save( const char* path )
{
	XmlNode* root = new XmlNode( NULL, "" );
	XmlNode* fs = NULL, *f = NULL;
	BString s;

	fs = root->CreateChild("settings");
	fs->AddAttribute("version", m_iVersion);

	f = new XmlNode(fs, "comment");
	f->Comment("This file is auto-generated. See documentation for help.");
	fs->AddChild(f);

	f = fs->CreateChild("generic/feeds");
	f->AddAttribute("a", m_iIndex);
	f->AddAttribute("b", m_iAnz);

	f = fs->CreateChild("generic/refresh");
	f->AddAttribute("rate", RefreshRate );
	if (RefreshAdvances)
		f->AddAttribute("advance", "true");
	else
		f->AddAttribute("advance", "false");

	fs->CreateChild("generic/language", Lang.String());
	fs->CreateChild("generic/feedfile", Feedlist.String());

	f = fs->CreateChild("window/position");
	f->AddAttribute("x", m_rWindow.left);
	f->AddAttribute("y", m_rWindow.top);
	f->AddAttribute("width", m_rWindow.Width());
	f->AddAttribute("height", m_rWindow.Height());

	fs->CreateChild("window/background/mode", (int)ColBackMode);
	f = fs->CreateChild("window/background/colour");
	f->AddAttribute("red", col.red);
	f->AddAttribute("green", col.green);
	f->AddAttribute("blue", col.blue);

	fs->CreateChild("window/foreground/mode", (int)ColForeMode);
	f = fs->CreateChild("window/foreground/colour");
	f->AddAttribute("red", high.red);
	f->AddAttribute("green", high.green);
	f->AddAttribute("blue", high.blue);

	fs->CreateChild("window/windowmode", (int)WindowMode);

	fs->CreateChild("browser/mode", (int)BrowserType);
	fs->CreateChild("browser/mime", BrowserMime.String());

	root->SaveToFile(path);
	delete root;
	return false;
}

void
FrissConfig::Defaults()
{
	// generic/misc =============================================================
	m_iVersion		= 6;
	RefreshRate		= 3 * 120;
	RefreshAdvances = false;
	Lang			= "enDE";

	// feedlist =================================================================
	m_iIndex		= 0;
	m_iAnz			= 1;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		Feedlist	= path.Path();
		Feedlist	<< "/fRiSS/feeds.opml";
	}
	else
		Feedlist	= "/boot/home/config/settings/fRiSS/feeds.opml";

	// Appearance ===============================================================
	m_rWindow.Set(50,50,300,200);

	ColBackMode		= ColBackDefault;
	ColForeMode		= ColForeAdapt;

	FrSetCol3( col,  51,102,152 );
	FrSetCol3( high, 0,0,0 );

	WindowMode		= WindowModePreview;

	// Browser ==================================================================

	BrowserType		= BrowserNetpositive;
	BrowserMime		= BROWSER_MIME_CUSTOM_DEFAULT;
}
