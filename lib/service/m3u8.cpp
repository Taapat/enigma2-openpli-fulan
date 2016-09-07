#include <lib/base/eerror.h>
#include "m3u8.h"
#include <cstdlib>
#include <cstring>

#define M3U8_HEADER "#EXTM3U"

#define M3U8_STREAM_INFO "#EXT-X-STREAM-INF"
#define M3U8_MEDIA_SEQUENCE "#EXT-X-MEDIA-SEQUENCE"


Url::Url(const std::string& url):
    m_url(url),
    m_port(80)
{
    parseUrl(url);
}

void Url::parseUrl(std::string url)
{
    size_t delim_start = url.find("://");
    if (delim_start == std::string::npos)
        return;
    size_t fragment_start = url.find("#");
    if (fragment_start != std::string::npos)
    {
        m_fragment = url.substr(fragment_start + 1);
        m_url = url = url.substr(0, url.length() - m_fragment.length() - 1);
    }
    m_proto = url.substr(0, delim_start);

    std::string host, path;
    size_t path_start = url.find("/", delim_start + 3);
    if (path_start != std::string::npos)
    {
        path = url.substr(path_start);
        host = url.substr(delim_start + 3, path_start - delim_start - 3);
    }
    else
    {
        host = url.substr(delim_start);
    }
    size_t port_start = host.find(":");
    if (port_start != std::string::npos)
    {
        m_port = atoi(host.substr(port_start + 1).c_str());
        host = host.substr(0, port_start);
    }
    size_t query_start = path.find("?");
    if (query_start != std::string::npos)
    {
        m_query = path.substr(query_start+1);
        path = path.substr(0, query_start);
    }
    m_host = host;
    m_path = path;
}

int M3U8VariantsExplorer::parse_attribute(char **ptr, char **key, char **value)
{
    if (ptr == NULL || *ptr == NULL || key == NULL || value == NULL)
        return -1;

    char *end; 
    char *p;
    p = end = strchr(*ptr, ',');
    if (end)
    {
        char *q = strchr(*ptr, '"');
        if (q && q < end)
        {
            q = strchr(++q, '"');
            if (q)
            {
                p = end = strchr(++q, ',');
            }
        }
    }
    if (end)
    {
        do
        {
            end++;
        }
        while(*end && *end == ' ');
        *p = '\0';
    }

    *key = *ptr;
    p = strchr(*ptr, '=');
    if (!p)
        return -1;
    *p++ = '\0';
    *value = p;
    *ptr = end;
    return 0;
}

//https://tools.ietf.org/html/draft-pantos-http-live-streaming-13#section-3.4.10
int M3U8VariantsExplorer::parseStreamInfoAttributes(const char *attributes, M3U8StreamInfo& info)
{
    char *myline = strdup(attributes);
    char *ptr = myline;
    char *key = NULL;
    char *value = NULL;
    while (!parse_attribute(&ptr, &key, &value))
    {
        if (!strcasecmp(key, "bandwidth"))
            info.bitrate = atoi(value);
        if (!strcasecmp(key, "resolution"))
            info.resolution = value;
        if (!strcasecmp(key, "codecs"))
            info.codecs = value;
    }
    free(myline);
    return 0;
}


