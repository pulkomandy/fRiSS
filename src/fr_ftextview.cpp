#include "fr_ftextview.h"

#include <assert.h>
#include <Cursor.h>

#include "fr_view.h"

bool FTextView::isInit = false;
text_run_array FTextView::linkStyle;
text_run_array FTextView::titleStyle;

FTextView::FTextView() :
	BTextView("Feed item text view", B_WILL_DRAW | B_FRAME_EVENTS)
{
}


FTextView::FTextView(BMessage* message)
	: BTextView(message)
{
}


FTextView::~FTextView()
{
	tLink* aLink;
	for(int i = 0; (aLink = (tLink*)links.ItemAt(i)); i++)
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
	for(int i = 0; (aLink = (tLink*)links.ItemAt(i)); i++)
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
	FrissView* parent = dynamic_cast<FrissView*>(Parent()->Parent());
	
	GetMouse(&cursor,&m_buttons);

	if (m_buttons & 0x2){
		// right mouse button is for popup only :-)
		BPoint m_point = point;
		ConvertToScreen(&m_point);
		
		parent->StartPopup( m_point );
	}
	else if (m_buttons & 0x4) {
		parent->LoadNext();
	}		
	else {
		// See if we hit one of the links
		tLink* aLink = GetLinkAt(point);

		if (aLink)
			parent->OpenURL(aLink->target);
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


void FTextView::RenderHeading(int level, text_run_array& textStyle)
{
	float multiplier = 1.8f;
	multiplier -= level / 10.0f;
	textStyle.runs[0].font.SetSize(be_plain_font->Size() * multiplier);
	Insert("\n");
}


void FTextView::RenderLi(text_run_array& textStyle)
{
	if (itemNumber == 0)
		Insert("\n\xE2\x80\xA2", &textStyle);
	else {
		BString txt("\n");
		txt << itemNumber;
		txt << ". ";
		Insert(txt, &textStyle);
		itemNumber++;
	}
}


void FTextView::Render(const XmlNode* const node, text_run_array& textStyle)
{
	switch(node->Type())
	{
		case XML_TYPE_NODE:
		{
			text_run_array oldStyle = textStyle;

			const char* name = node->Name();
			if(strcmp(name,"") == 0) {
				// Ok, this is our root node....
			} else if(strcmp(name,"br") == 0) {
				// TODO Which attributes or subelements could be useful ?
				Insert("\n");
			} else if(strcmp(name,"li") == 0) {
				RenderLi(textStyle);
			} else if(strcmp(name, "ol") == 0) {
				itemNumber = 1; // Start counting
				// TODO handle nested ol (need to save&restore itemNumber)
			} else if(strcmp(name,"p") == 0) {
				Insert("\n");
			} else if(strcmp(name, "ul") == 0) {
				itemNumber = 0; // li will use bullets
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
			} else if(strcmp(name,"em") == 0) {
				textStyle.runs[0].font.SetFace(B_ITALIC_FACE);
			} else if(strcmp(name,"h2") == 0) {
				RenderHeading(2, textStyle);
			} else if(strcmp(name,"h3") == 0) {
				RenderHeading(3, textStyle);
			} else if(strcmp(name,"h4") == 0) {
				RenderHeading(4, textStyle);
			} else if(strcmp(name,"hr") == 0) {
				// FIXME do something nicer with these...
				Insert("\n----------------------------");
			} else if(strcmp(name,"li") == 0) {
				RenderLi(textStyle);
			} else if(strcmp(name,"p") == 0) {
				Insert("\n");
			} else if(strcmp(name,"pre") == 0) {
				textStyle.runs[0].font = be_fixed_font;
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
			assert(false);
			break;
	}
}


void FTextView::SetContents(const BString& title, const XmlNode& body,
	const BString& link)
{
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

	// Now browse the XML tree and add stuff to the view
	text_run_array textStyle;
	textStyle.count = 1;
	textStyle.runs[0].font = be_plain_font;
	textStyle.runs[0].offset = 0;
	textStyle.runs[0].color = make_color(0,0,0);

	Render(&body, textStyle);
}


status_t
FTextView::Archive(BMessage* archive, bool deep) const
{
	status_t result = BTextView::Archive(archive, deep);
	archive->AddString("class", "FTextView");
	return result;
}


/*static*/ BArchivable*
FTextView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "FTextView"))
		return NULL;
	return new FTextView(archive);
}
