#include <cstdio>
#include <openssl/evp.h>

#include <lib/base/httpstream.h>
#include <lib/base/eerror.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

DEFINE_REF(eHttpStream);

eHttpStream::eHttpStream() : 
	 m_rbuffer(1024*1024*3)
	,m_url("")
	,m_streamSocket(-1)
	,m_scratch(NULL)
	,m_scratchSize(0)
	,m_contentLength(0)
	,m_contentServed(0)
	,m_currentChunkSize(0)
	,m_tryToReconnect(false)
	,m_chunkedTransfer(false)
{
}

eHttpStream::~eHttpStream()
{
	close();
}

int eHttpStream::open(const std::string& url)
{
	m_url = url;
	return 0;
}

int eHttpStream::_open(const std::string& url)
{
	eDebug("eHttpStream::open()");
	std::string currenturl(url), newurl;

	for (int i = 0; i < 3; i++)
	{
		if (openUrl(currenturl, newurl) < 0)
		{
			close();
			eDebug("%s: connection failed", __FUNCTION__);
			return -1;
		}
		if (newurl.empty())
		{
			/* we have a valid stream connection */
			eDebug("%s: started streaming...", __FUNCTION__);
			return 0;
		}
		/* switch to new url */
		close();
		currenturl.swap(newurl);
		newurl.clear();
	}
	eDebug("%s: too many redirects...", __FUNCTION__);
	/* too many redirect / playlist levels (we accept one redirect + one playlist) */
	close();
	return -1;
}

int eHttpStream::openUrl(const std::string &url, std::string &newurl)
{
	int port;
	std::string hostname;
	std::string authorizationData;
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
		authorizationData = hostname.substr(0, authenticationindex);
		hostname = hostname.substr(authenticationindex + 1);
		mbio = BIO_new(BIO_s_mem());
		b64bio = BIO_new(BIO_f_base64());
		bio = BIO_push(b64bio, mbio);
		BIO_write(bio, authorizationData.data(), authorizationData.length());
		BIO_flush(bio);
		int length = BIO_ctrl(mbio, BIO_CTRL_INFO, 0, (char*)&p);
		authorizationData = "";
		if (p && length > 0)
		{
			/* base64 output contains a linefeed, which we ignore */
			authorizationData.append(p, length - 1);
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

	m_streamSocket = eSocketBase::connect(hostname.c_str(), port, 100);
	if (m_streamSocket < 0) return -1;

	request = "GET ";
	request.append(uri).append(" HTTP/1.1\r\n");
	request.append("Host: ").append(hostname).append("\r\n");
	if (authorizationData.length())
	{
		request.append("Authorization: Basic ").append(authorizationData).append("\r\n");
	}
	request.append("Accept: */*\r\n");
	if (!m_tryToReconnect) {
		m_contentServed = 0;
		request.append("Range: bytes=0-\r\n");
	} else {
		char buffer [33];
		sprintf(buffer, "%d", m_contentServed);
		request.append("Range: bytes=");
		request.append(buffer);
		request.append("-\r\n");

	}	
	request.append("Connection: close\r\n");
	request.append("\r\n");

	std::string hdr;
	result = eSocketBase::openHTTPConnection(m_streamSocket, request, hdr);
	if (result < 0) return -1;

	size_t pos = hdr.find("\n");
	const std::string& httpStatus = (pos == std::string::npos)? hdr: hdr.substr(0, pos);

	result = sscanf(httpStatus.c_str(), "%99s %3d %99s", proto, &statuscode, statusmsg);
	if (result != 3 || (statuscode != 200 && statuscode != 206 && statuscode != 302))
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
	pos = hdr.find("Content-Length:");
	if (pos != std::string::npos) {
		std::string clen = hdr.substr(pos+strlen("Content-Length:"));
		pos = clen.find("\n");
		if (pos != std::string::npos) {
			clen = clen.substr(0, pos);
		}
		m_contentLength = strtol(clen.c_str(), NULL, 16);
	}

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

	if (m_chunkedTransfer) {
		eDebug("%s: chunked transfer enabled", __FUNCTION__);
		if (m_scratch == NULL) { m_scratchSize=64; m_scratch=(char*)malloc(m_scratchSize);}
		int c = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
		if (c <=0) return -1;
		m_currentChunkSize = strtol(m_scratch, NULL, 16);
		eDebug("%s: chunked transfer enabled, fisrt chunk size %i", __FUNCTION__, m_currentChunkSize);
	}

	m_rbuffer.reset();
	ssize_t toWrite = m_rbuffer.availableToWritePtr();
	if (toWrite > 0) {
		if (m_chunkedTransfer) toWrite = MIN(toWrite, m_currentChunkSize);
		toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 2000, 50);
		if (toWrite > 0) {
			eDebug("%s: writting %i bytes to the ring buffer", __FUNCTION__, toWrite);
                        ssize_t skipBytes = 0;
                        char* ptr = m_rbuffer.ptr();
                        while (skipBytes <= toWrite && ptr[skipBytes] != 0x47) ++skipBytes;
			m_rbuffer.ptrWriteCommit(toWrite);
			m_rbuffer.skip(skipBytes);

			if (m_chunkedTransfer) {
				m_currentChunkSize -= toWrite;
                                if (m_currentChunkSize==0){
                                        int rc = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
                                        eDebug("%s: reading the end of the chunk rc(%i)(%s)", __FUNCTION__, rc, m_scratch);
                                }
			}
		}
	}

	return 0;
}

