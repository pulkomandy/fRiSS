// BeAPI:
#include <net/Url.h>
#include <net/UrlSynchronousRequest.h>
#include <net/UrlResult.h>

#include <algorithm>

#include "fr_def.h"


/**
 * Load feed from url @feed, into buffer @buf of size @size.
 * Reurn size of data read, or -1 on error.
 */
char* LoadFeedNet(const char* feed, size_t& bufsize)
{
	BUrl url(feed);
	BUrlSynchronousRequest request(url);
	request.Perform();
	request.WaitUntilCompletion();
	const BUrlResult& result = request.Result();

	// FIXME the constness of the result prevents us from reading from it !
	const BMallocIO& io = result.RawData();
	bufsize = io.BufferLength();

	char* buf = (char*)malloc(bufsize);
	memcpy(buf, io.Buffer(), bufsize);
	return buf;
}
