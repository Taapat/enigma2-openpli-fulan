#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <vector>
#include <string>

#include "wrappers.h"

#include <lib/base/ebase.h>
#include <lib/base/eerror.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

int Select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int retval;
	fd_set rset, wset, xset;
	timeval interval;

	/* make a backup of all fd_set's and timeval struct */
	if (readfds) rset = *readfds;
	if (writefds) wset = *writefds;
	if (exceptfds) xset = *exceptfds;
	if (timeout)
	{
		interval = *timeout;
	}
	else
	{
		/* make gcc happy... */
		timerclear(&interval);
	}

	while (1)
	{
		retval = ::select(maxfd, readfds, writefds, exceptfds, timeout);

		if (retval < 0)
		{
			/* restore the backup before we continue */
			if (readfds) *readfds = rset;
			if (writefds) *writefds = wset;
			if (exceptfds) *exceptfds = xset;
			if (timeout) *timeout = interval;
			if (errno == EINTR) continue;
			eDebug("select error (%m)");
			break;
		}

		break;
	}
	return retval;
}

ssize_t singleRead(int fd, void *buf, size_t count)
{
	while (1)
	{
		int retval = ::read(fd, buf, count);
		if (retval < 0)
		{
			if (errno == EINTR) continue;
			eDebug("singleRead error (%m)");
		}
		return retval;
	}
}

ssize_t timedRead(int fd, void *buf, size_t count, int initialtimeout, int interbytetimeout)
{
	size_t totalread = 0;

	while (totalread < count)
	{
                struct pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLIN|POLLPRI;		
                int result = poll(&pfd, 1, ((totalread==0)?initialtimeout:interbytetimeout));
		if (result == 1 && (result = singleRead(fd, ((char*)buf) + totalread, count - totalread)) <= 0)
		{
			return -1;
		}

		if (result < 0)
		{
			if (totalread > 0) return totalread;
			else return result;
		}
		if (result == 0) break;
		totalread += result;
	}
	return totalread;
}

ssize_t readLine(int fd, char** buffer, size_t* bufsize)
{
	size_t i = 0;
	int result;
	while (1) 
	{
		if (i >= *bufsize) 
		{
			char *newbuf = (char*)realloc(*buffer, (*bufsize)+1024);
			if (newbuf == NULL)
				return -ENOMEM;
			*buffer = newbuf;
			*bufsize = (*bufsize) + 1024;
		}
		result = timedRead(fd, (*buffer) + i, 1, 3000, 100);
		if (result <= 0 || (*buffer)[i] == '\n')
		{
			if (i > 0 && (*buffer)[i-1] == '\r') i--;
			(*buffer)[i] = '\0';
			return (result <= 0)? result : i;
		}
		if ((*buffer)[i] != '\r') i++;
	}
	return -1;
}

ssize_t openHTTPConnection(int fd, const std::string& getRequest, std::string& httpHdr)
{
	fd_set wset, rset;
	struct timeval timeout;

	size_t wlen = getRequest.length();
	const char* buf_end = getRequest.data() + wlen;

	while (wlen) {
		struct pollfd pfd;
		pfd.fd = fd;
                pfd.events = POLLOUT;
                const int rc = poll(&pfd, 1, 5000);
		if (rc <= 0) {
			eDebug("%s: waiting to write timedout : %s", __FUNCTION__, strerror(errno));
			return -1;
		}

		const int sent = send(fd, buf_end-wlen, wlen, 0);
		if (sent > 0) wlen -= sent;
		else return -1;
	}

	const ssize_t rbuff_size = (1024*4);
	char* rbuff = (char*)malloc(rbuff_size);
	if (rbuff == NULL) return -1;

	while(true) {
                struct pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLIN;
                const int rc = poll(&pfd, 1, 5000);
		if (rc <= 0) {
			eDebug("%s: waiting to read timedout : %s", __FUNCTION__, strerror(errno));
			free(rbuff);
			return -1;
		}
	
		ssize_t hdroff=0;
		//read the response header
		int rcvd = ::recv(fd, rbuff, rbuff_size, MSG_PEEK);
		
		if (rcvd <= 0) {
			eDebug("%s: did not receive http header : %s", __FUNCTION__, strerror(errno));
			return -1;
		}
		char* hdrend=NULL;
		if ((hdrend=strstr(rbuff, "\r\n\r\n")) || (hdrend=strstr(rbuff, "\n\n"))) {
			while((hdrend-rbuff) < rcvd && (*hdrend=='\n' || *hdrend=='\r')) hdrend++;  
			httpHdr.append(rbuff, hdrend-rbuff);
			::recv(fd, rbuff, hdrend-rbuff, 0);
			break;
		} else {
			httpHdr.append(rbuff, rcvd);
			::recv(fd, rbuff, rcvd, 0);
		} 
	}

	free(rbuff);
	return 0;
}

