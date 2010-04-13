#ifndef FSTRINGITEM
#define	FSTRINGITEM

#include "fr_def.h"

class FStringItem : public BStringItem
{
public:
	FStringItem();
	FStringItem(const char *text, const char *url, uint32 level=0, bool expanded = true, bool addDescFlag = false);
	FStringItem(FStringItem& fi);
	
	const char* Title() const;
	const char* Url() const;
	const char* Desc() const;
	const char*	Date() const;
	bool		Visited() const;
	bool		IsAddDesc() const;
	
	void		SetUrl(const char* url);
	void		SetDesc(const char* desc);
	void		SetDate(const char* date);
	void		SetAddDesc(bool add=true);
	void 		SetVisited(bool vis = true);
	
	virtual void DrawItem(BView* owner, BRect frame, bool complete = false);

	BString		sUrl;
	BString		sDesc;
	
private:
	bool		bAddDesc;
	bool		visited;
	
	BString		sDate;
};

int compare_func(const BListItem* firstArg, const BListItem* secondArg);
int compare_func(const void* firstArg, const void* secondArg);

#endif
