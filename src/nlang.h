#ifndef NLANG
//----------------------------------------------------
#define NLANG

#include "fr_def.h"

#ifdef OPTIONS_USE_NLANG

#undef _T
#define _T(x)		no_locale.Translate(x)
#define B_LANGUAGE_CHANGED	'_BLC'


#define	CMD_LANG_LOAD	'3NLD'

//#include "xml/xmlnode.h"

class NLang
{
public:
	NLang();
	virtual ~NLang();
	
	virtual void	Init(const char* path);
	
	bool 			LoadFile(const char* filename);
	bool 			LoadFileID(const char* langID);

	const char*		Translate(const char* str);

	void			BuildLangMenu(BMenu* menu, const char* current=NULL);
protected:
	XmlNode			list;
	
private:
	bool			load_lang_file(const char* name);
	
	BString			mPath;
};

extern NLang	no_locale;

#endif
#endif
