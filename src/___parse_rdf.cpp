#include "fr_def.h"

//#define TRACE_PARSER

#ifdef TRACE_PARSER
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif

int Parse_rdf(char *buf, int bufsize, BList* list, BString& status)
{
	FPRINT(("Parsing...\n"));
	
	buf[bufsize] = 0;
	
	char *line	= strtok(buf, "\n");
	int lnr		= 0;	// No. of lines
	int items	= 0;	// No. of items
	
	status = line;
	status.RemoveSet("\t\r");
	FPRINT(( "Status: %s\n", status.String() ));
	line += 9;
	if (strncmp(line,"404", 3)==0) {
		return -1;
	}
	if (strncmp(line,"403", 3)==0) {
		return -1;
	}	
	if (strncmp(line,"503", 3)==0) {
		return -1;
	}	
	
	int mode = 0;
	FStringItem *fi;
	
	// fast forward until <channel>
	char* ctag = "<channel";
	char *ci = ctag;
	char *b = buf;
	
	FPRINT(("FF start\n"));
	
	b += strlen(line)+10;
	do {
		//printf("%c", *b);
		//printf("%u\n", (unsigned int)(b-buf));
		if (*b == *ci) {
			ci++;
			b++;
		}
		else {
			ci = ctag;
			if (*b != *ci)
				b++;
		}
	} while (*b && *ci);
	
	// We got out the loop and have no bytes left in the buffer:
	if (!*b) {
		FPRINT(("\nFF fail\n"));
		status = "Invalid File";
		return -2;
	}
	
	// Find closing bracket (e.g. N24 has an attrib for channel-tag)
	while (*b && *b!='>')
		b++;
	
	if (!*b) {
		FPRINT(("\nFF fail\n"));
		status = "Invalid File";
		return -3;
	}
	b++;
	
	FPRINT(("\nFF ok\n"));
	buf = b;
	mode = 1;
	fi = new FStringItem();
		
	line = strtok(buf, "\n");
	
	
	BString ChannelLink;
	
	while (line) {
		lnr++;	
		
		//FPRINT(("Vorher:\t%s\n",line));
		
		while (*line==' ' || *line=='\t')
			line++;
			
		BString lst(line);
		lst.RemoveAll("\r");

		// Skip empty lines:		
		if (lst.Length()>0) {
		
			lst.IReplaceAll("\t", " ");
			lst.IReplaceAll("&quot;", "\"");
			
			// Fok! has those:
			lst.IReplaceAll("\\\"", "\"");
			lst.IReplaceAll("\\\'", "\'");
			lst.RemoveAll("<![CDATA[");
			lst.RemoveAll("]]>");
			
	//		lst.IReplaceAll( "\004", "\0xc3\0xa4" ); // ä
	//		lst.IReplaceAll( "\026", "\0xc3\0xb6"); // ö
	//		lst.IReplaceAll( "\034", "\0xc3\0xbc"); // ü
	
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x84"); // Ä
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x96"); // Ö
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x9c"); // Ü
			
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x9f"); // ß
	
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0xa4"); // é
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0xb6"); // è
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0xbc"); // ê
	
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x84"); // á
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x96"); // à
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x9c"); // â
	
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x84"); // ú
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x96"); // ù
	//		lst.IReplaceAll( "\0xXX", "\0xc3\0x9c"); // û
			
			//if (lst.Compare(line)!=0)
			FPRINT(( "%i\t%s\n", mode, lst.String() ));
			
			switch (mode) {
				case 0: // Suche Title
					if (lst.Compare("<channel>",9)==0) {
						fi = new FStringItem();
						mode++;
					}
					break;
				case 1: // Parse Title
					if (lst.Compare("<title>",7)==0) {
						lst.RemoveFirst("<title>");
						lst.RemoveLast("</title>");					
						fi->SetText(lst.String());
						
						FPRINT(( "TITLE: %s\n", lst.String() ));
					}
					else if (lst.Compare("<link>",6)==0) {
						lst.RemoveFirst("<link>");
						lst.RemoveLast("</link>");				
						fi->SetUrl(lst.String());
						ChannelLink = lst;
						
						FPRINT(( "LINK: %s\n", lst.String() ));
					}
					else if (lst.Compare("<description>",13)==0) {
						lst.RemoveFirst("<description>");
						lst.RemoveLast("</description>");
						fi->SetDesc(lst.String());
					}
					else if (lst.Compare("</channel>",10)==0) {
						list->AddItem(fi);
						mode++;
					}
					else if (lst.Compare("<item>",6)==0 || lst.Compare("<item ",6)==0) {
						// Channel abschließen:
						list->AddItem(fi);
						
						// RSS hat Items in Channel gepackt
						mode = 3;
						fi = new FStringItem();
						fi->SetUrl(ChannelLink.String());
						
						FPRINT(( "NEUES ITEM\n" ));
					}
					else {
						//FPRINTf("  CHANNEL.???: '%s'\n", line);
					}
					
					break;
				
				case 2: // Suche Item
					if (lst.Compare("<item>",6 || lst.Compare("<item ",6)==0)==0) {
						mode++;
						fi = new FStringItem();
						fi->SetUrl(ChannelLink.String());
						
						FPRINT(( "NEUES ITEM\n" ));
					}
					break;
				
				case 3: // Parse Item
					if (lst.Compare("<title>",7)==0) {
						lst.RemoveFirst("<title>");
						lst.RemoveLast("</title>");
						//puts(lst.String());
						fi->SetText(lst.String());
					}
					else if (lst.Compare("<link>",6)==0) {
						lst.RemoveFirst("<link>");
						lst.RemoveLast("</link>");				
						fi->SetUrl(lst.String());
					}
					else if (lst.Compare("<description>",13)==0) {
						lst.RemoveFirst("<description>");
						lst.RemoveLast("</description>");
						fi->SetDesc(lst.String());
					}
					else if (lst.Compare("</item>",7)==0) {
						list->AddItem(fi);
						mode--;
						items++;
					}
					else {
						//FPRINTf("  ITEM.???: %s\n", line);
					}
					
					break;	
					
				default:
					//FPRINTf("Zeile %d: %s\n", lnr, line);
					break;
			}
		}
		
		// Get next line:		
		line = strtok(NULL, "\n");
	}
	
	FPRINT(( "Lines: %i\nItems: %i\n", lnr,items ));
	
	return lnr;
}
