// ======= OPTIONS ===========================================
// buffer size for IO
#define BUFSIZE	25000
#define URL_BUFSIZE	1000

// Debug build?
//#define N3S_DEBUG

// Enable to suppress charset conversion
// also remove libtextencoding.so
// reduces size by 8kb
//#define OPTIONS_NO_CHARSET_CONVERSION

// You can disable ICS if you like:
// reduces size by 8kb, for those purists *g*
//#define OPTIONS_NO_ICS

// You can disable ICS if you like:
// reduces size by 8kb, for those purists *g*
//#define OPTIONS_NO_ATOM

// Experimental transparency, disable for releases
//#define ALLOW_TRANSP	1

// Auto-enabled on Zeta-builds:
//#define __ZETA__ 1

// Use N3S custom stupid language support, or save 5kb
#define OPTIONS_USE_NLANG 1

// Use helper window instead of polling
//#define OPTIONS_USE_HELPERWINDOW 1

// RefreshRate (in minutes)
#define REFRESH_MIN_NORM	3
#define REFRESH_REC_NORM	60

#define REFRESH_MIN_ADVC	3
#define REFRESH_REC_ADVC	30

// Enable preview and full windows
#define OPTIONS_WINDOW_MODE 1


// OVERRIDES
#ifdef __ZETA__
	#undef OPTIONS_USE_NLANG
#else
	//#undef OPTIONS_WINDOW_MODE
#endif
