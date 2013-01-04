#ifndef __lib_base_httpstream_h
#define __lib_base_httpstream_h

#include <string>
#include <lib/base/ebase.h>
#include <lib/base/itssource.h>
#include <lib/base/socketbase.h>
#include <lib/base/thread.h>
#include <lib/base/elock.h>

class eHttpStream: public iTsSource, public Object, public eThread
{
	DECLARE_REF(eHttpStream);

	int m_streamSocket;
	std::string m_authorizationData;
        std::string m_url;

	int openUrl(const std::string &url, std::string &newurl);

        void thread();

	/* iTsSource */
	ssize_t read(off_t offset, void *buf, size_t count);
	off_t length();
	off_t offset();
	int valid();
	int close();

public:
	eHttpStream();
	~eHttpStream();
	int open(const std::string& url);
};

#endif
