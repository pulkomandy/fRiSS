#include "fr_options.h"

#include "xmlnode.h"
#include <stdlib.h>

#include <File.h>

//#define TRACE_XML
#define XML_FUNC	3

#ifdef TRACE_XML
	#define XPRINT( lvl, x ) if(lvl>=XML_FUNC) printf x
#else
	#define XPRINT( lvl, x )
#endif

#define SkipW( x ) 	while (x && *x && (*x=='\r' || *x=='\n' || *x=='\t' || *x==' ')) x++;
#define myName			mName.String()
#define anzChildren		mChild.CountItems()
#define anzAttribute	mAttribute.CountItems()
#define anzAttributes	mAttribute.CountItems()


uint32	XmlNode::encoding = XML_ENCODING_NONE;

XmlNode::XmlNode(const char* name) :
	BStringItem(name),
	mName(name),
	mData("")
{
	mParent	= NULL;
	mType	= XML_TYPE_SIMPLE;
	mMarked = false;
	
	p = NULL;
}


XmlNode::XmlNode(XmlNode* parent, const char* name, uint32 level, bool expanded) :
	BStringItem(name, level, expanded),
	mName(name),
	mData("")
{
	mParent	= parent;
	mType	= XML_TYPE_NODE;
	mMarked = false;

	p = NULL;	
}

XmlNode::XmlNode(const char* buf, XmlNode* parent) :
	BStringItem("root"),
	mName(""),
	mData("")
{
	mParent	= parent;
	mType	= XML_TYPE_NODE;
	mMarked = false;
	
	p = NULL;	
	
	// root object does not parse itself;
	while (buf && *buf) {
		SkipW( buf );
		XmlNode* c = new XmlNode(this);
		mChild.AddItem(c);
		
		buf = c->Parse(buf);
	}
	
	if (!buf)
		XPRINT(0,("Parse error\n"));
	else
		XPRINT(0,("XmlNode(buf,parent): ok\n"));
}


XmlNode::XmlNode(BMessage *archive) :
	BStringItem(archive),
	mName(""),
	mData("")
{
	// Init
	mParent = NULL;
	mType	= XML_TYPE_SIMPLE;
	mMarked	= false;	
	p = NULL;	

	// auspacken:
	archive->FindString("name", &mName);
	archive->FindString("data", &mData);
	archive->FindInt32("type", (int32*)&mType);
	archive->FindBool("marked", &mMarked);

	// Parent wird von aussen gesetzt, hier nur default:
	mParent = NULL;
	
	int a = archive->FindInt32("attributes");
	int c = archive->FindInt32("children");
	
	for (int i=0; i<a; i++) {
		XmlNode* n = new XmlNode(this);
		n->mType = XML_TYPE_SIMPLE;
		
		if (archive->FindString("attr_n", i, &n->mName) == B_OK
		&&	archive->FindString("attr_v", i, &n->mData) == B_OK) {
			mAttribute.AddItem(n);
		}
		else
			delete n;
	}
	
	for (int i=0; i<c; i++) {
		BMessage msg;
		
		if (archive->FindMessage("child", i, &msg) == B_OK) {		
			XmlNode* node = new XmlNode(&msg);
			AddChild(node);
		}
	}

	//encoding = 
}


status_t
XmlNode::Archive(BMessage *msg, bool deep) const
{
	if (!msg)
		return B_ERROR;
		
	msg->AddString("class", "XmlNode");
	
	msg->AddString("name", mName.String());
	msg->AddInt32("type", mType);
	msg->AddString("data", mData.String());
		
	int a = Attributes(), c = anzChildren;
	msg->AddInt32("attributes", a);
	
	for (int i=0; i<a; i++) {
		XmlNode* n = (XmlNode*)mAttribute.ItemAt(i);
		
		if (!n)
			return B_ERROR;
		
		const char* strName = n->Name();	// "version"
		msg->AddString("attr_n", strName);  
		
		const char* strVal = n->Value();	// NULL
		msg->AddString("attr_v", strVal);   // crasht hier!
	}
	
	if (deep) {
		msg->AddInt32("children", c);
		for (int i=0; i<c; i++) {
			BMessage* childmsg = new BMessage;
			XmlNode* n = (XmlNode*)mChild.ItemAt(i);

			if (!n)
				return B_ERROR;			
			
			if (n->Archive(childmsg, deep) == B_OK)
				msg->AddMessage("child", childmsg);
			else
				return B_ERROR;
		}
	}
	else
		msg->AddInt32("children", 0);
	
	return B_OK;
}

