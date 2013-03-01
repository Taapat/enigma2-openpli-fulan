#ifndef _socketbase_h
#define _socketbase_h

class eSocketBase
{
public:
	static ssize_t singleRead(int fd, void *buf, size_t count);
	static ssize_t timedRead(int fd, void *buf, size_t count, const int timeoutms);
	static ssize_t timedRead(int fd, void *buf, size_t count, int* timeoutms);
	static ssize_t readLine(int fd, char** buffer, size_t* bufsize, const int timeoutms = 5000);
	static ssize_t readLine(int fd, char** buffer, size_t* bufsize, int* timeoutms);
	static ssize_t writeAll(int fd, const void *buf, size_t count);
	static int select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
	static int connect(const char *hostname, int port, int timeoutsec);
	static int finishConnect(int fd, int timeoutsec);
	static ssize_t openHTTPConnection(int fd, const std::string& getRequest, std::string& httpHdr);
};

#endif
