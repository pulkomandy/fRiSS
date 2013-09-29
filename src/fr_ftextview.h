#ifndef FR_FTEXTVIEW
#define FR_FTEXTVIEW

#include "fr_view.h"
#include "fr_def.h"
#include "fr_fstringitem.h"

class FTextView : public BTextView
{
public:
	FTextView(FrissView& parentView, BRect br);
	~FTextView();
	
	void AttachedToWindow();
	void FrameResized(float width, float height);
	void MouseDown(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	void SetContents(const BString& title, const XmlNode& contents,
		const BString& link);
	
private:
	struct tLink {
		const int32 linkoffset;
		const int32 linklen;
		const BString target;

		tLink(int32 off, int32 len, BString trg)
			: linkoffset(off)
			, linklen(len)
			, target(trg)
		{
		}
	};

	void Render(const XmlNode* const node, text_run_array& textStyle);

	void RenderHeading(int level, text_run_array& textStyle);
	void RenderLi(text_run_array& textStyle);

	tLink* GetLinkAt(BPoint point);

	FrissView& parent;


	BList links;
	int itemNumber; // for <ol> enumerations

	static bool isInit;
	static text_run_array linkStyle;
	static text_run_array titleStyle;
};


#endif