BArchivable *
XmlNode::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "XmlNode"))
		return NULL;
	return new XmlNode(data);
}


XmlNode::~XmlNode()
{
	// clean up lists and delete children
	RemoveAllAttributes();
	RemoveAllChildren();
	
	// finally, remove from parent:
	if (mParent)
		mParent->RemoveChild(this);
}

const char*
XmlNode::Name() const
{
	return mName.String();
}

void
XmlNode::SetName(const char* name)
{
	mName = name;
	SetText(name);
}

uint32
XmlNode::Type() const
{
	return mType;
}

const char*
XmlNode::Value() const
{
	if (mType == XML_TYPE_SINGLE)
		return mData.String();
	else
		return NULL;
}

int
XmlNode::ValueAsInt() const
{
	if (mType == XML_TYPE_SINGLE)
		return atoi(mData.String());
	else
		return 0;
}

XmlNode*
XmlNode::Parent() const
{
	return mParent;
}

void
XmlNode::SetValue( const char* value)
{
	if (mType == XML_TYPE_SINGLE)
		mData = value;
}



// ----------------------------------------------------------------------
uint32
XmlNode::Attributes() const
{
	return mAttribute.CountItems();
}

const char*
XmlNode::AttributeKey(uint32 index) const
{
	if (index<0 || index>=mAttribute.CountItems())
		return NULL;
		
	XmlNode* n = (XmlNode*)mAttribute.ItemAt(index);
		
	return n->mName.String();
}

const char*
XmlNode::Attribute(uint32 index) const
{
	if (index<0 || index>=mAttribute.CountItems())
		return NULL;
		
	XmlNode* n = (XmlNode*)mAttribute.ItemAt(index);
		
	return n->mData.String();
}

const char*
XmlNode::Attribute(const char* key) const
{
	uint32 index = IndexOfAttribute(key);
	
	if (index == XMLNODE_NONE)
		return NULL;
	
	return Attribute(index);
}

int
XmlNode::AttributeAsInt(const char* key, int defaultValue) const
{
	const char *val = Attribute(key);
	if (!val)
		return defaultValue;
	else
		return atoi(val);
}

bool
XmlNode::AttributeAsBool(const char* key, bool defaultValue) const
{
	const char *val = Attribute(key);
	if (!val)
		return defaultValue;
	
	BString t(val);
	
	if (t.ICompare("true")==0 || t.ICompare("yes")==0 || t.ICompare("1")==0)
		return true;

	return false;
}



void
XmlNode::AddAttribute(const char* name, const char* value)
{
	if (!name || !value) return;
	
	int32 a = anzAttributes;
	
	XmlNode* n = NULL;
	bool found = false;
	
	// first check own children	
	for (int32 i=0; i<a; i++) {
		n = (XmlNode*)mAttribute.ItemAt(i);
		
		if (n->mName.Compare(name)==0) {
			found = true;
			break;
		}
	}
	
	if (found) {
		n->mData.SetTo(value);
	}
	else {
		XmlNode* node = new XmlNode(NULL, name);
		node->mType = XML_TYPE_SIMPLE;
		node->mData = value;
		
		mAttribute.AddItem(node);
	}
}

void
XmlNode::AddAttribute(const char* name, int value)
{
	BString temp;
	temp << value;
	AddAttribute(name, temp.String());
}

