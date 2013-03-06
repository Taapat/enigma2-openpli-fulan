#ifndef __lib_base_httpstream_h
#define __lib_base_httpstream_h

#include <string>
#include <lib/base/ebase.h>
#include <lib/base/itssource.h>
#include <lib/base/socketbase.h>
#include <lib/base/thread.h>
#include <lib/base/ringbuffer.h>

class eHttpStream: public iTsSource, public Object
{
	DECLARE_REF(eHttpStream);


	RingBuffer m_rbuffer;

	std::string m_url;
	int m_streamSocket;

	char*  m_scratch;
	size_t m_scratchSize;
	size_t m_contentLength;
	size_t m_contentServed;
	size_t m_currentChunkSize;

	bool m_tryToReconnect;
	bool m_chunkedTransfer;

	int openUrl(const std::string& url, std::string &newurl);
	int _open(const std::string& url);

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
