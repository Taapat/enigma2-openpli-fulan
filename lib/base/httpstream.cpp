#include <cstdio>
#include <openssl/evp.h>

#include <lib/base/httpstream.h>
#include <lib/base/eerror.h>
#include <lib/base/wrappers.h>
#include <sys/socket.h>

DEFINE_REF(eHttpStream);

eHttpStream::eHttpStream():
	m_streamSocket(-1)
	,m_isChunked(false)
	,m_currentChunkSize(0)
	,m_connectionStatus(FAILED)
	,m_tmp(NULL)
	,m_tmpSize(32)
	,m_partialPktSz(0)
{
	m_tmp = (char*)malloc(m_tmpSize);
}

eHttpStream::~eHttpStream()
{
	if (m_tmp) free(m_tmp);
	kill(true);
	close();
}

int eHttpStream::openUrl(const std::string &url, std::string &newurl)
{
	int port;
	std::string hostname;
	std::string uri = url;
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
		uri = "/";
	}
	size_t authenticationindex = hostname.find("@");
	std::string authorizationData;
	if (authenticationindex > 0)
	{
		BIO *mbio, *b64bio, *bio;
		char *p = (char*)NULL;
		int length = 0;
		authorizationData = hostname.substr(0, authenticationindex);
		hostname = hostname.substr(authenticationindex + 1);
		mbio = BIO_new(BIO_s_mem());
		b64bio = BIO_new(BIO_f_base64());
		bio = BIO_push(b64bio, mbio);
		BIO_write(bio, authorizationData.data(), authorizationData.length());
		BIO_flush(bio);
		length = BIO_ctrl(mbio, BIO_CTRL_INFO, 0, (char*)&p);
		authorizationData = "";
		if (p && length > 0)
		{
			/* base64 output contains a linefeed, which we ignore */
			authorizationData.append(p, length - 1);
		}
		BIO_free_all(bio);
	}
	size_t customportindex = hostname.find(":");
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
	m_streamSocket = Connect(hostname.c_str(), port, 10);
	if (m_streamSocket < 0)
	{
		eDebug("%s: could not connect : %s", __FUNCTION__, strerror(errno));
		close();
		return -1;
	}

	std::string request = "GET ";
	request.append(uri).append(" HTTP/1.1\r\n");
	request.append("Host: ").append(hostname).append("\r\n");
	if (authorizationData != "")
	{
		request.append("Authorization: Basic ").append(authorizationData).append("\r\n");
	}
	request.append("Accept: */*\r\n");
	request.append("Connection: close\r\n\r\n");

	std::string hdr;
	result = openHTTPConnection(m_streamSocket, request, hdr);
	if (result < 0){
		eDebug("%s: did not receive http header : %s", __FUNCTION__, strerror(errno));
		close();
		return -1;
	}

	size_t pos = hdr.find("\n");
	const std::string& httpStatus = (pos == std::string::npos)? hdr: hdr.substr(0, pos);

	result = sscanf(httpStatus.c_str(), "%99s %3d %99s", proto, &statuscode, statusmsg);
	if (result != 3 || (statuscode != 200 && statuscode != 206 && statuscode != 302))
	{
		eDebug("%s: wrong http response code: %d", __FUNCTION__, statuscode);
		eDebug("%s: http response header: %s", __FUNCTION__, hdr.c_str());
		close();
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

	m_isChunked = (hdr.find("Transfer-Encoding: chunked") != std::string::npos);
/*
	pos = hdr.find("Content-Length:");
	if (pos != std::string::npos) {
		std::string clen = hdr.substr(pos+strlen("Content-Length:"));
		pos = clen.find("\n");
		if (pos != std::string::npos) {
			clen = clen.substr(0, pos);
		}
		m_contentLength = strtol(clen.c_str(), NULL, 16);
	}
*/
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
			}
		}
	}

	// kernel will normally double this value, if it does not exceed max configured
	int sndbuf = 1024*1024;
	//try and set a bigger receive buffer, ask OS to do some buffering for us
	setsockopt(m_streamSocket, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof sndbuf);  

	return 0;
}

int eHttpStream::open(const char *url)
{
	m_streamUrl = url;
	/*
	 * We're in gui thread context here, and establishing
	 * a connection might block for up to 10 seconds.
	 * Spawn a new thread to establish the connection.
	 */
	m_connectionStatus = BUSY;
	eDebug("eHttpStream::Start thread");
	run();
	return 0;
}

