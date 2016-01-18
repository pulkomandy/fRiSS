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
	const XmlNode* const Desc() const;
	time_t		Date() const;
	bool		Visited() const;
	bool		IsAddDesc() const;

	void		SetUrl(const char* url);
	void		SetDesc(const XmlNode* const node);
	void		SetDate(time_t date);
	void		SetAddDesc(bool add=true);
	void 		SetVisited(bool vis = true);

	void		SetTitleHtml(BString title);

	void		DrawItem(BView* owner, BRect frame, bool complete);

private:
	bool		bAddDesc;
	bool		visited;

	BString		sUrl;
	const XmlNode*	sDesc;
	time_t		sDate;
};

#endif
