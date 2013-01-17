#include <cstdio>
#include <openssl/evp.h>

#include <lib/base/httpstream.h>
#include <lib/base/eerror.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

DEFINE_REF(eHttpStream);

eHttpStream::eHttpStream() : 
	m_streamSocket(-1)
	,m_rbuffer(1024*1024)
	,m_tryToReconnect(false)
{
}

eHttpStream::~eHttpStream()
{
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
	size_t buflen = 1024;
	char *linebuf = NULL;
	int result;
	char proto[100];
	int statuscode = 0;
	char statusmsg[100];
	bool playlist = false;
	bool contenttypeparsed = false;

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
	if (m_streamSocket < 0) goto error;

	request = "GET ";
	request.append(uri).append(" HTTP/1.1\r\n");
	request.append("Host: ").append(hostname).append("\r\n");
	if (m_authorizationData.length())
	{
		request.append("Authorization: Basic ").append(m_authorizationData).append("\r\n");
	}
	request.append("Accept: */*\r\n");
	request.append("Connection: close\r\n");
	request.append("\r\n");

	eSocketBase::writeAll(m_streamSocket, request.data(), request.length());

	linebuf = (char*)malloc(buflen);

	result = eSocketBase::readLine(m_streamSocket, &linebuf, &buflen);
	if (result <= 0) goto error;

	result = sscanf(linebuf, "%99s %3d %99s", proto, &statuscode, statusmsg);
	if (result != 3 || (statuscode != 200 && statuscode != 302))
	{
		eDebug("%s: wrong http response code: %d", __FUNCTION__, statuscode);
		goto error;
	}

	while (1)
	{
		result = eSocketBase::readLine(m_streamSocket, &linebuf, &buflen);
		if (!contenttypeparsed)
		{
			char contenttype[32];
			if (sscanf(linebuf, "Content-Type: %31s", contenttype) == 1)
			{
				contenttypeparsed = true;
				if (!strcmp(contenttype, "application/text")
				|| !strcmp(contenttype, "audio/x-mpegurl")
				|| !strcmp(contenttype, "audio/mpegurl")
				|| !strcmp(contenttype, "application/m3u"))
				{
					/* assume we'll get a playlist, some text file containing a stream url */
					playlist = true;
				}
			}
		}
		else if (playlist && !strncmp(linebuf, "http://", 7))
		{
			newurl = linebuf;
			eDebug("%s: playlist entry: %s", __FUNCTION__, newurl.c_str());
			break;
		}
		else if (statuscode == 302 && !strncmp(linebuf, "Location: ", 10))
		{
			newurl = &linebuf[10];
			eDebug("%s: redirecting to: %s", __FUNCTION__, newurl.c_str());
			break;
		}
		if (!playlist && result == 0) break;
		if (result < 0) break;
	}

	free(linebuf);
	return 0;
error:
	eDebug("%s failed", __FUNCTION__);
	free(linebuf);
	close();
	return -1;
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
		if (m_rbuffer.availableToRead() >= 188*42) {
			//do not starve the reader if we have enough data to read and there is nothing on the socket
			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 0, 50);
		} else {
			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 5000, 50);
		}
                if (toWrite > 0) {
                       	eDebug("eHttpStream::read() - writting %i bytes to the ring buffer", toWrite);
                       	m_rbuffer.ptrWriteCommit(toWrite);
			//try to reconnect on next failure
			m_tryToReconnect = true;
                } else if (m_rbuffer.availableToRead() < 188) {
                       	eDebug("eHttpStream::read() - failed to red from the socket...");
			// we failed to read and there is nothing to play, try to reconnect?
			// so far recoonect worked for me as best effort, it is really doing well 
			// when initial connection fails for some reason.
/* it makes player 191 to crash, commenting out until player's problem is sorted
			if (m_tryToReconnect) {
				if (open(m_url) == 0) {
					// tell the caller to try again
					errno = EAGAIN;
				}
				m_tryToReconnect = false;
			} else close();

			m_rbuffer.reset();
*/
			/*
			 * IF: the reconnect does not really work during more extensive testing,
			 *  then we might want to just close the connection and not watse bandwidth
			 */
                 	return -1 ;
               	} else{}//may be we should try reconnect here?
	}

	ssize_t toRead = m_rbuffer.availableToRead();
	toRead = MIN(toRead, count);
//	eDebug(" eHttpStream::read() - reading %i bytes", (toRead - (toRead%188)));
	toRead = m_rbuffer.read((char*)buf, (toRead - (toRead%188)));
	return (toRead > 0)? toRead: -1;
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