void eHttpStream::thread()
{
	hasStarted();
	std::string currenturl(m_streamUrl), newurl;

	for (unsigned int i = 0; i < 3; i++)
	{
		if (openUrl(currenturl, newurl) < 0)
		{
			/* connection failed */
			eDebug("%s: connection failed", __FUNCTION__);
			m_connectionStatus = FAILED;
			return;
		}
		if (newurl.empty())
		{
			/* we have a valid stream connection */
			eDebug("%s: started streaming...", __FUNCTION__);
			m_connectionStatus = CONNECTED;
			return;
		}
		/* switch to new url */
		close();
		currenturl.swap(newurl);
		newurl.clear();
	}
	/* too many redirect / playlist levels (we accept one redirect + one playlist) */
	eDebug("%s: too many redirects...", __FUNCTION__);
	m_connectionStatus = FAILED;
	return;
}

int eHttpStream::close()
{
	int retval = -1;
	if (m_streamSocket >= 0)
	{
		retval = ::close(m_streamSocket);
		m_streamSocket = -1;
	}
	return retval;
}

ssize_t eHttpStream::httpChunkedRead(void* buf, size_t count)
{
	ssize_t ret = -1;
	size_t total_read = m_partialPktSz;

	// write partial packet from the previous read
	if (m_partialPktSz > 0) memcpy(buf, m_partialPkt, m_partialPktSz);

	if (m_isChunked)
	{
		while (total_read < count)
		{
			if (m_currentChunkSize==0)
			{
				do 
				{
					ret = readLine(m_streamSocket, &m_tmp, &m_tmpSize);
					if (ret < 0) return -1;
				}while(!*m_tmp && ret > 0); /* skip CR LF from last chunk */
				if (ret == 0) break;
				m_currentChunkSize = strtol(m_tmp, NULL, 16);
				if (m_currentChunkSize == 0) return -1;
			}

			// filter the stream, when strating streamming or read should start at the start of the packet
			if (total_read==0 && m_partialPktSz==0)
			{
				ret = timedRead(m_streamSocket, m_partialPkt, sizeof m_partialPkt, 5000, 100);
				if (ret <= 0) return ret;
				m_currentChunkSize -= ret;
				int i=0;
				while(i < sizeof(m_partialPkt) && m_partialPkt[i] != 0x47) i++;
				if (i < sizeof(m_partialPkt))
				{
					total_read = ret - i;
					memcpy(buf, m_partialPkt+i, total_read);
				} 
			}
			size_t to_read = count - total_read;
			if (m_currentChunkSize < to_read) to_read = m_currentChunkSize;

			// do not wait too long if we have something in the buffer already
			ret = timedRead(m_streamSocket, buf+total_read, to_read, ((total_read)? 100:5000), 100);
			if (ret <=0) break;
			m_currentChunkSize -= ret;
			total_read += ret;
		}
		if (total_read > 0) ret = total_read;
	}
	else
	{
		// filter the stream, when strating streamming or read should start at the start of the packet
		if (m_partialPktSz==0)
		{
			ret = timedRead(m_streamSocket, m_partialPkt, sizeof m_partialPkt, 5000, 100);
			if (ret <= 0) return ret;
			int i=0;
			while(i < sizeof(m_partialPkt) && m_partialPkt[i] != 0x47) i++;
			if (i < sizeof(m_partialPkt))
			{
				m_partialPktSz = ret-i;
				memcpy(buf, m_partialPkt+i, m_partialPktSz);
			} 
		}

		ret = timedRead(m_streamSocket, buf+m_partialPktSz, count-m_partialPktSz, 5000, 100);
		if (ret > 0) ret+=m_partialPktSz;
	}

	if (ret > 0)
	{
		m_partialPktSz = ret%188;
		ret = ret - m_partialPktSz;
		memcpy(m_partialPkt, buf+ret, m_partialPktSz);		
	}
	return ret;
}
 
ssize_t eHttpStream::read(off_t offset, void *buf, size_t count)
{
	if (m_connectionStatus == CONNECTED)
	{
		return httpChunkedRead(buf, count);
	}
	else if (m_connectionStatus == BUSY)
	{
		return 0;
	}

	return -1;
}

int eHttpStream::valid()
{
	if (m_connectionStatus == BUSY)
		return 0;
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
