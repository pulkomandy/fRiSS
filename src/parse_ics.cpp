#include "fr_options.h"

#ifndef OPTIONS_NO_ICS
// ICS can be disabled

#include "fr_def.h"

#include "fr_fstringitem.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

//#define TRACE_PARSER
#ifdef TRACE_PARSER
	#define FPRINT(x) printf x
#else
	#define FPRINT(x)
#endif


#ifndef Error
#define Error(x) {status=x;return -1;}
#endif

int compare_func_date(const FStringItem* a, const FStringItem* b)
{
	assert(a != NULL && b != NULL);

	time_t da = a->Date();
	time_t db = b->Date();

	//assert(da != NULL && db != NULL);

	return da < db;
}

#define SKIPZ(line)										\
	do {												\
		line = strtok(NULL, "\n");						\
		lnr++;											\
		while(line && (line[0]==' ' || line[0]=='\t'))	\
			line++;										\
	} while (line && line[0]==';')


int Parse_ics( char* buf, BObjectList<FStringItem>* list, BString& status)
{
	puts("BEGIN PARSE_ICS");

	char* line;
	int	lnr = 0;
	int items = 0;
	int mode = 0;
	FStringItem* fi;


	time_t now = time(NULL);
	tm* localtm = localtime( &now );

	char today[7] = {0};
	sprintf(today, "%d%02d%02d", 1900+localtm->tm_year, localtm->tm_mon+1, localtm->tm_mday);
	printf("Jetz is '%s'\n", today);

	line = strtok(buf, "\n");

	BString Summary, DTStart, DTEnd, RRule, Description, Uid;

	while (line) {
		lnr++;

		while (*line==' ' || *line=='\t')
			line++;

		BString lst(line);
		lst.RemoveAll("\r");

		// Skip empty lines:
		if (lst.Length()>0) {

			//if (lst.Compare(line)!=0)
			FPRINT(( "%i\t%s\n", mode, lst.String() ));

			XmlNode desc(NULL, "p");
				// FIXME will be destroyed while fi still references it

			switch (mode) {
				case 0: // Suche BEGIN:VEVENT
					if (lst.Compare("BEGIN:VEVENT")==0) {
						fi = new FStringItem();
						Summary = "";
						DTStart = "";
						DTEnd = "";
						Uid = "";
						RRule = "";
						Description = "";

						FPRINT(( "NEW VEVENT\n" ));

						mode = 1;
					}
					break;
				case 1: // Parse Entry
					if (lst.Compare("SUMMARY",7)==0) {
						// get next line:
						SKIPZ(line);
						if (!line)
							Error("Invalid file (summary)\n");

						Summary.SetTo(line+1);
						FPRINT(( "Summary: %s\n", Summary.String() ));
					}
					else if (lst.Compare("DTSTART")==0) {
						// get next line:
						SKIPZ(line);
						if (!line)
							Error("Invalid file (dtstart)\n");

						DTStart.SetTo( line+1 );

						FPRINT(( "DTStart: %s\n", DTStart.String() ));
					}
					else if (lst.Compare("DTEND")==0) {
						// get next line:
						SKIPZ(line);
						if (!line)
							Error("Invalid file (dtend)\n");

						DTEnd.SetTo( line+1 );

						FPRINT(( "DTStart: %s\n", DTStart.String() ));
					}
					else if (lst.Compare("RRULE")==0) {
						// get next line:
						SKIPZ(line);
						if (!line)
							Error("Invalid file (dtend)\n");

						RRule.SetTo( line+1 );

						FPRINT(( "RRule: %s\n", DTStart.String() ));
					}
					else if (lst.Compare("DESCRIPTION")==0) {
						// get next line:
						SKIPZ(line);
						if (!line)
							Error("Invalid file (dtend)\n");

						Description.SetTo( line+1 );

						FPRINT(( "Description: %s\n", DTStart.String() ));
					}
					else if (lst.Compare("UID")==0) {
						// get next line:
						SKIPZ(line);
						if (!line)
							Error("Invalid file (dtend)\n");

						Uid.SetTo( line+1 );

						FPRINT(( "Uid: %s\n", DTStart.String() ));
					}
					else if (lst.Compare("END:VEVENT")==0) {
						// Debug
						BString title;

						long dStart = atoi(DTStart.String());
						long dEnd = atoi(DTEnd.String());

						if (dEnd-dStart > 1)
							title << DTStart << " - " << DTEnd << " : " << Summary;
						else
							title << DTStart << " : " << Summary;

						Description << "\nRULK: " << RRule << "\nUID: " << Uid;


						fi->SetText( title.String() );
						fi->SetUrl("about:blank");
						desc.SetValue(Description);
						fi->SetDesc(&desc); // Reference after delete

						//fi->SetDate(DTStart.String());

						if (DTEnd.Compare(today) < 0) {
							fi->SetVisited();
						}

						list->AddItem(fi);
						items ++;

						FPRINT(( "END OF VEVENT\n" ));

						mode = 0;
					}
					else {
						FPRINT(("  ICS: Unknown '%s'\n", line));
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

	puts("Sorting...");
	list->SortItems(&compare_func_date);

	puts("Sorted.");

	return items;
}

#endif
