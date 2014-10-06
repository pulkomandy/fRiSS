// BeAPI:
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlResult.h>
#include <UrlSynchronousRequest.h>

#include <algorithm>

#include "fr_def.h"


class SynchronousListener: public BUrlProtocolListener
{
	public:
		virtual	~SynchronousListener() {};
			void	DataReceived(BUrlRequest*, const char* data, off_t position,
					ssize_t size) {
			result.WriteAt(position, data, size);
		}
		BMallocIO result;
};


/**
 * Load feed from url @feed, into buffer @buf of size @size.
 * Reurn size of data read, or -1 on error.
 */
char* LoadFeedNet(const char* feed, size_t& bufsize)
{
	BUrl url(feed);
	SynchronousListener listener;
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, &listener);
	request->Run();
	while(request->IsRunning()) snooze(1000);

	// FIXME the constness of the result prevents us from reading from it !
	bufsize = listener.result.BufferLength();

	char* buf = (char*)malloc(bufsize);
	memcpy(buf, listener.result.Buffer(), bufsize);

	delete request;

	return buf;
}
