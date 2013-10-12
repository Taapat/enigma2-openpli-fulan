#ifndef __lib_base_httpstream_h
#define __lib_base_httpstream_h

#include <string>
#include <lib/base/ebase.h>
#include <lib/base/itssource.h>
#include <lib/base/thread.h>

class eHttpStream: public iTsSource, public Object, public eThread
{
	DECLARE_REF(eHttpStream);

	int m_streamSocket;
	bool m_isChunked;
	size_t m_currentChunkSize;
	char m_partialPkt[188];
	int  m_partialPktSz;
	enum { BUSY, CONNECTED, FAILED } m_connectionStatus;
	char* m_tmp;
	size_t m_tmpSize;
	std::string m_streamUrl;

	int openUrl(const std::string &url, std::string &newurl);
	ssize_t filter(void* buf, size_t count);
	ssize_t httpChunkedRead(void* buf, size_t count);
	void thread();

	/* iTsSource */
	ssize_t read(off_t offset, void *buf, size_t count);
	off_t length();
	off_t offset();
	int valid();
	bool isStream() { return true; };

public:
	eHttpStream();
	~eHttpStream();
	int open(const char *url);
	int close();
};

#endif