status_t
XmlNode::RemoveAttribute(uint32 index)
{
	if (index >= (uint32)anzAttribute)
		return B_BAD_INDEX;
		
	XmlNode* node = (XmlNode*)mAttribute.RemoveItem(index);
	delete node;
	
	return B_OK;
}

status_t
XmlNode::RemoveAttribute(const char* key)
{
	uint32 idx = IndexOfAttribute(key);
	if (idx != XMLNODE_NONE)
		return RemoveAttribute(idx);
	
	return B_NAME_NOT_FOUND;
}

void
XmlNode::RemoveAllAttributes()
{
	void* attr;
	for (int32 i=0; (attr = mAttribute.ItemAt(i)); i++)
		delete attr;
		
	mAttribute.MakeEmpty();
}

uint32
XmlNode::IndexOfAttribute(const char* name) const
{
	int32 a = anzAttributes;
	
	for (int32 i=0; i<a; i++) {
		XmlNode* n = (XmlNode*)mAttribute.ItemAt(i);
		
		if (n->mName.Compare(name)==0)
			return i;
	}
	
	return XMLNODE_NONE;
}

// ----------------------------------------------------------------------

uint32
XmlNode::Children() const
{
	return mChild.CountItems();
}

void
XmlNode::AddChild(XmlNode* Child, int pos)
{
	if (!Child)
		return;
		
	Child->mParent = this;
	mType	= XML_TYPE_NODE;
	
	if (pos >= 0 && pos < anzChildren)
		mChild.AddItem(Child, pos);
	else	
		mChild.AddItem(Child);
}

void
XmlNode::RemoveChild(uint32 index)
{
	mChild.RemoveItem(index);
}

void
XmlNode::RemoveChild(XmlNode* me)
{
	mChild.RemoveItem(me);
}

void
XmlNode::RemoveAllChildren()
{
	void* attr;
	for (int32 i=0; (attr = mChild.ItemAt(i)); i++)
		delete attr;
		
	mChild.MakeEmpty();
}

XmlNode*
XmlNode::DetachChild(uint32 index)
{
	uint32 anz = anzChildren;
	if (index >= anz)
		return NULL;
	
	XmlNode* node = (XmlNode*)mChild.ItemAt(index);
	node->mParent = NULL;
	mChild.RemoveItem(node);
	return node;
}

XmlNode*
XmlNode::ItemAt(uint32 index) const
{
	return (XmlNode*)mChild.ItemAt(index);
}

uint32
XmlNode::IndexOf(XmlNode* child) const
{
	for (int a = anzChildren, i=0; i<a; i++) {
		if (mChild.ItemAt(i) == child)
			return i;	
	}
	
	return XMLNODE_NONE;
}

XmlNode*
XmlNode::FindChild(const char* name, XmlNode* prev, bool recursive)
{
	XPRINT(1,("XmlNode::FindChild(%s,%i,%i) within '%s'\n", name, (prev!=NULL), (recursive), myName ));
	
	if (prev && recursive) {
		XPRINT(1,("XmlNode::FindChild(). Can't FindNext recursivly!\n"));
		return NULL;
	}
	
	//
	int32 a = mChild.CountItems();
	int32 b = 0;
	
	if (prev) {
		b = mChild.IndexOf(prev);
		XPRINT(1,("Prev: %li = #%i\n", prev, b));
		if (b<0) {
			XPRINT(1, ("Xml: There is no such predecessor!\n"));
			return NULL;
		}
		
		// not actual, but next:
		b++;
	}
	
	// first check own children	
	for (int32 i=b; i<a; i++) {
		XmlNode* n = ItemAt(i);
		
		XPRINT(1,("Teste Kind #%i (%s)\n",i,n->Name() ));
		
		if (n->mName.Compare(name)==0)
			return n;
	}
	

	// check greatchildren
	if (recursive) {
		XPRINT(1,("Going recursive...\n"));
		
		for (int32 i=0; i<a; i++) {
			XmlNode* n = ItemAt(i);
			XPRINT(1,("Verzweige zu %i (%s)\n",i,n->Name() ));
			
			n = n->FindChild(name,NULL,true);
			if (n)
				return n;
		}
	}
	
	// no matching child found
	return NULL;
}


