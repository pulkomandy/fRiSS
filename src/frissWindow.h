/* fRiSS - An RSS feed reader for Haiku
 * Copyright 2012, Adrien Destugues <pulkomandy@gmail.com>
 * Distributed under the terms of the MIT licence
 */

class FrissWindow: public BWindow
{
	public:
		FrissWindow(FrissConfig* config, XmlNode* theList, BRect frame,
			const char* Title);

		void MessageReceived(BMessage* message);
		void PopulateFeeds(XmlNode* theList);
		bool QuitRequested();
		void Save(FrissConfig* conf, XmlNode* root);

	private:
		FrissView	*myf;
		BListView* feedList;
		XmlNode* Xfeeds;
};
