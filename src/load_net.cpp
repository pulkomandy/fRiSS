// BeAPI:
#include <Url.h>
#include <UrlProtocolListener.h>
#include <UrlProtocolRoster.h>
#include <UrlResult.h>
#include <UrlSynchronousRequest.h>

#include <algorithm>

#include "fr_def.h"


/**
 * Load feed from url @feed, into buffer @buf of size @size.
 * Reurn size of data read, or -1 on error.
 */
void LoadFeedNet(const char* feed, BUrlProtocolListener* listener)
{
	BUrl url(feed);
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, listener);
	request->Run();
}
