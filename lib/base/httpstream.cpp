#include <cstdio>
#include <openssl/evp.h>

#include <lib/base/httpstream.h>
#include <lib/base/eerror.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CHUNKED_BUFF_SIZE (1024*1024)

DEFINE_REF(eHttpStream);

eHttpStream::eHttpStream() : 
	m_streamSocket(-1)
	,m_rbuffer(1024*1024)
	,m_tryToReconnect(false)
	,m_chunkedTransfer(false)
	,m_chunkedBuffer(NULL)
{
}

eHttpStream::~eHttpStream()
{
	if (m_chunkedBuffer) free(m_chunkedBuffer);
	close();
}

int eHttpStream::open(const std::string& url)
{
	eDebug("eHttpStream::open()");
	std::string currenturl(url), newurl;

	for (unsigned int i = 0; i < 3; i++)
	{
		if (openUrl(currenturl, newurl) < 0)
		{
			/* connection failed */
			close();
			eDebug("%s failed", __FUNCTION__);
			break;
		}
		if (newurl.empty())
		{
			/* we have a valid stream connection */
			eDebug("eHttpStream::open() - started streaming...");
			m_url = currenturl;
			return 0;
		}
		/* switch to new url */
		close();
		currenturl.swap(newurl);
		newurl.clear();
	}
	eDebug("eHttpStream::open() - too many redirects...");
	/* too many redirect / playlist levels (we accept one redirect + one playlist) */
	return -1;
}

int eHttpStream::openUrl(const std::string &url, std::string &newurl)
{
	int port;
	std::string hostname;
	std::string uri = url;
	std::string request;
	int result;
	char proto[100];
	int statuscode = 0;
	char statusmsg[100];

	close();

	int pathindex = uri.find("/", 7);
	if (pathindex > 0)
	{
		hostname = uri.substr(7, pathindex - 7);
		uri = uri.substr(pathindex, uri.length() - pathindex);
	}
	else
	{
		hostname = uri.substr(7, uri.length() - 7);
		uri = "";
	}
	int authenticationindex = hostname.find("@");
	if (authenticationindex > 0)
	{
		BIO *mbio, *b64bio, *bio;
		char *p = (char*)NULL;
		m_authorizationData = hostname.substr(0, authenticationindex);
		hostname = hostname.substr(authenticationindex + 1);
		mbio = BIO_new(BIO_s_mem());
		b64bio = BIO_new(BIO_f_base64());
		bio = BIO_push(b64bio, mbio);
		BIO_write(bio, m_authorizationData.data(), m_authorizationData.length());
		BIO_flush(bio);
		int length = BIO_ctrl(mbio, BIO_CTRL_INFO, 0, (char*)&p);
		m_authorizationData = "";
		if (p && length > 0)
		{
			/* base64 output contains a linefeed, which we ignore */
			m_authorizationData.append(p, length - 1);
		}
		BIO_free_all(bio);
	}
	int customportindex = hostname.find(":");
	if (customportindex > 0)
	{
		port = atoi(hostname.substr(customportindex + 1, hostname.length() - customportindex - 1).c_str());
		hostname = hostname.substr(0, customportindex);
	}
	else if (customportindex == 0)
	{
		port = atoi(hostname.substr(1, hostname.length() - 1).c_str());
		hostname = "localhost";
	}
	else
	{
		port = 80;
	}
	m_streamSocket = eSocketBase::connect(hostname.c_str(), port, 10);
	if (m_streamSocket < 0) return -1;

	request = "GET ";
	request.append(uri).append(" HTTP/1.1\r\n");
	request.append("Host: ").append(hostname).append("\r\n");
	if (m_authorizationData.length())
	{
		request.append("Authorization: Basic ").append(m_authorizationData).append("\r\n");
	}
	request.append("Accept: */*\r\n");
	request.append("Range: bytes=0-\r\n");
	request.append("Connection: close\r\n");
	request.append("\r\n");

	std::string hdr;
	result = eSocketBase::openHTTPConnection(m_streamSocket, request, hdr);
	if (result < 0) return -1;

	size_t pos = hdr.find("\n");
	const std::string& httpStatus = (pos == std::string::npos)? hdr: hdr.substr(0, pos);

	result = sscanf(httpStatus.c_str(), "%99s %3d %99s", proto, &statuscode, statusmsg);
	if (result != 3 || (statuscode != 200 && statuscode != 302))
	{
		eDebug("%s: wrong http response code: %d", __FUNCTION__, statuscode);
		return -1;
	}

	hdr = hdr.substr(pos+1);

	if (statuscode == 302) {
		pos = hdr.find("Location: ");
		if (pos != std::string::npos) {
			newurl = hdr.substr(pos+strlen("Location: "));
			pos = newurl.find("\n");
			if (pos != std::string::npos) newurl = newurl.substr(0, pos);
			eDebug("%s: redirecting to: %s", __FUNCTION__, newurl.c_str());
			return 0;
		}
	}

	m_chunkedTransfer = (hdr.find("Transfer-Encoding: chunked") != std::string::npos);

	pos = hdr.find("Content-Type:");
	if (pos != std::string::npos) {
		hdr = hdr.substr(pos);
		pos = hdr.find("\n");
		const std::string& contentStr = (pos == std::string::npos)? hdr: hdr.substr(0, pos); 
		/* assume we'll get a playlist, some text file containing a stream url */
		const bool playlist = (contentStr.find("application/text") != std::string::npos
				|| contentStr.find("audio/x-mpegurl")  != std::string::npos
				|| contentStr.find("audio/mpegurl")    != std::string::npos
				|| contentStr.find("application/m3u")  != std::string::npos);
		if (playlist) {
			hdr = hdr.substr(pos+1);
			pos = hdr.find("http://");
			if (pos != std::string::npos) {
				newurl = hdr.substr(pos);
				pos = hdr.find("\n");
				if (pos != std::string::npos) newurl = newurl.substr(0, pos);
				eDebug("%s: playlist entry: %s", __FUNCTION__, newurl.c_str());
				return 0;
			}
		}
	}

	ssize_t toWrite = m_rbuffer.availableToWritePtr();
	if (toWrite > 0) {
		toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 0, 5);
		if (toWrite > 0) {
                        ssize_t skipBytes = 0;
                        char* ptr = m_rbuffer.ptr();
                        while (skipBytes <= toWrite && ptr[skipBytes] != 0x47) ++skipBytes;
			eDebug("%s: writting %i bytes to the ring buffer", __FUNCTION__, toWrite);
			m_rbuffer.ptrWriteCommit(toWrite);
			m_rbuffer.skip(skipBytes);
		}
	}

	if (m_chunkedTransfer) {
		if (m_chunkedBuffer == NULL) {
			m_chunkedBuffer = (char*)malloc(CHUNKED_BUFF_SIZE);
			if (m_chunkedBuffer == NULL) {
				eDebug("%s: failed to allocate a buffer for chunked transfer", __FUNCTION__);
				return -1;
			}
		}
	} else if (m_chunkedBuffer != NULL) {
		free(m_chunkedBuffer);
		m_chunkedBuffer=NULL;
	}

	return 0;
}

