#include "fr_preflistview.h"

PrefListView::PrefListView(XmlNode* listroot, BRect f, const char *name,
	list_view_type type,
	uint32 resizingMode,
	uint32 flags) :
	BOutlineListView(f, name, type, resizingMode, flags)
{
	root = listroot;
}

bool
PrefListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	BOutlineListView::InitiateDrag(point,index,wasSelected);
	if (wasSelected) {
		int idx = IndexOf(point);
		if (idx == B_ERROR)
			return false;
		BStringItem* item = (BStringItem*)ItemAt(idx);
		BRect r = ItemFrame(idx);
		
		BMessage msg('ITEM');
		msg.AddInt32("Item", (int)item);
		msg.AddInt32("Item_nr", (int)index);
		msg.AddPoint("click_location", point);
		
		DragMessage( &msg, r, NULL );
		return true;
	}
	return false;
}


void
PrefListView::SelectionChanged()
{
	BOutlineListView::SelectionChanged();
	Window()->PostMessage( MSG_LIST_CHANGED );
}

void
PrefListView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'ITEM':
		{
			puts("We have a winner, supposably ;-)");
			BPoint dropzone;
			msg->FindPoint("_drop_point_", &dropzone);
			dropzone = ConvertFromScreen(dropzone);

			int32 i, nr;
			msg->FindInt32("Item", &i);
			msg->FindInt32("Item_nr", &nr);

			XmlNode* Item = (XmlNode*)i;
			XmlNode* toItem = dynamic_cast<XmlNode*>(ItemAt(IndexOf(dropzone)));

			// Only proceed if valid
			if (toItem && Item && toItem!=Item) {
				XmlNode* parent = Item->Parent();

				uint32 index = parent->IndexOf(Item);
				uint32 toIndex = toItem->Parent()->IndexOf(toItem);
				if (index<toIndex)
					toIndex++;

				parent->DetachChild(index);

				if (toItem->Attribute(OPML_URL)!=NULL) {
					toItem->Parent()->AddChild(Item, toIndex);
				}
				else {
					toItem->AddChild(Item,0);
				}

				MakeEmpty();
				BuildView(root);

				Invalidate();
			}
		}
		break;

		default:
			BOutlineListView::MessageReceived(msg);
	}
}



	void
PrefListView::BuildView(XmlNode *node, int level, BListItem* parent)
{
	if (!node) {
		puts("Rebuild mit NULL?!");
		return;
	}

	int anz = node->Children();

	//printf("BuildView: %s %d einträge\n", node->Attribute("text"), anz);

	if (anz>0) {
		if (level==0) {
			for (int i=0;i<anz;i++) {
				XmlNode* c = (XmlNode*)node->ItemAt(i);
				const char* t = c->Attribute("text");

				BStringItem* item = new BStringItem(t);
				item->SetText( t );

				if (level>0)
					AddUnder(item, parent);
				else
					AddItem(item);

				//printf("Hinzufügen war %d\n", b);

				if (c->Children()>0) {
					BuildView(c, level+1, item);
				}
			}
			DeselectAll();
		}
		else {
			for (int i=anz-1;i>=0;i--) {
				XmlNode* c = (XmlNode*)node->ItemAt(i);
				const char* t = c->Attribute("text");

				BStringItem* item = new BStringItem(t);
				item->SetText( t );

				if (level>0)
					AddUnder(item, parent);
				else
					AddItem(item);

				//printf("Hinzufügen war %d\n", b);

				if (c->Children()>0) {
					BuildView(c, level+1, item);
				}
			}
		}
	}
	else {
		//puts("Tja...");
	}	
}

void
PrefListView::MouseDown(BPoint point)
{
	BPoint cursor;
	ulong m_buttons;
	
	GetMouse(&cursor,&m_buttons);
	
	if (!(m_buttons & 0x2))
		BOutlineListView::MouseDown(point);
	else {
		// right mouse button is for popup only :-)		
		BMessage msg(MSG_LIST_POPUP);
		msg.AddPoint("point", point);
		
		int32 index = IndexOf(point);
		msg.AddInt32("index", index);
		
		if (index>0) {
			DeselectAll();
			Select(index);
			InvalidateItem(index);
		}

		Window()->PostMessage( &msg );
	}
}

int sortX(const BListItem* itemA, const BListItem* itemB)
{
	XmlNode *a = (XmlNode*)itemA;
	XmlNode *b = (XmlNode*)itemB;
	
	BString sa( a->Attribute(OPML_TITLE) );
	return sa.Compare( b->Attribute(OPML_TITLE) );
}

void
PrefListView::Sort(BListItem* node)
{
	SortItemsUnder(node, false, &sortX);
}