const char*
XmlNode::Parse(const char* buf)
{
	SkipW(buf);
		
	if (!buf || !*buf) {
		XPRINT(2,("Xp: 1"));
		return NULL;
	}
	
	if (*buf=='<') {
		// New tag
	
		if (*(buf+1)=='!') {
			if (*(buf+2)=='-' && *(buf+3)=='-') {
				XPRINT(2,("comment"));
				p = buf+4;
				buf+=5;
				while (*buf && !(*buf=='>' && *(buf-1)=='-' && *(buf-2)=='-'))
					buf++;
				
				mType = XML_TYPE_COMMENT;
				SetName("");
				mData.SetTo(p,buf-p-3);
				return buf+1;
			}
			else if (memcmp(buf,"<![CDATA[",9)==0) {
				XPRINT(2,("CDATA"));
				p = buf+9;
				buf+=5;
				while (*buf && !(*(buf-2)==']' && *(buf-1)==']' & *(buf)=='>'))
					buf++;
				
				mType = XML_TYPE_STRING;
				SetName("CDATA");
				mData.SetTo(p,buf-p-2);
				return buf+1;
			}
		}	
	
		XPRINT(2,("New Item to cat '%s'\n",mParent->myName));

		mType = XML_TYPE_NODE;
		buf++;
		p = buf;
		while (*buf && (*buf!='>' && *buf!=' ' && *buf!='\n'))
			buf++;
		if (!*buf) {
			XPRINT(2,("Xp: 2"));
			return NULL;
		}
		
		mName.SetTo(p,buf-p);
		mName.RemoveAll("\r");
		XPRINT(2,("New Item is '%s'\n", myName ));
		SetText(mName.String());
		
		int attrs = 0;
		
		if (*buf!='>') {
			// attributes!
			int brs = 1;
			// skip attributes (for now)
			p = buf;
			while (*buf) {
				if (*buf=='<')
					brs++;
				else if( *buf=='>' ) {
					brs--;
					if (brs==0)
						break;
				}
				
				buf++;
			}

			if (!*buf) {
				XPRINT(2,("Xp: 3"));
				return NULL;
			}
			
			BString d;
			d.SetTo(p,buf-p);
			attrs = ParseAttributes( d.String() );
		}
		
		
		if (*(buf-1)=='/') {
			if (*(buf) && *(buf)=='>') {
				XPRINT(2,("Single Item"));
				mType = XML_TYPE_SINGLE;
				return buf+1;
			}
			else
				return NULL;
		}
		else // >
			buf++;

		// Special tags:		
		if (mName.Compare("?xml",4)==0) {
			mType = XML_TYPE_SINGLE;
			mData = "";
			
			if (attrs>0) {
				const char* encstr = Attribute("encoding");;
				// shortcut:
				if (!encstr)
					return buf;

				// yes, we have an encoding string
				//printf("Document encoding is '%s'\n",encstr);
				
				BString d(encstr);
				
				//printf("Encoding is %s\n", encstr);
				
				if (d.ICompare("iso-8859-",9)==0) {
					int mode = atoi(encstr+9);
					//printf("That makes ISO-8859-'%d'\n", mode);
					if (mode<11)
						encoding = B_ISO1_CONVERSION + mode - 1;
					else if (mode>12)
						encoding = B_ISO13_CONVERSION + mode - 13;
					else
						encoding = XML_ENCODING_NONE;					
				}
				else if (d.ICompare("EUC-JP")==0)
					encoding = B_EUC_CONVERSION;
				else if (d.ICompare("EUC-KR")==0)
					encoding = B_EUC_KR_CONVERSION;					
				else if (d.ICompare("Shift_JIS")==0)
					encoding = B_SJIS_CONVERSION;					
				else if (d.ICompare("windows-1252")==0)
					encoding = B_MS_WINDOWS_CONVERSION;
				else 
					encoding = XML_ENCODING_NONE;
				
				//printf("Var encoding set to: %d\n", encoding);
				//B_SJIS_CONVERSION
			}
						
			return buf;
		}
		
		if (mName.Compare("!DOCTYPE",4)==0) {
			mType = XML_TYPE_SINGLE;
			mData = "";
			return buf;
		}		
		
		
		// add children if any
		SkipW(buf);
		while (*buf && *(buf+1) && !(*buf=='<' && *(buf+1)=='/')) {
			XmlNode* c = new XmlNode(this);
			mChild.AddItem(c);
			
			buf = c->Parse(buf);
			if (!buf) {
				return NULL;
			}
			SkipW(buf);
			XPRINT(2,("This item '%s' has now %li children\n", myName, Children() ));
		}
		
		if (!buf || !*buf || !*(buf+1)) {
			XPRINT(2,("Xp: Parse Error (77)"));
			return NULL;
		}
		XPRINT(2,("Done children for %s\n", myName));
		
		if (Children() == 0) {
			mType = XML_TYPE_SINGLE;
			mData = "";
		}
		else if (Children() == 1) {
			// Merge
			XmlNode* c = (XmlNode*)mChild.ItemAt((int32)0);
			if (c->mType == XML_TYPE_STRING) {
				XPRINT(2,("Merged String-Child into parent\n"));
				mData = c->mData;
				delete c;
				mChild.MakeEmpty();
				mType = XML_TYPE_SINGLE;
			}
		}
		else {
		}

		// lazy find end of closing tag:
		p = buf;
		while (*buf && *buf!='>')
			buf++;
		if (!*buf) {
			XPRINT(2,("Xp: 4\n"));
			return NULL;
		}
		buf++;
		
		XPRINT(2,("End of new Item '%s'\n", myName));
		
		SkipW(buf);
		
		// this node is done.
	}
	else {
		XPRINT(2,("New String: "));
		mType = XML_TYPE_STRING;
		p = buf;
		while (*buf && (*buf!='<'))
			buf++;
		if (!*buf) {
			XPRINT(2,("Xp: 4\n"));
			return NULL;
		}
		
		int32 len = buf-p;
		

		mData.SetTo(p,len);
		mData.ReplaceAll('\n', ' ');
		mData.RemoveAll("\r");
		mData.ReplaceAll('\t', ' ');
		
		while (mData.FindFirst("  ")!=B_ERROR)
			mData.ReplaceAll("  ", " ");

#ifndef OPTIONS_NO_CHARSET_CONVERSION
		int32 targetlen = 2*len;
		
		if (encoding!=XML_ENCODING_NONE) {
			char* buf = (char*)malloc( targetlen );
			int32 state = 0;
			if (convert_to_utf8( encoding, mData.String(), &len, buf, &targetlen, &state ) == B_OK) {
				mData.SetTo(buf,targetlen);
			}
			free(buf);
		}
#endif
		// Replace the remaining HTML-entities:
		mData.ReplaceAll("&quot;", "\"");
		mData.ReplaceAll("&#8222;", "\"");	// DieZeit schmeisst mit HTML-Entities um sich...
		mData.ReplaceAll("&#8220;", "\"");
		mData.ReplaceAll("&#039;", "'");
		mData.ReplaceAll("&#8211;", "--");
		
		mData.ReplaceAll("&lt;", "<");
		mData.ReplaceAll("&gt;", ">");
		mData.ReplaceAll("&apos;", "'");
		
		mData.ReplaceAll("&#228;", "ä");	// Danke an SWR3
		mData.ReplaceAll("&#246;", "ö");
		mData.ReplaceAll("&#252;", "ü");
		mData.ReplaceAll("&#223;", "ß");
		
		mData.ReplaceAll("&#196;", "Ä");	// Golem...
		mData.ReplaceAll("&#214;", "Ö");
		mData.ReplaceAll("&#220;", "Ü");
	
				
		/*
		mData.ReplaceAll("&#8211;", "--");
		mData.ReplaceAll("&#8211;", "--");
		*/
		
		// VERY LAST ONE:
		mData.ReplaceAll("&amp;", "&");		// does not work always...
		
		// this node is done.
		XPRINT(2,("'%s'\n", mData.String() ));
	}
	
	// done:	
	return buf;
}


