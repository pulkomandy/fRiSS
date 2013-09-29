#include "fr_options.h"

#include "xmlnode.h"

// RSS / RDF (xml)
int Parse_rss( XmlNode* root, BObjectList<FStringItem>* list, BString& status, bool addDesc=false);

#ifndef OPTIONS_NO_ICS
// iCal / Sunbird .ICS
int Parse_ics( char* buf, BObjectList<FStringItem>* list, BString& status);
#endif

#ifndef OPTIONS_NO_ATOM
int Parse_atom( XmlNode* root, BObjectList<FStringItem>* list, BString& status, bool addDesc=false);
#endif

