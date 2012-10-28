#include <cstdio>
#include <openssl/evp.h>

#include <lib/base/httpstream.h>
#include <lib/base/eerror.h>

DEFINE_REF(eHttpStream);

eHttpStream::eHttpStream() : sock_mutex(), ret_code(-1)
{
	streamSocket = -1;
	//TODO: malloc and free
	this->url = new char[1000];
}

eHttpStream::~eHttpStream()
{
	kill();
	close();
}

int eHttpStream::open(const char *url)
{
	// lock the mutex until socket is opened for reading
	sock_mutex.lock();
	strcpy(this->url, url);
	eDebug("http thread");
	int rc = run();
}

void eHttpStream::thread()
{
	hasStarted();
	ret_code = openHttpConnection();
	eDebug("eHttpStream::open: ret %d", ret_code);
	sock_mutex.unlock();
}

int eHttpStream::openHttpConnection()
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
	bool redirected = false;

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
		int length = 0;
		authorizationData = hostname.substr(0, authenticationindex);
		hostname = hostname.substr(authenticationindex + 1);
		mbio = BIO_new(BIO_s_mem());
		b64bio = BIO_new(BIO_f_base64());
		bio = BIO_push(b64bio, mbio);
		BIO_write(bio, authorizationData.c_str(), authorizationData.length());
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
	streamSocket = connect(hostname.c_str(), port, 10);
	if (streamSocket < 0) goto error;

	request = "GET ";
	request.append(uri).append(" HTTP/1.1\r\n");
	request.append("Host: ").append(hostname).append("\r\n");
	if (authorizationData != "")
	{
		request.append("Authorization: Basic ").append(authorizationData).append("\r\n");
	}
	request.append("Accept: */*\r\n");
	request.append("Connection: close\r\n");
	request.append("\r\n");

	writeAll(streamSocket, request.c_str(), request.length());

	linebuf = (char*)malloc(buflen);

	result = readLine(streamSocket, &linebuf, &buflen);
	if (result <= 0) goto error;

	result = sscanf(linebuf, "%99s %d %99s", proto, &statuscode, statusmsg);
	if (result != 3 || (statuscode != 200 && statuscode != 302))
	{
		eDebug("%s: wrong http response code: %d", __FUNCTION__, statuscode);
		goto error;
	}

	while (result > 0)
	{
		result = readLine(streamSocket, &linebuf, &buflen);
		if (statuscode == 302 && sscanf(linebuf, "Location: %999s", this->url) == 1)
		{
				eDebug("eHttpStream::open: redirecting");
				if (openHttpConnection() < 0) goto error;
				redirected = true;
				break;
		}
	}
	if (statuscode == 302 && !redirected) goto error;

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
	if (streamSocket >= 0)
	{
		retval = ::close(streamSocket);
		streamSocket = -1;
	}
	return retval;
}

ssize_t eHttpStream::read(off_t offset, void *buf, size_t count)
{
	sock_mutex.lock();
	sock_mutex.unlock();
	if (streamSocket < 0) {
		eDebug("eHttpStream::read not valid fd");
		return -1;
	}
	return timedRead(streamSocket, buf, count, 5000, 500);
}

int eHttpStream::valid()
{
	return true;
	return streamSocket >= 0;
}

off_t eHttpStream::length()
{
	return (off_t)-1;
}

off_t eHttpStream::offset()
{
	return 0;
}