void
XmlNode::Display() const
{
	for (int32 i=0, a=Children();i<a;i++)
		ItemAt(i)->Display(0);
}


// cases to check for:
// 1.   key=value
// 2.   key="value"
// 3.   key='value'
// 4.	key
// keys will be XmlNodes with Type XML_TYPE_SIMPLE.
// key => Name()
// value => Value()
int
XmlNode::ParseAttributes(const char* buf)
{
	XPRINT( 3, ("Parse Attribute zu item '%s'\n", myName) );

	int anz = 0;
	
	const char* p;
	
	SkipW(buf);
	
	while (*buf && *buf!='?') {
		XPRINT( 3, ("Neues Attribut\n") );
		
		p=buf;
		
		// find key
		while (*buf && !(*buf=='=' || *buf==' ' || *buf=='?'))
			buf++;
			
		if (!*buf) {
			return B_ERROR;
		}
		
		XmlNode* attr = new XmlNode(this);
		attr->mType = XML_TYPE_SIMPLE;
		
		attr->mName.SetTo(p,buf-p);
		attr->SetText(attr->Name());
		XPRINT( 3, ("  Attr.Name = '%s'\n", attr->mName.String() ) );
		
		if (*buf == '=') {
			buf++; // skip =
			
			if (*buf=='\"' || *buf=='\'') {
				char esc = *buf;
				buf++;
			
				p=buf;
				while (*buf && *buf!=esc)
					buf++;
				
				if (!*buf) {
					delete attr;
					return B_ERROR;
				}
			
				attr->mData.SetTo(p,buf-p);
				
				// finally skip quote-char:
				buf++;
			}
			else {
				p=buf;
				while (*buf && !(*buf==' ' || *buf=='?'))
					buf++;

				if (!*buf) {
					delete attr;
					return B_ERROR;
				}	

				attr->mData.SetTo(p,buf-p);
			}
	
			XPRINT( 3, ("  Attr.Value = '%s'\n", attr->mData.String() ) );
		}
		else
			XPRINT( 3, ("  Attr.Value ist leer.\n") );

		mAttribute.AddItem(attr);
		anz++;
		
		SkipW(buf);
	}	
	//puts("Parsing attributes... to do");
	
	return anz;
}