int Connect(const char *hostname, int port, int timeoutsec)
{
	struct hostent *server=gethostbyname(hostname);
	if (server == NULL) {
		eDebug("can't resolve %s", hostname);
		return -1;
	}
	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) return sd;

	sockaddr_in  addr_in;
	bzero(&addr_in, sizeof(addr_in));
	addr_in.sin_family=AF_INET;
	bcopy(server->h_addr, &addr_in.sin_addr.s_addr, server->h_length);
	addr_in.sin_port=htons(port);

	int flags = fcntl(sd, F_GETFL, 0);
	bool setblocking = false;
	if (!(flags & O_NONBLOCK)) {
		/* set socket nonblocking, to allow for our own timeout on a nonblocking connect */
		flags |= O_NONBLOCK;
		if (fcntl(sd, F_SETFL, flags) == 0) setblocking = true;
	}

	int connectresult = ::connect(sd, (const sockaddr*)&addr_in, sizeof(addr_in)); 
	if (connectresult < 0 && (errno == EINTR || errno == EINPROGRESS)) {
		if (!timeoutsec) return sd; // do not wait connect to finish, the caller should take care about this
		timeval timeout = {timeoutsec, 0};
		fd_set wset;
		FD_ZERO(&wset);
		FD_SET(sd, &wset);

		if (select(sd + 1, NULL, &wset, NULL, &timeout) > 0) {
			int error = 0;
			socklen_t len = sizeof(error);
			if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && !error) connectresult = 0; /* we are connected */
		}
	}
	if (connectresult < 0) {
		::close(sd);
		return -1;
	}
	if (setblocking) {
		/* set socket blocking again */
		flags &= ~O_NONBLOCK;
		if (fcntl(sd, F_SETFL, flags) < 0) {
			::close(sd);
			return -1;
		}
	}
#ifdef SO_NOSIGPIPE
	int val = 1;
	setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&val, sizeof(val));
#endif
	return sd;
}

int finishConnect(int fd, int timeoutsec)
{
	timeval timeout = {timeoutsec, 0};
	fd_set wset;
	FD_ZERO(&wset);
	FD_SET(fd, &wset);

        int connectresult = -1;
	if (select(fd + 1, NULL, &wset, NULL, &timeout) > 0) {
		int error = 0;
		socklen_t len = sizeof(error);
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && !error) connectresult = 0; /* we are connected */
	}
        if (connectresult < 0) {
                ::close(fd);
                return -1;
        }

	int flags = fcntl(fd, F_GETFL, 0);
	if(flags < 0) {
		::close(fd);
		return -1;
	} 
	/* set socket blocking again */
	flags &= ~O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) < 0) {
		::close(fd);
		return -1;
	}
#ifdef SO_NOSIGPIPE
        int val = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&val, sizeof(val));
#endif
        return fd;
}

ssize_t writeAll(int fd, const void *buf, size_t count)
{
	char *ptr_end = (char*)buf + count;
	size_t tosend = count;
	while (tosend > 0)
	{
		const int retval = ::write(fd, (ptr_end-tosend), tosend);

		if (retval <= 0)
		{
			if (retval == 0 && errno == EINTR) continue;
			eDebug("writeAll error (%m)");
			return retval;
		}
		tosend -= retval;
	}
	return (count - tosend);
}
