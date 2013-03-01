#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <vector>
#include <string>

#include "socketbase.h"

#include <lib/base/ebase.h>
#include <lib/base/eerror.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

int eSocketBase::select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
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
			eDebug("eSocketBase::select error (%m)");
			break;
		}

		break;
	}
	return retval;
}

ssize_t eSocketBase::singleRead(int fd, void *buf, size_t count)
{
	int retval;
	while (1)
	{
		retval = ::read(fd, buf, count);
		if (retval < 0)
		{
			if (errno == EINTR) continue;
			eDebug("eSocketBase::singleRead error (%m)");
		}
		return retval;
	}
}

ssize_t eSocketBase::timedRead(int fd, void *buf, size_t count, const int timeoutms)
{
	int timeout = timeoutms;
	return timedRead(fd, buf, count, &timeout);
}

ssize_t eSocketBase::timedRead(int fd, void *buf, size_t count, int* timeoutms)
{
	fd_set rset;
	struct timeval timeout;
	int result;
	size_t totalread = 0;
	
	timeout.tv_sec = *timeoutms/1000;
	timeout.tv_usec = (*timeoutms % 1000) * 1000;
	
	while (totalread < count)
	{
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		if ((result = select(fd + 1, &rset, NULL, NULL, &timeout)) < 0) return -1; /* error */
		if (result == 0) break;
		if ((result = singleRead(fd, ((char*)buf) + totalread, count - totalread)) <= 0)
		{
			return -1;
		}
		if (result == 0) break;
		totalread += result;
	}
	*timeoutms = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
	eDebug("timedRead %d", totalread);
	return totalread;
}

/*
ssize_t eSocketBase::readLine(int fd, char** buffer, size_t* bufsize)
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
		result = timedRead(fd, (*buffer) + i, 1, 3000);
		if (result <= 0 || (*buffer)[i] == '\n')
		{
			(*buffer)[i] = '\0';
			return result <= 0 ? -1 : i;
		}
		if ((*buffer)[i] != '\r') i++;
	}
	return -1;
} */

ssize_t eSocketBase::readLine(int fd, char** buffer, size_t* bufsize, const int timeoutms)
{
	int timeout = timeoutms;
	return readLine(fd, buffer, bufsize, &timeout);
}

ssize_t eSocketBase::readLine(int fd, char** buffer, size_t* bufsize, int* timeoutms)
{
	fd_set rset;
	struct timeval timeout;
	char *lbuf = *buffer;
	
	timeout.tv_sec = *timeoutms/1000;
	timeout.tv_usec = (*timeoutms % 1000) * 1000;

	size_t pos = 0;
	size_t read = 0;
	size_t bytesToFlush = 0;
	bool is_end = false;

	while (!is_end) {
		if (read >= *bufsize) {
			char *newbuf = (char*)realloc(lbuf, (*bufsize)+1024);
			if (newbuf == NULL)
				return -ENOMEM;
			*buffer = newbuf;
			lbuf = newbuf;
			*bufsize = (*bufsize) + 1024;
		}
		
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		const int rc = ::select(fd+1, &rset, NULL, NULL, &timeout);
		if (rc <= 0) {
			break;
		}
		int rcvd = ::recv(fd, lbuf+read, *bufsize-read, MSG_PEEK);
		read += rcvd;

		for(pos = read - rcvd; pos + 1 < read; pos++) {
			if (lbuf[pos] == '\r' && lbuf[pos+1] == '\n') {
				is_end = true;
				bytesToFlush = pos;
				while(lbuf[bytesToFlush] == '\n' || lbuf[bytesToFlush] == '\r') bytesToFlush++;
				break;
			}
		}
	}

	//flush the line
	eDebug("to flush %d", bytesToFlush);
	char tmpbuf[1024];
	while (bytesToFlush > 0) {
		int rc = MIN(bytesToFlush, sizeof(tmpbuf));
		rc = ::recv(fd, tmpbuf, rc, 0);
		if (rc > 0) bytesToFlush -= rc;
		else return -1;
	}
	
	lbuf[pos] = '\0';
	*timeoutms = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
	return pos;
}

ssize_t eSocketBase::openHTTPConnection(int fd, const std::string& getRequest, std::string& httpHdr)
{
	fd_set wset, rset;
	struct timeval timeout;

	size_t wlen = getRequest.length();
	const char* buf_end = getRequest.data() + wlen;

	while (wlen) {
		FD_ZERO(&wset);
		FD_SET(fd, &wset);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		const int rc = select(fd+1, NULL, &wset, NULL, &timeout);
		if (rc <= 0) {
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
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		const int rc = ::select(fd+1, &rset, NULL, NULL, &timeout);
		if (rc <= 0) {
			free(rbuff);
			return -1;
		}
	
		size_t hdroff=0;
		//read the response header
		int rcvd = ::recv(fd, rbuff, rbuff_size, MSG_PEEK);
		int hdr_end = 0;
		while(hdroff < rcvd && hdr_end < 2) {
			if (rbuff[hdroff] == '\n') hdr_end++;
			else if (rbuff[hdroff] != '\r') hdr_end = 0;
			hdroff++;
		}
		while(rbuff[hdroff] == '\n' || rbuff[hdroff] == '\r') hdroff++;

		httpHdr.append(rbuff, hdroff);
		if (hdroff <= rcvd) break;
	}

	//flush the header
	int bytesToFlush = httpHdr.length();
	while (bytesToFlush > 0) {
		int rc = MIN(bytesToFlush, rbuff_size);
		rc = ::recv(fd, rbuff, rc, 0);
		if (rc > 0) bytesToFlush -= rc;
		else{free(rbuff); return -1;}
	}

	free(rbuff);
	return 0;
}

int eSocketBase::connect(const char *hostname, int port, int timeoutsec)
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

int eSocketBase::finishConnect(int fd, int timeoutsec)
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

ssize_t eSocketBase::writeAll(int fd, const void *buf, size_t count)
{
	char *ptr_end = (char*)buf + count;
	size_t tosend = count;
	while (tosend > 0)
	{
		const int retval = ::write(fd, (ptr_end-tosend), tosend);

		if (retval <= 0)
		{
			if (retval == 0 && errno == EINTR) continue;
			eDebug("eSocketBase::writeAll error (%m)");
			return retval;
		}
		tosend -= retval;
	}
	return (count - tosend);
}