void
XmlNode::Display(int level) const
{
	for (int i=0;i<level;i++)
		printf("\t");
	
	if (mType == XML_TYPE_NODE) {
		if (Attributes()==0)
			printf("<%s>\n", mName.String() );
		else {
			printf("<%s\n", mName.String() );

			for (int32 i=0, a=Attributes();i<a;i++) {
				for (int ti=0;ti<level+1;ti++)
					printf("\t");
				
				XmlNode* n = (XmlNode*)mAttribute.ItemAt(i);
				printf("%s = %s\n", n->Name(), n->Value());
			}
			for (int ti=0;ti<level;ti++) printf("\t");
			printf(">\n");
		}
		
	
		for (int32 i=0, a=Children();i<a;i++)
			ItemAt(i)->Display(level+1);

		for (int i=0;i<level;i++)
			printf("\t");
		printf("</%s>\n", mName.String() );
	}
	else if (mType == XML_TYPE_SINGLE) {
		if (Attributes()==0)
			printf("<%s", mName.String() );
		else {
			printf("<%s\n", mName.String() );

			for (int32 i=0, a=Attributes();i<a;i++) {
				for (int ti=0;ti<level+1;ti++) printf("\t");
				
				XmlNode* n = (XmlNode*)mAttribute.ItemAt(i);
				printf("%s = %s\n", n->Name(), n->Value());
			}
			for (int ti=0;ti<level;ti++) printf("\t");
		}
		
		if (mData.Length()>0)
			printf(">%s</%s>\n", mData.String(), myName );
		else
			printf("/>\n");
	}
	else if (mType == XML_TYPE_COMMENT)
		printf("<!--|%s|-->\n", mData.String() );
}


