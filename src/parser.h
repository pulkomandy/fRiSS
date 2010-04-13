#include "fr_options.h"

#include "xmlnode.h"

// RSS / RDF (xml)
int Parse_rss( XmlNode* root, BList* list, BString& status, bool& updatesFeedList, bool addDesc=false);

#ifndef OPTIONS_NO_ICS
// iCal / Sunbird .ICS
int Parse_ics( char* buf, BList* list, BString& status, bool& updatesFeedList);
#endif

#ifndef OPTIONS_NO_ATOM
int Parse_atom( XmlNode* root, BList* list, BString& status, bool& updatesFeedList, bool addDesc=false);
#endif