int eHttpStream::close()
{
	eDebug(__FUNCTION__);
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
		eDebug("%s: not valid fd", __FUNCTION__);
		if (_open(m_url) < 0) return -1;
	}

	bool outBufferHasData = false;
	if (m_rbuffer.availableToRead() >= count) {
        	m_contentServed += m_rbuffer.read((char*)buf, (count - (count%188)));
		outBufferHasData = true;
	}
	

	int read2Chunks=2;
READAGAIN:
	ssize_t toWrite = m_rbuffer.availableToWritePtr();
	eDebug("%s: Ring buffer available to write %i", __FUNCTION__, toWrite);
	if (toWrite > 0) {

		if (m_chunkedTransfer){
			if (m_currentChunkSize==0) {
				int c = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
				if (c <= 0) return -1;
				m_currentChunkSize = strtol(m_scratch, NULL, 16);
				if (m_currentChunkSize == 0) return -1;
			}
			toWrite = MIN(toWrite, m_currentChunkSize);
		}
		if (outBufferHasData || m_rbuffer.availableToRead() >= (1024*6) || m_rbuffer.availableToRead() >= count) {
			//do not starve the reader if we have enough data to read and there is nothing on the socket
			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 0, 0);
		} else {
			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 3000, 500);
		}
		if (toWrite > 0) {
			m_rbuffer.ptrWriteCommit(toWrite);
			if (m_chunkedTransfer) {
                                --read2Chunks;
				m_currentChunkSize -= toWrite;
				if (m_currentChunkSize==0){
					eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
				} else if (read2Chunks > 0) {
					goto READAGAIN;
				}
			}
			//try to reconnect on next failure
			m_tryToReconnect = true;
		} else if (!outBufferHasData && m_rbuffer.availableToRead() < 188) {
			eDebug("%s: failed to read from the socket errno(%i) timedRead(%i)...", __FUNCTION__, errno, toWrite);
			// we failed to read and there is nothing to play, try to reconnect?
			// for some reason select() returns EAGAIN sometimes
			// do not reconnect if sream has a content length and all lentgh is served
			if ((toWrite <= 0 || errno == EAGAIN) && m_tryToReconnect && (!m_contentLength || m_contentLength > m_contentServed)) {
				m_tryToReconnect = false;
				if (_open(m_url) == 0) {
					goto READAGAIN;
				}
			} else if (toWrite == 0 && (!m_contentLength || m_contentLength > m_contentServed)) {
				errno = EAGAIN; //timeout
			} else close();

			eDebug("%s: timed out", __FUNCTION__);
			return -1 ;
		}
	}

	if (!outBufferHasData) {
		ssize_t toRead = MIN(count, m_rbuffer.availableToRead());
		toRead = m_rbuffer.read((char*)buf, (toRead - (toRead%188)));
		m_contentServed += toRead;
		return toRead;
	} else {
		return (count - (count%188));
	}
}

int eHttpStream::valid()
{
	return 1;
//	return (m_streamSocket != -1);
}

off_t eHttpStream::length()
{
	return (off_t)-1;
}

off_t eHttpStream::offset()
{
	return 0;
}

