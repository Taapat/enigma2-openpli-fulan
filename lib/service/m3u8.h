#ifndef __m3u8variant__h
#define __m3u8variant__h
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <lib/base/wrappers.h>

class Url
{
    std::string m_url;
    std::string m_proto;
    std::string m_host;
    unsigned int m_port;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    std::string url() const;
    std::string host() const;
    unsigned int port() const;
    std::string path() const;
    std::string query() const;
    std::string fragment() const;
    void parseUrl(const std::string url);

public:
    Url(const std::string &url);
    std::string url(){return m_url;}
    std::string proto(){return m_proto;}
    std::string host(){return m_host;}
    int port(){return m_port;}
    std::string path(){return m_path;}
    std::string query(){return m_query;}
    std::string fragment(){return m_fragment;}
};

struct M3U8StreamInfo
{
    std::string url;
    std::string codecs;
    std::string resolution;
    // TODO audio/video/subtitles..
    unsigned long int bitrate;

    bool operator<(const M3U8StreamInfo& m) const
    {
        return bitrate < m.bitrate;
    }
};

class M3U8VariantsExplorer
{
    std::string url;
    std::vector<M3U8StreamInfo> streams;
    const unsigned int redirectLimit;
    int parse_attribute(char **ptr, char **key, char **value);
    int parseStreamInfoAttributes(const char *line, M3U8StreamInfo& info);
    int getVariantsFromMasterUrl(const std::string& url, unsigned int redirect);
public:
    M3U8VariantsExplorer(const std::string& url):
        url(url),
        redirectLimit(3){};
    std::vector<M3U8StreamInfo> getStreams();

};
#endif

