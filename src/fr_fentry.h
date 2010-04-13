#include "fr_def.h"

class FEntry : public BStringItem
{
public:
	FStringItem(const char *text, const char *url, uint32 level=0, bool expanded = true) :
		BStringItem(text, level, expanded),
		sUrl(url)
	{
	}

private:
	BString sUrl;
	BString sDesc;
};
