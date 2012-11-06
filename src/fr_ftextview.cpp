#include "fr_ftextview.h"
#include "fr_view.h"

bool FTextView::isInit = false;
text_run_array FTextView::linkStyle;
text_run_array FTextView::titleStyle;

FTextView::FTextView(FrissView& parentView, BRect br) :
	BTextView(br, "stview", BRect(0, 0, br.Width() - B_V_SCROLL_BAR_WIDTH - 10,
		br.Height() - 1), B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
	, parent(parentView)
{
}


FTextView::~FTextView()
{
	tLink* aLink;
	for(int i = 0; aLink = (tLink*)links.ItemAt(i); i++)
		delete aLink;
}


void FTextView::AttachedToWindow()
{
	if(!isInit)
	{
		isInit = true;

		BFont linkfont(be_plain_font);
		linkfont.SetFace(B_UNDERSCORE_FACE);

		linkStyle.count = 1;
		linkStyle.runs[0].offset = 0;
		linkStyle.runs[0].font = linkfont;
		linkStyle.runs[0].color = make_color(0, 0, 255);

		titleStyle.count = 1;
		titleStyle.runs[0].font = be_bold_font;
	}
}


void
FTextView::FrameResized(float width, float height)
{
	SetTextRect(BRect(0,0,width, height));
	BTextView::FrameResized(width, height);
}


void
FTextView::MouseDown(BPoint point)
{
	BPoint	cursor;
	ulong	m_buttons;
	
	GetMouse(&cursor,&m_buttons);

	if (m_buttons & 0x2){
		// right mouse button is for popup only :-)
		BPoint m_point = point;
		ConvertToScreen(&m_point);
		
		parent.StartPopup( m_point );
	}
	else if (m_buttons & 0x4) {
		parent.LoadNext();
	}		
	else {
		// See if we hit one of the links
		int32 offset = OffsetAt(point);
		
		tLink* aLink;
		for(int i = 0; aLink = (tLink*)links.ItemAt(i); i++)
		{
			if (offset >= aLink->linkoffset &&
					offset < aLink->linkoffset + aLink->linklen) {
				parent.OpenURL(aLink->target);
			} else
				BTextView::MouseDown(point);
		}
	}
}


void FTextView::SetContents(const BString& title, const BString& contents,
	const BString& link)
{
	int current_offset = 0;

	// First of all, clear the view
	SetText(NULL);
	SetStylable(true);

	// Insert the title
	Insert(title.String(), &titleStyle);
	Insert("\n\n");

	// TODO consider using a BWebView when it becomes available
	// For now just do some crappy search and replace to make the thing 
	// somewhat readable...
	text_run_array textStyle;
	textStyle.count = 1;
	textStyle.runs[0].font = be_plain_font;
	textStyle.runs[0].offset = 0;
	textStyle.runs[0].color = make_color(0,0,0);

	typedef struct {
		char* from;
		char* to;
	} transform;

	static const transform replacement[] = {
		{"<p>", ""}, {"</p>", "\n\n"}, {"<br>", "\n"}, {"<br />", "\n"},
		{"<ul>", "\n"}, {"<li>", "\xe2\x80\xa2"}, {"</li>", "\n"},
		{"</ul>", "\n"}, {"<hr />", "\n-------------------\n"},
		{"&amp;", "&"}, {"&quot;", "\""}, {"&eacute;", "é"}
	};

	BString contentsParsed = contents;
	for(int i = sizeof(replacement) / sizeof(transform); --i >= 0;)
		contentsParsed.ReplaceAll(replacement[i].from, replacement[i].to);

	int nextOffset = -1;
	while((nextOffset = contentsParsed.FindFirst("&#", nextOffset + 1)) != B_ERROR)
	{
		char dest[5];
		char source[8];
		strncpy(source, contentsParsed.String() + nextOffset, 7);
		uint32_t codepoint = strtol(contentsParsed.String() + nextOffset + 2,
			NULL, 10);

		if(codepoint < 0x80)
		{
			dest[0] = codepoint;
			dest[1] = 0;
		} else if(codepoint < 0x800) {
			dest[0] = codepoint >> 6  & 0x1F | 0xC0;
			dest[1] = codepoint & 0x3F | 0x80;
			dest[2] = 0;
		} else if(codepoint < 0x010000) {
			dest[0] = codepoint >> 12  & 0xF | 0xE0;
			dest[1] = codepoint >> 6  & 0x3F | 0x80;
			dest[2] = codepoint & 0x3F | 0x80;
			dest[3] = 0;
		} else {
			dest[0] = codepoint >> 18  & 0x7 | 0xF0;
			dest[1] = codepoint >> 12  & 0x3F | 0x80;
			dest[2] = codepoint >> 6  & 0x3F | 0x80;
			dest[3] = codepoint & 0x3F | 0x80;
			dest[4] = 0;
		}

		contentsParsed.ReplaceAll(source, dest);

	}

	// Now attempt to do some formatting !
	typedef struct {
		int start;
		int end;
		const BFont* font;
		rgb_color color;
	} Range;

	BList list;

	typedef struct {
		char* from;
		char* to;
		const BFont* style;
	} tag;

	BFont emfont(be_plain_font);
	emfont.SetFace(B_ITALIC_FACE);

	static const tag bold[] = {
		{"<b>", "</b>", be_bold_font},
		{"<strong>", "</strong>", be_bold_font},
		{"<em>", "</em>", &emfont}
	};

	for(int i = sizeof(bold) / sizeof(tag); --i >= 0;) {
		nextOffset == -1;
		while((nextOffset = contentsParsed.FindFirst(bold[i].from,
			nextOffset + 1)) != B_ERROR)
		{
			Range* r = new Range();
			r->start = nextOffset;
			contentsParsed.RemoveFirst(bold[i].from);
			r->end = contentsParsed.FindFirst(bold[i].to, nextOffset + 1);
			contentsParsed.RemoveFirst(bold[i].to);

			r->font = bold[i].style;

			list.AddItem(r);
		}
	}

	int32 linkoffset = TextLength();

	// Parse links
	nextOffset == -1;
	while((nextOffset = contentsParsed.FindFirst("<a href=\"",
		nextOffset + 1)) != B_ERROR)
	{
		Range* r = new Range();
		r->start = nextOffset;

		// The string now has the url at offset, followed by ">, then the link 
		// text.
		r->end = contentsParsed.FindFirst("\"", nextOffset + 9);
		
		// Now we need to remove everything between start and end+2 to leave
		// only the link text.
		BString url;
		contentsParsed.CopyInto(url, r->start + 9, r->end - (r->start + 9));
		printf("URL > %s\n",url.String());

		r->end = contentsParsed.FindFirst(">", nextOffset + 1);
		contentsParsed.Remove(r->start, r->end + 1 - r->start);

		r->end = contentsParsed.FindFirst("</a>", nextOffset + 1);
		contentsParsed.RemoveFirst("</a>");

		r->font = &linkStyle.runs[0].font;
		r->color = make_color(0, 0, 255);

		links.AddItem(new tLink(r->start + linkoffset, r->end - r->start, url));
		list.AddItem(r);
	}

	// Insert the parsed contents and set the style on it
	Insert(contentsParsed.String(), &textStyle);
	Insert("\n\n");

	Range* r;
	while((r = (Range*)list.FirstItem()))
	{
		SetFontAndColor(r->start + linkoffset, r->end + linkoffset,
			r->font, B_FONT_ALL, &r->color);
		list.RemoveItem(r);
		delete r;
	}

	linkoffset = TextLength();

	int32 linklen = link.Length();
	Insert(link.String(), &linkStyle);

	links.AddItem(new tLink(linkoffset, linklen, link));
}
