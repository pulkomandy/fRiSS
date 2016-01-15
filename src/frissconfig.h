#ifndef FRISSCONFIG_INCLUDED
#define FRISSCONFIG_INCLUDED

#include "fr_def.h"

enum ColBack_t
{
	ColBackCustom = 0,
	ColBackTransparent = 1,
	ColBackDesktop = 2,
	ColBackDefault = 3
};

enum ColFore_t
{
	ColForeCustom = 0,
	ColForeAdapt = 1
};

enum BrowserType_t
{
	BrowserDefault = 0,
	BrowserNetpositive = 1,
	BrowserFirefox = 2,
	BrowserCustom = 3
};

enum WindowMode_t
{
	WindowModeSimple = 0,
	WindowModePreview = 1,
	WindowModeFull = 2,


	WindowMode_NONE = 999
};

class FrissConfig : public BArchivable
{
public:
	// Constructor sets all members to default
	FrissConfig();
	FrissConfig(BMessage* archive);
	status_t Archive(BMessage *msg, bool deep = true) const;
	static BArchivable* Instantiate(BMessage* msg);

	//
	~FrissConfig();

	// Load and Sa
	bool Load( const char* path );
	bool Save( const char* path );

	void Defaults();

	inline void		SetWindowRect(BRect frame) { m_rWindow = frame; }
	inline BRect 	GetWindowRect() { return m_rWindow; }

	inline uint32	Index() { return m_iIndex; }
	inline void		SetIndex(uint32 newindex) { if (newindex<m_iAnz) m_iIndex = newindex; }

public:
	// generic/misc =============================================================
	uint32 		m_iVersion;
	uint32		RefreshRate;		// minutes
	bool 		RefreshAdvances; 	// 0=reloads same page, 1=index++
	BString 	Lang;					// char Lang[5];

	// feedlist =================================================================
	uint32		m_iIndex;			// index in Feed list
	uint32		m_iAnz;				// number of feeds
	BString		Feedlist;			//char Feedlist[1024];

	// Appearance ===============================================================

	BRect		m_rWindow;

	ColBack_t ColBackMode;

	ColFore_t ColForeMode;

	rgb_color		col;	// background color
	rgb_color		high;	// text color

	WindowMode_t	WindowMode;


	// Browser ==================================================================

	BrowserType_t BrowserType;

	BString BrowserMime;			// char BrowserMime[1024];
};

#endif
