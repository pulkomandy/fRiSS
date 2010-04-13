#ifndef NLANG_CPP
//----------------------------------------------------
#define NLANG_CPP

#include "nlang.h"

#ifdef OPTIONS_USE_NLANG

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <be/support/String.h>

#include <be/StorageKit.h>


#ifndef SkipW
	#define SkipW( x ) 	while (x && *x && (*x=='\r' || *x=='\n' || *x=='\t' || *x==' ')) x++;
#endif

NLang::NLang() :
	list(NULL, "Language"),
	mPath("./Language/Dictionaries/")
{
	 
}

NLang::~NLang()
{
}


void
NLang::Init(const char* path)
{
	BEntry ent(path, true);
	BPath p;
	ent.GetPath(&p);
	
	printf("Language directory is %s\n", p.Path());
	
	mPath = p.Path();
}

const char*
NLang::Translate(const char* str)
{
	const char* s = list.Attribute(str);
	
	if (s) {
		//printf("(NLANG) Found string: %s = %s\n", str, s);
		return s;
	}
	else {
		//printf("(NLANG) String not found: %s\n", str);
		return str;
	}
}

/*
void
TestLang(XmlNode* list)
{
	int a = list->Attributes();
	for (int i=0; i<a; i++) {
		printf("(%02d) %s = %s\n", i, list->AttributeKey(i), list->Attribute(i));
	}
}
*/

bool
NLang::LoadFile(const char* filename)
{
	list.RemoveAllAttributes();
	
	if (!filename || (filename[0]=='.' && filename[1]==0)) {
		//puts("default ok");
		return true;
	}
	
	return (load_lang_file(filename));
}

bool
NLang::LoadFileID(const char* langID)
{
	list.RemoveAllAttributes();
	
	BEntry ent(mPath.String(), true);
	BDirectory dir(&ent);
	
	BEntry entry;
	while (dir.GetNextEntry(&entry, true) == B_OK) {
		BPath path(&entry);
		BString name(path.Leaf());
		name.Remove(0, name.Length()-4);
		
		if (name.Compare(langID) == 0) {
			// filename matches *.langID
			
			if (load_lang_file(path.Path())) {
				return true;
			}
		}
	}
	
	// no match:
	//puts("(NLANG) fail (2)");
	return false;
}


bool
NLang::load_lang_file(const char* name)
{
	// convert filename into actual handle
	BEntry entry(name, true);
	BFile node(&entry, B_READ_ONLY);
	if (node.InitCheck() != B_OK)
		return false;

	int filesize;
	node.GetSize((off_t*)&filesize);
	char *buf = (char*)calloc(1,filesize+1);
		
		
	if (node.Read(buf, filesize) < filesize) {
		node.Unset();
		free(buf);
		return false;
	}	

	
	char *b = buf, *bb;
	
	while (*b) {
		BString key, value;

		SkipW(b);
		
		while (*b == '#') {
			while (*b!='\n')
				b++;
			
			SkipW(b);
		}
		if (!b) {
			printf("EOF.\n");
			free(buf);
			return true;
		}
		b++; // Anf.z.
		
		bb = b;
		while (*bb && *bb!='\"')
			bb++;
		
		if (!bb) {
			printf("EOF?!\n");
			free(buf);
			return false;
		}
		
		key.SetTo(b,bb-b);
		
		b = bb+1;
		SkipW(b);
		b++;
		
		bb = b;
		while (*bb && *bb!='\"')
			bb++;
		if (!bb) {
			printf("EOF?!\n");
			free(buf);
			return false;
		}
			
		value.SetTo(b,bb-b);
		b = bb+1;
		
		//printf("Zeile:\t'%s' => '%s'\n", key.String(), value.String());
		list.AddAttribute(key.String(), value.String());
		
		SkipW(b);
	}
	
	node.Unset();
	
	free(buf);
	return true;
}


void
NLang::BuildLangMenu(BMenu* menu, const char* current)
{
	BMenuItem* mi;
	
	menu->SetRadioMode(true);
	
	BMessage *defmsg = new BMessage(CMD_LANG_LOAD);
	defmsg->AddString("filename", ".");
	menu->AddItem( mi = new BMenuItem( Translate("Default"), defmsg));
	if (current && (current[0]=='.' || current[0]==0)) {
		mi->SetMarked(true);
	}	
	
	menu->AddSeparatorItem();
		
	BDirectory dir(mPath.String());
	if (dir.InitCheck() != B_OK) {
		printf("%s : Directory %s is wrong ?\n",__func__,mPath.String());
		return;
	}
	BEntry entry;
	int files=0;
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path(&entry);
		BString name(path.Leaf());
		BString lang(path.Leaf());
		
		//printf("\tFound: %s\n", name.String());
		files++;		
		
		int l = name.FindLast('.');
		name.Remove(l, name.Length()-l);
		lang.Remove(0, l+1);
		
		BMessage *msg = new BMessage(CMD_LANG_LOAD);
		msg->AddString("filename", path.Path());
		mi = new BMenuItem(name.String(), msg );
		menu->AddItem( mi );
		if (current) {
			//printf("\t%s =?= %s , %d\n", lang.String(), current, l);
			if (lang.Compare(current)==0) {
				mi->SetMarked(true);
			}
		}
	}
	
	//printf("\t%d files\n", files);
}


NLang no_locale;



#endif
#endif
