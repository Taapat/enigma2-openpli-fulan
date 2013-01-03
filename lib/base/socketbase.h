#ifndef _socketbase_h
#define _socketbase_h

class eSocketBase
{
public:
	static ssize_t singleRead(int fd, void *buf, size_t count);
	static ssize_t timedRead(int fd, void *buf, size_t count, int initialtimeout, int interbytetimeout);
	static ssize_t readLine(int fd, char** buffer, size_t* bufsize);
	static ssize_t writeAll(int fd, const void *buf, size_t count);
	static int select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
	static int connect(const char *hostname, int port, int timeoutsec);
};

#endif
