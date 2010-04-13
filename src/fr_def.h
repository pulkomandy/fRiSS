#ifndef FR_DEF
#define FR_DEF

#ifdef __ZETA__
typedef unsigned int uint;
typedef unsigned long ulong;
#endif

#include <stdio.h>
#include <be/AppKit.h>
#include <be/InterfaceKit.h>
//#include <be/support/String.h>
#include <be/SupportKit.h>

#include "fr_options.h"
#include "xmlnode.h"



#define APP_SIGNATURE	"application/N3S.fRiSS"

// ======= VERSION ===========================================
//#define VER_ACTUAL	"0.5"
#define VER_ACTUAL	"0.5 pre7"

#define VER_ADD		"\tthis version for testing purposes only\n"

// predefine strings for output
#ifndef	__ZETA__
	#define VERSION_	"fRiSS Ver."VER_ACTUAL
#else
	#define VERSION_	"fRiSS/Zeta Ver."VER_ACTUAL
#endif
#define VER_HTTP	"friss/"VER_ACTUAL" (BeOS)"
// #define STR_ABOUT	 moved to FrissView::AboutRequested()


// ======= MESSAGES ========================================
// Commands for popup
#define CmdNext 'next'
#define CmdPrev 'prev'
#define CmdRefr 'refr'
#define CmdOpts 'opts'
#define CmdQuit 'quit'
#define CmdLoad 'load'

#define MSG_PREF_DONE	'PREF'
#define MSG_LOAD_DONE	'FLOD'
#define MSG_LOAD_FAIL	'FLFL'
#define MSG_COL_CHANGED	'CHCL'
#define MSG_SB_CHANGED	'CHSB'
#define MSG_WORKSPACE	'WSCH'


// Preferences:
#define MSG_LIST_CHANGED	'Lchg'
#define MSG_LIST_POPUP		'Lpop'

#define MSG_P_ITEM_UPDATED	'PIUP'


// ======= CONSTANTS =======================================
#define BrowserDefaultStr		"0"
#define BrowserNetpositiveStr	"1"
#define BrowserFirefoxStr		"2"
#define BrowserCustomStr		"3"


// ======= CONSTANTS - OPML ================================

#define OPML_TITLE				"text"
#define OPML_URL				"xmlURL"
#define OPML_ADD_DESCRIPTION	"x-addDesc"
#define OPML_OVERRIDE_BROWSER	"x-overrideBrowser"
#define OPML_TTL				"x-ttl"


#define BROWSER_MIME_NETPOSITIVE		"application/x-vnd.Be-NPOS"
#define BROWSER_MIME_FIREFOX			"application/x-vnd.Firefox"
#define BROWSER_MIME_CUSTOM_DEFAULT		"application/x-vnd.Mozilla"


// ======= STUFF ===========================================
extern const char *app_signature;

// ======= MAKROS ============================================
#ifdef __ZETA__
	#include <locale/Locale.h>
#else
	#ifdef OPTIONS_USE_NLANG
		#include "nlang.h"
	#endif
	
	#ifndef _T
		#define _T(x)			x
	#endif	
	
	#ifndef B_LANGUAGE_CHANGED
		#define B_LANGUAGE_CHANGED	'_BLC'
	#endif
#endif


#define FrSetCol3(COL, R,G,B)		COL.red=R;COL.green=G;COL.blue=B;COL.alpha=0
#define FrSetCol4(COL, R,G,B,A)		COL.red=R;COL.green=G;COL.blue=B;COL.alpha=A

#ifndef SAFE_DELETE
	#define SAFE_DELETE(pThing)		{if (pThing) {delete pThing; pThing = NULL; }}
#endif

// ===========================================================
#endif
