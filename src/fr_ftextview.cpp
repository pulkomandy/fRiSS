#include "fr_ftextview.h"

#include <Cursor.h>

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

	SetViewColor(make_color(255,255,255));
	SetLowColor(make_color(255,255,255));
}


void
FTextView::FrameResized(float width, float height)
{
	SetTextRect(BRect(0,0,width, height));
	BTextView::FrameResized(width, height);
}

FTextView::tLink* 
FTextView::GetLinkAt(BPoint point)
{
	int32 offset = OffsetAt(point);
		
	tLink* aLink;
	for(int i = 0; aLink = (tLink*)links.ItemAt(i); i++)
	{
		if (offset >= aLink->linkoffset
				&& offset < aLink->linkoffset + aLink->linklen) {
			return aLink;
		}
	}
	return NULL;
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
		tLink* aLink = GetLinkAt(point);

		if (aLink)
			parent.OpenURL(aLink->target);
		else
			BTextView::MouseDown(point);
	}
}


void FTextView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	static BCursor* cursor = new BCursor(B_CURSOR_ID_FOLLOW_LINK);

	if(transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW)
	{
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
		return;
	}

	if(GetLinkAt(point)) {
		SetViewCursor(cursor);
	} else {
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
	}
}


void FTextView::Render(XmlNode* node, text_run_array& textStyle)
{
	switch(node->Type())
	{
		case XML_TYPE_NODE:
		{
			text_run_array oldStyle = textStyle;

			const char* name = node->Name();
			if(strcmp(name,"") == 0) {
				// Ok, this is our root node....
			} else if(strcmp(name,"li") == 0) {
				Insert("\n\xE2\x80\xA2", &textStyle);
			} else if(strcmp(name,"p") == 0) {
				Insert("\n");
			} else {
				printf("FIXME unhandled node %s\n", name);
			}

			// TODO node.mAttribute (list of XmlNodes)
			int32 count = node->Children();
			for(int32 i = 0; i < count; i++)
			{
				Render(node->ItemAt(i), textStyle);
			}

			textStyle = oldStyle;
			break;
		}

		case XML_TYPE_SINGLE:
		{
			int32 linkstart = -1;
			text_run_array oldStyle = textStyle;
			const char* name = node->Name();

			if(strcmp(name,"a") == 0) {
				linkstart = TextLength();
				textStyle.runs[0].color = make_color(0, 0, 255);
			} else if(strcmp(name,"b") == 0) {
				textStyle.runs[0].font = be_bold_font;

			} else if(strcmp(name,"br/") == 0 || strcmp(name, "br") == 0) {
				// FIXME the parser shouldn't feed us br/ as the node name...
				Insert("\n");
			} else if(strcmp(name,"h3") == 0) {
				textStyle.runs[0].font.SetSize(be_plain_font->Size() * 1.5f);
				Insert("\n");
			} else if(strcmp(name,"hr") == 0) {
				// FIXME do something nicer with these...
				Insert("\n----------------------------");
			} else if(strcmp(name,"p") == 0) {
				Insert("\n");
			} else if(strcmp(name,"strong") == 0) {
				textStyle.runs[0].font = be_bold_font;
			} else {
				printf("FIXME unhandled single node %s\n", name);
			}

			Insert(node->Value(), &textStyle);

			if(linkstart >= 0) {
				int32 linklen = strlen(node->Value());
				links.AddItem(new tLink(linkstart, linklen, node->Attribute("href")));
			}

			textStyle = oldStyle;

			Insert(" ");
			break;
		}

		case XML_TYPE_STRING:
		{
			Insert(node->Value(), &textStyle);
			break;
		}

		case XML_TYPE_COMMENT:
			// Comments are ignored
			break;

		default:
			puts("FIXME unknown node type !");
			break;
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
	Insert("\n");

	int32 linkoffset = TextLength();
	int32 linklen = link.Length();
	Insert(link.String(), &linkStyle);
	links.AddItem(new tLink(linkoffset, linklen, link));

	Insert("\n\n");

	// TODO consider using a BWebView when it becomes available

	// Parse the contents as XML to interpret html tags
	// TODO maybe we should get the XML node from the caller instead of a
	// flattened string to parse again ?
	XmlNode body(contents.String(), NULL);

	// Now browse the XML tree and add stuff to the view
	/*
	puts(contents.String());
	puts("--- XML CONTENTS ---");
	body.Display();
	puts("--- END XML CONTENTS ---");
	*/
	
	text_run_array textStyle;
	textStyle.count = 1;
	textStyle.runs[0].font = be_plain_font;
	textStyle.runs[0].offset = 0;
	textStyle.runs[0].color = make_color(0,0,0);

	// Handle text-only RSS feeds
	if(body.Children() == 0 || body.Children() == 1 && strcmp(body.ItemAt(0)->Name(), "") == 0) {
		Insert(contents, &textStyle);
	} else
		Render(&body, textStyle);
#if 0
	// For now just do some crappy search and replace to make the thing 
	// somewhat readable...

	typedef struct {
		char* from;
		char* to;
	} transform;

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
		{"<strong>", "</strong>", be_bold_font},
		{"<em>", "</em>", &emfont}
	};

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

#endif
}
