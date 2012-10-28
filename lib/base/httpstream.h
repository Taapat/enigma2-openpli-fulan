#ifndef __lib_base_httpstream_h
#define __lib_base_httpstream_h

#include <string>
#include <lib/base/ebase.h>
#include <lib/base/itssource.h>
#include <lib/base/socketbase.h>
#include <lib/base/thread.h>
#include <lib/base/elock.h>

class eHttpStream: public iTsSource, public eSocketBase, public Object, public eThread
{
	DECLARE_REF(eHttpStream);

	int streamSocket;
	std::string authorizationData;

	int openUrl(const std::string &url, std::string &newurl);

	/* iTsSource */
	ssize_t read(off_t offset, void *buf, size_t count);
	off_t length();
	off_t offset();
	int valid();
	void thread();
	int openHttpConnection();
	int close();
	char *url;
	int ret_code;
	eSingleLock sock_mutex;

public:
	eHttpStream();
	~eHttpStream();
	int open(const char *url);
};

#endif
