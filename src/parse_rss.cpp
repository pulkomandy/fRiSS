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

/*	Extracts the CHANNEL and ITEM-information from XML-Tree into 
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
int Parse_rss( XmlNode* root, BObjectList<FStringItem>* list, BString& status, bool addDesc)
{
	PPUTS("BEGIN PARSE_RSS");
	XmlNode* channel = root->FindChild( "channel", NULL, true );
	
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
	
	PPRINT(( "\nNUN DIE ITEMS!\n" ));
	
	XmlNode* item = root->FindChild("item", NULL, true);
	if (!item) {
		PPUTS("File has no items");
		return -1;
	}
	
	PPRINT(("Found: %s = %i\n", item->Name(), item->Type() ));
	
	// parent is at least root
	XmlNode* items = item->Parent();
	XmlNode* p;
	
	int itemcount = 0;
	
	while (item) {
		itemcount++;
		
		
		p = item->FindChild("title");
		fi = new FStringItem();
		if (p) {
			fi->SetText(p->Value());
			PPRINT(( "Item name is '%s'\n", p->Value() ));
		}
		else
			PPUTS("Item has no title");
		
		p = item->FindChild("link");
		if (p) {
			fi->SetUrl(p->Value());
			PPRINT(( "Item link is '%s'\n", p->Value() ));
		}
		else {
			if (hasChanLink) {
				fi->SetUrl(ChanLink.String());
				PPUTS("Item has no link, we will use the Channel's link instead!");		
			}
			else
				PPUTS("Item has no link");
		}
		
		p = item->FindChild("description");
		if (p) {
			if (addDesc) {
				BString a(fi->Text());
				a << " : " << p->Value();
				fi->SetText(a.String());
			}

			// The HTML data comes XML-escaped inside the description element.
			// We have to do a second run of parsing to handle that
			XmlNode* html = new XmlNode(p->Value(), NULL);
				// FIXME this is leaked. Where can we delete it ?
				// fi could check if the node it owns is a root, and delete it
				// if so.

			if(html->Children() == 0 || (html->Children() == 1 && html->ItemAt(0)->Children() <= 1)) {
				// Not enough markup, assume textmode feed
				delete html;
				html = new XmlNode(NULL, "p");
				html->SetValue(p->Value());
			}

			fi->SetDesc(html);
		}
		else
			PPUTS("Item has no description");	
	
		list->AddItem(fi);
		
		PPUTS("");
		
		// get next item:
		item = items->FindChild("item", item);
	}
	
	PPUTS("END OF PARSE_RSS");	
	return itemcount;
}