bool
XmlNode::SaveToFile(const char* filename) const
{	
	int ref = creat(filename, 0666);
	if (ref < 0) {
		perror("open");
		return false;
	}

	BString dummy("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n\n");
	write(ref, dummy.String(), dummy.Length());
	
	int c = Children();	
	for (int i=0; i<c; i++) {
		ItemAt(i)->SaveNode(ref, 0);
	}
	
	//dummy = "</xml>\n";
	//write(ref, dummy.String(), dummy.Length());
	
	close(ref);
	return true;
}

#define TabS( depth ) s.Prepend( '\t', depth )

bool
XmlNode::SaveNode(int ref, int depth) const
{
	BString s; 			// s is reused later!
	TabS(depth);
	
	if (mType == XML_TYPE_COMMENT) {
		s << "<!-- " << mData.String() << " -->\n";
		write(ref, s.String(), s.Length());
		
		return true;
	}

	// Name
	s << "<" << Name();
	write(ref, s.String(), s.Length());	
		
	// attributes
	int a = Attributes();
	if (a>0) {
		for (int i=0; i<a; i++) {
			XmlNode *aa = (XmlNode*)mAttribute.ItemAt(i);
			s = " ";
			s << aa->Name() << "=\"" << aa->Value() << "\"";
			write(ref, s.String(), s.Length());	
		}
	}
	
	// Traverse children
	if (mType == XML_TYPE_NODE) {
	
		int c = Children();
		if (c==0) {
			s = " />\n";
			write(ref, s.String(), s.Length());
		}
		else {
			s = ">\n";
			write(ref, s.String(), s.Length());
			
			for (int i=0; i<c; i++) {
				ItemAt(i)->SaveNode(ref, depth+1);
			}
	
			// closing tag:
			s.SetTo("");
			TabS(depth);
			s << "</" << Name() << ">\n";
			write(ref, s.String(), s.Length());
		}
	}
	else if (mType == XML_TYPE_SINGLE) {
		if (mData.Length()==0) {
			s = " />\n";
			write(ref, s.String(), s.Length());
		}
		else {
			s = ">";
			s << mData.String() << "</" << myName << ">\n";
			write(ref, s.String(), s.Length());
		}
	}
	
	return true;
}

bool
XmlNode::LoadFile(const char* filename)
{
	printf("Loading file: %s\n", filename);

	// convert filename into actual handle
	BEntry entry(filename, true);
	BFile node(&entry, B_READ_ONLY);
	if (node.InitCheck() != B_OK)
		return false;

	int filesize;
	node.GetSize((off_t*)&filesize);
	char *bbuf = (char*)calloc(1,filesize+1);
		
		
	if (node.Read(bbuf, filesize) < filesize) {
		node.Unset();
		free(bbuf);
		return false;
	}		
	node.Unset();
	
	puts("File loaded to buffer");
	
	const char* buf = bbuf;
	
	mType	= XML_TYPE_NODE;
	
	// root object does not parse itself;
	while (buf && *buf) {
		SkipW( buf );
		XmlNode* c = new XmlNode(this);
		mChild.AddItem(c);
		
		buf = c->Parse(buf);
	}
	
	free(bbuf);
	
	if (!buf) {
		puts("parse error");
		return false;
	}
	else {
		puts("XmlNode::Load: ok");
		return true;
	}
}