int eHttpStream::close()
{
	eDebug("eHttpStream::close socket");
	int retval = -1;
	if (m_streamSocket >= 0)
	{
		retval = ::close(m_streamSocket);
		m_streamSocket = -1;
	}
	// no reconnect attempts after close
	m_tryToReconnect = false;
	return retval;
}

ssize_t eHttpStream::read(off_t offset, void *buf, size_t count)
{
	if (m_streamSocket < 0) {
		eDebug("eHttpStream::read not valid fd");
		return -1;
	}

	eDebug("eHttpStream::read()");
	ssize_t toWrite = m_rbuffer.availableToWritePtr();
//	eDebug("Ring buffer available to write %i", toWrite);
	if (toWrite > 0) {
		if (m_rbuffer.availableToRead() >= count) {
			//do not starve the reader if we have enough data to read and there is nothing on the socket
			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 0, 50);
		} else {
			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 5000, 500);
		}
		if (toWrite > 0) {
			eDebug("eHttpStream::read() - writting %i bytes to the ring buffer", toWrite);
			m_rbuffer.ptrWriteCommit(toWrite);
			//try to reconnect on next failure
			m_tryToReconnect = true;
		} else if (m_rbuffer.availableToRead() < 188) {
			eDebug("eHttpStream::read() - failed to read from the socket...");
			// we failed to read and there is nothing to play, try to reconnect?
			// so far reconnect worked for me as best effort, it is really doing well 
			// when initial connection fails for some reason.
			if (toWrite < 0 && m_tryToReconnect) {
				if (open(m_url) == 0) {
					// tell the caller to try again
					errno = EAGAIN;
				}
				m_tryToReconnect = false;
				m_rbuffer.reset();
			} else if (toWrite == 0) {
				errno = EAGAIN; //timeout
			} else close();

			return -1 ;
		}
	}

	if (m_chunkedTransfer) {
		ssize_t totalRead=0;
		const ssize_t availableToRead = m_rbuffer.availableToRead();
		eDebug("%s: chunked transfer available to read: %i", __FUNCTION__, availableToRead);
		while(availableToRead > (188*2) && totalRead < (count - count%188)){
			ssize_t toRead = MIN(availableToRead - 188, count);
			eDebug("%s: chunked transfer will read max: %i", __FUNCTION__, toRead - toRead%188);
			toRead = m_rbuffer.read(m_chunkedBuffer, toRead - toRead%188);
			char* ptr = m_chunkedBuffer;
			char* end = (m_chunkedBuffer + toRead);
			while(ptr < end) {
				while(ptr < end && *ptr != 0x47) ++ptr;
				ssize_t count188=0;
				while((ptr + 188) <= end && ptr[count188] == 0x47) count188+=188;
				eDebug("%s: chunked transfer will read %i of 188 bytes", __FUNCTION__, count188/188);
				if (count188 > 0) {
					memcpy((char*)buf+totalRead, ptr, count188);
					totalRead += count188;
					ptr+=count188;
				} else if (ptr < end) {
					memcpy((char*)buf+totalRead, ptr, (end - ptr));
					totalRead += (end - ptr);
					ptr = end;
				}
			}
			if (totalRead%188) {
				ssize_t last188Tail = (188 - (totalRead%188));
				eDebug("%s: chunked transfer will read the last %i of 188 bytes", __FUNCTION__, last188Tail);
				m_rbuffer.read((char*)buf+totalRead, last188Tail);
				totalRead += last188Tail;
			}
			eDebug("%s: chunked transfer read total %i", __FUNCTION__, totalRead);
			return totalRead;
		}
		return totalRead;
	} else {
		ssize_t toRead = m_rbuffer.availableToRead();
		toRead = MIN(toRead, count);
//		eDebug(" eHttpStream::read() - reading %i bytes", (toRead - (toRead%188)));
		toRead = m_rbuffer.read((char*)buf, (toRead - (toRead%188)));
		return (toRead > 0)? toRead: -1;
	}
}

int eHttpStream::valid()
{
	return m_streamSocket >= 0;
}

off_t eHttpStream::length()
{
	return (off_t)-1;
}

off_t eHttpStream::offset()
{
	return 0;
}