int M3U8VariantsExplorer::getVariantsFromMasterUrl(const std::string& url, unsigned int redirect)
{
    if (redirect > redirectLimit)
    {
        eDebug("[m3u8::%s] - reached maximum number of %d - redirects", __func__, redirectLimit);
        return -1;
    }
    Url purl(url);
    int sd;
    if((sd = Connect(purl.host().c_str(), purl.port(), 5)) < 0)
    {
        eDebug("[m3u8::%s] - Error in Connect", __func__);
        return -1;
    }
    //eDebug("[m3u8::%s] - Connect desc = %d", __func__, sd);

    std::string userAgent = "Enigma2 HbbTV/1.1.1 (+PVR+RTSP+DL;OpenPLi;;;)";
    std::string path = purl.path();
    std::string query = purl.query();
    if (!query.empty())
        path += "?" + query;
    std::string request = "GET ";
    request.append(path).append(" HTTP/1.1\r\n");
    request.append("Host: ").append(purl.host()).append("\r\n");
    request.append("User-Agent: ").append(userAgent).append("\r\n");
    request.append("Accept: */*\r\n");
    request.append("Connection: close\r\n");
    request.append("\r\n");

    eDebug("[m3u8::%s] - Request:", __func__);
    eDebug("%s", request.c_str());

    if (writeAll(sd, request.c_str(), request.length()) < (signed long) request.length())
    {
        eDebug("[m3u8::%s] - writeAll, didn't write everything", __func__);
        ::close(sd);
        return -1;
    }
    int lines = 0;

    int contentLength = 0;
    int contentSize = 0;
    bool contentTypeParsed = false;
    bool m3u8HeaderParsed = false;
    bool m3u8StreamInfoParsing = false;
    M3U8StreamInfo m3u8StreamInfo;

    size_t bufferSize = 1024;
    char *lineBuffer = (char *) malloc(bufferSize);

    int statusCode;
    char protocol[64], statusMessage[64];

    int result = readLine(sd, &lineBuffer, &bufferSize);
    eDebug("[m3u8::%s] Response[%d](size=%d): %s", __func__, lines++, result, lineBuffer);
    result = sscanf(lineBuffer, "%99s %d %99s", protocol, &statusCode, statusMessage);
    if (result != 3 || (statusCode != 200 && statusCode != 302))
    {
            eDebug("[m3u8::%s] - wrong http response code: %d", __func__, statusCode);
            free(lineBuffer);
            ::close(sd);
            return -1;
    }
    int ret = -1;
    while(1)
    {
        if (contentLength && contentLength <= contentSize)
        {
            if (!streams.size())
            {
                eDebug("[m3u8::%s] - no streams find!", __func__);
                break;
            }
            ret = 0;
            break;
        }

        result = readLine(sd, &lineBuffer, &bufferSize);
        eDebug("[m3u8::%s] Response[%d](size=%d): %s", __func__, lines++, result, lineBuffer);

        if (result < 0)
        {
            if (!streams.size())
            {
                eDebug("[m3u8::%s] - no streams find!", __func__);
                break;
            }
            ret = 0;
            break;
        }

        if (!contentLength && !m3u8HeaderParsed)
        {
            sscanf(lineBuffer, "Content-Length: %d", &contentLength);
        }

        if (!contentTypeParsed)
        {
            char contenttype[33];
            if (sscanf(lineBuffer, "Content-Type: %32s", contenttype) == 1)
            {
                contentTypeParsed = true;
                if (!strncasecmp(contenttype, "application/text", 16)
                        || !strncasecmp(contenttype, "audio/x-mpegurl", 15)
                        || !strncasecmp(contenttype, "application/x-mpegurl", 21)
                        || !strncasecmp(contenttype, "application/vnd.apple.mpegurl", 29)
                        || !strncasecmp(contenttype, "audio/mpegurl", 13)
                        || !strncasecmp(contenttype, "application/m3u", 15))
                {
                    continue;
                }
                eDebug("[m3u8::%s] - not supported contenttype detected: %s!", __func__, contenttype);
                break;
            }
        }

        if (statusCode == 302 && strncasecmp(lineBuffer, "location: ", 10) == 0)
        {
            std::string newurl = &lineBuffer[10];
            eDebug("[m3u8::%s] - redirecting to: %s", __func__, newurl.c_str());
            ret = getVariantsFromMasterUrl(newurl, ++redirect);
            break;
        }

        if (!m3u8HeaderParsed)
        {
            // find M3U8 header
            if (result && !strncmp(lineBuffer, M3U8_HEADER, strlen(M3U8_HEADER)))
            {
                m3u8HeaderParsed = true;
                contentSize += result + 1; // newline char
            }
            continue;
        }

        contentSize += result + 1; // newline char
        if (!strncmp(lineBuffer, M3U8_MEDIA_SEQUENCE, strlen(M3U8_MEDIA_SEQUENCE)))
        {
            eDebug("[m3u8::%s] - we need master playlist not media sequence!", __func__);
            break;
        }

        if (m3u8StreamInfoParsing)
        {
            // there shouldn't be any empty line
            if (!result)
            {
                m3u8StreamInfoParsing = false;
                continue;
            }

            //eDebug("[m3u8::%s] - continue parsing m3u8 stream info", __func__);
            if (!strncmp(lineBuffer, "http", 4))
            {
                m3u8StreamInfo.url = lineBuffer;
            }
            else
            {
                m3u8StreamInfo.url = url.substr(0, url.rfind('/') + 1) + lineBuffer;
            }
            streams.push_back(m3u8StreamInfo);
            m3u8StreamInfoParsing = false;
        }
        else
        {
            if (!strncmp(lineBuffer, M3U8_STREAM_INFO, 17))
            {
                m3u8StreamInfoParsing = true;
                std::string parsed(lineBuffer);
                parseStreamInfoAttributes(parsed.substr(18).c_str(), m3u8StreamInfo);
            }
            else
            {
                eDebug("[m3u8::%s] - skipping unrecognised data", __func__);
            }
        }
    }
    free(lineBuffer);
    ::close(sd);
    return ret;
}

std::vector<M3U8StreamInfo> M3U8VariantsExplorer::getStreams()
{
    streams.clear();
    std::vector<std::string> masterUrl;
    masterUrl.push_back(url);
    // try also some common master playlist filenames
    // masterUrl.push_back(url.substr(0, url.rfind('/') + 1) + "master.m3u8");
    // masterUrl.push_back(url.substr(0, url.rfind('/') + 1) + "playlist.m3u8");
    for (std::vector<std::string>::const_iterator it(masterUrl.begin()); it != masterUrl.end(); it++)
    {
        int ret = getVariantsFromMasterUrl(*it, 0);
        if (ret < 0)
            continue;
        break;
    }
    return streams;
}