XmlNode*
XmlNode::GetChild(const char* path)
{
	BString d(path);
	BString e(path);
	bool recurse = false;
	
	int n = d.FindFirst('/');
	if (n != B_ERROR) {
		e.Remove(n, e.Length()-n);
		d.Remove(0,n+1);
		
		recurse = true;
	}
	
	int anz = Children();
	
	for (int i=0; i<anz; i++) {
		XmlNode* node = ItemAt(i);
		
		if (e.Compare(node->Name())==0) {
			if (recurse)
				return node->GetChild(d.String());
			else
				return node;
		}
	}

	// no matching child:	
	return NULL;
}
	
XmlNode*
XmlNode::CreateChild(const char* path, const char* value)
{
	BString d(path);
	BString e(path);
	bool recurse = false;
	
	int n = d.FindFirst('/');
	if (n != B_ERROR) {
		e.Remove(n, e.Length()-n);
		d.Remove(0,n+1);
		
		recurse = true;
	}
	
	int anz = Children();
	
	for (int i=0; i<anz; i++) {
		XmlNode* node = ItemAt(i);
		
		if (e.Compare(node->Name())==0) {
			if (recurse)
				return node->CreateChild(d.String(), value);
			else {
				if (value)
					node->SetValue(value);
				return node;
			}
		}
	}

	// no matching child:	
	if (recurse) {
		XmlNode* node = new XmlNode(e.String());
		AddChild(node);

		return node->CreateChild(d.String(), value);
	}
	else {
		XmlNode* node = new XmlNode(path);
		AddChild(node);
		
		if (value)
			node->SetValue(value);
		
		return node;
	}
}

XmlNode*
XmlNode::CreateChild(const char* path, int value)
{
	BString temp;
	temp << value;
	return CreateChild(path, temp.String());
}
	
bool
XmlNode::RemoveChild(const char* path)
{
	return false;
}


XmlNode*
XmlNode::Duplicate()
{
	XmlNode* node = new XmlNode(mParent, Name(), OutlineLevel(), IsExpanded());
	node->mType = mType;
	node->mData = mData;
	node->mMarked = mMarked;
	node->SetText(Text());

	for (int i=0, a=anzAttributes; i<a; i++) {
		XmlNode* child = ((XmlNode*)mAttribute.ItemAt(i))->Duplicate();
		node->mAttribute.AddItem(child);
	}
	
	for (int i=0, c=anzChildren; i<c; i++) {
		XmlNode* child = ((XmlNode*)mChild.ItemAt(i))->Duplicate();
		node->mChild.AddItem(child);
	}
	
	return node;
}


void
XmlNode::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color saveH = owner->HighColor();
	rgb_color saveL = owner->LowColor();
	rgb_color colorB, colorF;
	
	rgb_color hui;
	if (mMarked) {
		hui.red=64; hui.green=64; hui.blue=160; hui.alpha=0;
	}
	else
		hui = owner->HighColor();

	if (IsSelected()) {
		colorB = hui;
		colorF = owner->ViewColor();
	}
	else {
		colorB = owner->ViewColor();
		colorF = hui;
	}

	if (IsSelected() || complete) {
		owner->SetHighColor(colorB);
		owner->FillRect(frame);
	}


	owner->MovePenTo(frame.left+4, frame.bottom-2);
	
	if (mMarked) {
		owner->SetHighColor(colorF);
		owner->SetLowColor(colorB);
		
		BFont saveFont;
		owner->GetFont(&saveFont);
		owner->SetFont(be_bold_font);
		
		owner->DrawString(Text());
		owner->SetFont(&saveFont);
	}	
	else {
		owner->SetHighColor(colorF);
		owner->SetLowColor(colorB);
		owner->DrawString(Text());
	}
	
	// restore Highcolor:
	owner->SetHighColor(saveH);
	owner->SetLowColor(saveL);
}


void
XmlNode::SetMarked(bool mark)
{
	mMarked = mark;
}

bool
XmlNode::Marked()
{
	return mMarked;
}

void
XmlNode::Comment(const char* commentstring)
{
	RemoveAllChildren();
	RemoveAllAttributes();
	
	mType = XML_TYPE_COMMENT;
	mData = commentstring;
}
