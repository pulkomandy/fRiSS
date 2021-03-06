#include "fr_options.h"

#ifndef OPTIONS_NO_ATOM

#include "fr_def.h"
#include "xmlnode.h"
#include "fr_fstringitem.h"

#if 0
	#define PPRINT(x) printf x
	#define PPUTS(x) puts(x)
#else
	#define PPRINT(x)
	#define PPUTS(x)
#endif

/*	Extracts the FEED and ITEM-information from XML-Tree into 
 *	a BList of XmlNodes.
 *
 *	XmlNode* root		the tree
 *	BList* list			the target list
 *	BString& status		a good place for error messages
 *
 *	returns
 *		<0				on error
 *		0				no items in tree (sort of error)
 *		>0				items in list
 */
int Parse_atom( XmlNode* root, BObjectList<FStringItem>* list, BString& status,
	bool addDesc)
{
	PPUTS("BEGIN PARSE_ATOM");
	XmlNode* channel = root->FindChild( "feed", NULL, true );
	
	if (!channel) {
		status = "Error: Invalid File. File does not have <channel>";
		PPUTS("ERROR PARSE_RSS: can't find channel");
		return -1;
	}
	
	/*
	puts("===============");
	root->Display();
	puts("===============");
	*/
	
	FStringItem* fi;
	
	BString ChanLink;
	bool	hasChanLink = false;

#if 0
	{
		XmlNode* p = channel->FindChild("title");
		fi = new FStringItem();
		if (p)
			fi->SetText(p->Value());
		else
			PPUTS("Feed has no title");
		
		p = channel->FindChild("link");
		if (p) {
			fi->SetUrl(p->Value());
			ChanLink = p->Value();
			hasChanLink = true;
		}
		else
			PPUTS("Feed has no link");
			
		p = channel->FindChild("tagline");
		if (p)
			fi->SetDesc(p);
		else
			PPUTS("Feed has no tagline");
	
		list->AddItem(fi);
		PPUTS("");
	}
#endif
	
	PPRINT(( "\nNUN DIE ITEMS!\n" ));
	
	XmlNode* item = root->FindChild("entry", NULL, true);
	if (!item) {
		PPUTS("File has no items");
		return -1;
	}
	
	PPRINT(("Found: %s = %i\n", item->Name(), item->Type() ));
	
	// parent is at least root
	XmlNode* items = item->Parent();
	XmlNode* p = NULL;
	
	int itemcount = 0;
	
	while (item) {
		itemcount++;
		
		fi = new FStringItem();
		
		p = item->FindChild("title");
		if (p) {
			if(p->Attribute("type") && strcmp(p->Attribute("type"),"html") == 0)
				fi->SetTitleHtml(BString(p->Value()));
			else
				fi->SetText(p->Value());
			PPRINT(( "Item name is '%s'\n", p->Value() ));
		}
		else
			PPUTS("Item has no title");
		
		p = item->FindChild("link");
		if (p) {
			fi->SetUrl(p->Attribute("href"));
			PPRINT(( "Item link is '%s'\n", p->Attribute("href") ));
		}
		else {
			if (hasChanLink) {
				fi->SetUrl(ChanLink.String());
				PPUTS("Item has no link, we will use the Channel's link instead!");		
			}
			else
				PPUTS("Item has no link");
		}
		
		p = item->FindChild("content");
		if (p) {
			if (addDesc) {
				BString a(fi->Text());
				a << " : " << p->Value();
				fi->SetText(a.String());
			}
			else {
				const char* type = p->Attribute("type");
				if(strcmp(type, "html") == 0) {
					// html: not XML conformant, so it is escaped in the feed.
					// Need to parse it, as XML will do for now.
					XmlNode* html = new XmlNode(p->Value(), NULL);
					fi->SetDesc(html); // FIXME memory leak
				} else if(strcmp(type, "xhtml") == 0) {
					fi->SetDesc(p->ItemAt(0)); // Get the div inside
				} else {
					// plaintext: can be handled directly
					fi->SetDesc(p);
				}
			}
		}
		else
			PPUTS("Item has no description");	
	
		list->AddItem(fi);
		
		PPUTS("");
		
		// get next item:
		item = items->FindChild("entry", item);
	}
	
	PPUTS("END OF PARSE_ATOM");	
	return itemcount;
}


#endif
