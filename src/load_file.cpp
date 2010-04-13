#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>


int LoadFeedFile(const char* feed, char* buf, int bufsize)
{
	if (strncmp(feed,"file://",7)==0)
		feed += 7;	
	
	int fd = open(feed, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return 0;
	}
	
	int r = read(fd, buf, bufsize);
	if (r == -1) {
		perror("read");
		close(fd);
		return 0;
	}
	buf[r] = 0;
	
	close(fd);
	return r;
}
