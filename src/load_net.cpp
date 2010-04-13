// BeAPI:
#include <be/net/NetEndpoint.h>
#include <be/net/NetAddress.h>
#include <be/support/String.h>

// sonst:
// für printf
#include <stdio.h>

#include "fr_def.h"

#define ERROR_NET			-1
#define ERROR_NET_DNS		-10
#define ERROR_NET_TIMEOUT	-55

//#define TRACE_NET

#ifdef TRACE_NET
	#define FPRINT(x) printf x 
#else
	#define FPRINT(x)
#endif

int LoadFeedNet(const char* feed, char* buf, int bufsize)
{
	FPRINT(( "\n========LoadFeedNet=======\nLoading %s\n", feed ));
	
	// Hostname auflösen:
	BString st(feed);
	st.RemoveFirst("http://");
	st.Truncate( st.FindFirst('/') );
	
	BNetAddress *addr;
	try {
		addr = new BNetAddress(st.String(),80);
		if (addr->InitCheck() != B_OK) {
			puts("...");
			return ERROR_NET_DNS;
		}
	}
	catch(...) {
		FPRINT(("Exception?!?"));
		return ERROR_NET_DNS;
	}
	
	feed += 7 + st.Length();
	
	FPRINT(("Host:\t%s\nFile:\t%s\n", st.String(), feed ));
	
	// Socket anlegen:
	BNetEndpoint sock;
	if (sock.InitCheck() != B_OK)
		return ERROR_NET;

	// Verbinden:
	sock.SetTimeout( 2000000 );		
    if (sock.Connect(*addr) != B_OK) {
    	sock.Close();
    	return ERROR_NET;
    }
	FPRINT(("  Connect!\n" ));
	    
    char cmd[1000];

	//#define VER_HTTP "Wget/1.8"

	// workaround for bezip.de:
	if (st.ICompare("www.bezip.de")==0)
		sprintf(cmd, "GET %s\r\n\r\n",feed);
	else
		sprintf(
			cmd,
			"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nAccept: */*\r\nConnection: Close\r\n\r\n",
			feed,
			st.String(),
			VER_HTTP
			);

	FPRINT(( "HTTP-Request:\n------------\n%s\n------------\n", cmd ));

	sock.SetNonBlocking(true);
    if (sock.Send(cmd, sizeof(cmd), 0) == B_ERROR) {
    	puts(sock.ErrorStr());
    	sock.Close();
		return ERROR_NET;
    }
    
    FPRINT(( "  Send ok\n" ));
  
    if (!sock.IsDataPending(5000000)) {
    	FPRINT(( _T("Error: Server did not respond.") ));
    	return ERROR_NET_TIMEOUT;
    }
  
    BNetBuffer buffy(bufsize);
    FPRINT(( "  Buffer ready\n" ));
 
 	sock.SetNonBlocking(false);
 	sock.SetTimeout( 5000000 );
 	
    int read = 0, nr=0;
    while ((read = sock.Receive(buffy, bufsize-1)) > 0) {
    	++nr;
    	FPRINT(("Getting packet %i (read:%d)\n",nr,read));
    }

   	//printf("Reading: %s\n", sock.ErrorStr() );
	sock.Close();
	
	int size = buffy.BytesRemaining();
	if (size>bufsize)
		size = bufsize-1;
    buffy.RemoveData(buf,size);
    buf[size] = 0; // string sicher machen
    
    FPRINT(( "  Receive ok" ));
	return size;
}
